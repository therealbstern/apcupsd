/*
 *  apcipc.c  -- IPC functions
 *
 *  Copyright (C) 1999 Jonathan H N Chin <jc254@newton.cam.ac.uk>
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		     BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		  -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  All rights reserved.
 *
 */

/*
 *			   GNU GENERAL PUBLIC LICENSE
 *			      Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *				 675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS "APCUPSD-Team" BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE "APCUPSD-Team" HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE "APCUPSD-Team" SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN "AS IS" BASIS, AND THE "APCUPSD-Team" HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE "APCUPSD-Team" HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, "APCC".  THE "APCUPSD-Team" DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH "APCC".  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */

/*
 *  Contributed by Facchetti Riccardo <riccardo@master.oasi.gpa.it>
 *
 *  Rewritten locking to use sem.h
 *  -RF
 *
 *  Thanks to Jonathan for his locking algoritm.
 */

#include "apc.h"

#ifdef HAVE_PTHREADS

int init_ipc(UPSINFO *ups)
{
    int stat;

    if ((stat = pthread_mutex_init(&ups->mutex, NULL)) != 0) {
        Error_abort1("Could not create pthread mutex. ERR=%s\n",
	   strerror(stat));
	return FAILURE;
    }
    return SUCCESS;
}

int attach_ipc(UPSINFO *ups, int shmperm)
{
    return SUCCESS;
}

int detach_ipc(UPSINFO *ups)
{
    return SUCCESS;
}

int destroy_ipc(UPSINFO *ups)
{
    pthread_mutex_destroy(&ups->mutex);
    return SUCCESS;
}

int read_lock(UPSINFO *ups)
{
    P(ups->mutex);
    return SUCCESS;
}

int read_unlock(UPSINFO *ups)
{
    V(ups->mutex);
    return SUCCESS;
}

int write_lock(UPSINFO *ups)
{
    P(ups->mutex);
    return SUCCESS;
}

int write_unlock(UPSINFO *ups)
{
    V(ups->mutex);
    return SUCCESS;
}

int read_shmarea(UPSINFO *ups, int lock)
{
    return SUCCESS;
}

int write_shmarea(UPSINFO *ups)
{
    return SUCCESS;
}


/*
 * To do at the very start of a thread, to sync data from the common data
 * structure.
 */
int read_andlock_shmarea(UPSINFO *ups)
{
    if (write_lock(ups) == FAILURE)
	return FAILURE;
    return SUCCESS;
}

/*
 * To do at the very end of a thread, to sync data to the common data
 * structure.
 */
int write_andunlock_shmarea(UPSINFO *ups)
{
    return write_unlock(ups);
}

/*
 * For read-only requests.  No copying is done.
 * Danger! Use with caution:
 *
 * 1) Don't modify anything in the shmUPS-structure while holding this lock.
 * 2) Don't hold the lock for too long.
 */
UPSINFO *lock_shmups(UPSINFO *ups)
{
    if (read_lock(ups) == FAILURE)
	return NULL;
    return ups;
}

void unlock_shmups(UPSINFO *ups)
{
    read_unlock(ups);
}


#else

#define SEMBUF ((struct sembuf *)(&(ups->semUPS)))

/* Max re-tries for obtaining a shm segment */
#define MAX_TRIES 1000


/* Forward referenced subroutines */
static int create_shmarea(UPSINFO *ups, int shmperm);
static int attach_shmarea(UPSINFO *ups, int shmperm);
static int detach_shmarea(UPSINFO *ups);
static int destroy_shmarea(UPSINFO *ups);

/*
 * Dealing with shm we cannot simply memcpy because we would overwrite
 * the shmUPS pointer. We have to save it and restore after the copy.
 */
static void shmcpyUPS(UPSINFO *dst, UPSINFO *src)
{
    UPSINFO *ptr;

    ptr = dst->shmUPS;
    memcpy(dst, src, sizeof(UPSINFO));
    dst->shmUPS = ptr;
}

int init_ipc(UPSINFO *ups)
{
    /*
     * If we fail initializing IPC structures it is better destroy any
     * semi-initialized IPC.
     */
    if (create_shmarea(ups, 0644) == FAILURE)
	return FAILURE;

    if (create_semaphore(ups) == FAILURE) {
	destroy_shmarea(ups);
	return FAILURE;
    }
    return SUCCESS;
}

int attach_ipc(UPSINFO *ups, int shmperm)
{
    /*
     * If we fail attaching IPC structures, the problem is local so return
     * failure but don't touch already initialized IPC structures.
     */
    if (attach_shmarea(ups, shmperm) == FAILURE)
	return FAILURE;
    if (attach_semaphore(ups) == FAILURE)
	return FAILURE;
    return SUCCESS;
}

int detach_ipc(UPSINFO *ups)
{
    if (detach_shmarea(ups) == FAILURE) {
	return FAILURE;
    }
    return SUCCESS;
}

int destroy_ipc(UPSINFO *ups)
{
    if (destroy_shmarea(ups) == FAILURE) {
	return FAILURE;
    }
    if (destroy_semaphore(ups) == FAILURE) {
	return FAILURE;
    }
    return SUCCESS;
}

int read_lock(UPSINFO *ups)
{
    /*
     * Acquire writer lock to be sure no other write is in progress.
     */
    ups->semUPS[0].sem_num = WRITE_LCK;
    ups->semUPS[0].sem_op = -1;
    ups->semUPS[0].sem_flg = SEM_UNDO;
    /*
     * Increment reader counter.
     */
    ups->semUPS[1].sem_num = READ_CNT;
    ups->semUPS[1].sem_op = +1;
    ups->semUPS[1].sem_flg = SEM_UNDO;
    /*
     * Release writer lock: we don't need it any more.
     */
    ups->semUPS[2].sem_num = WRITE_LCK;
    ups->semUPS[2].sem_op = +1;
    ups->semUPS[2].sem_flg = SEM_UNDO;

    /*
     * Note that the above three operations are atomic.
     */
    if (semop(ups->idsemUPS, SEMBUF, 3) == -1) {
        log_event(ups, LOG_WARNING, _("read_lock: cannot increment read cnt. ERR=%s"),
	    strerror(errno));
	return FAILURE;
    }
    return SUCCESS;
}

int read_unlock(UPSINFO *ups)
{
    /*
     * Reader counter must be decremented
     */
    ups->semUPS[0].sem_num = READ_CNT;
    ups->semUPS[0].sem_op = -1;
    ups->semUPS[0].sem_flg = SEM_UNDO;

    if (semop(ups->idsemUPS, SEMBUF, 1) == -1) {
        log_event(ups, LOG_WARNING, _("read_unlock: cannot unlock read sem. ERR=%s"),
	    strerror(errno));
	return FAILURE;
    }
    return SUCCESS;
}

int write_lock(UPSINFO *ups)
{
    /*
     * Acquire writer lock to be sure no other write is in progress.
     */
    ups->semUPS[0].sem_num = WRITE_LCK;
    ups->semUPS[0].sem_op = -1;
    ups->semUPS[0].sem_flg = SEM_UNDO;
    if (semop(ups->idsemUPS, SEMBUF, 1) == -1) {
        log_event(ups, LOG_WARNING, _("write_lock: cannot acquire write lock. ERR=%s"),
	    strerror(errno));
	return FAILURE;
    }
    /*
     * Reader counter must be zero.
     */
    ups->semUPS[0].sem_num = READ_CNT;
    ups->semUPS[0].sem_op = 0;
    ups->semUPS[0].sem_flg = 0;

    if (semop(ups->idsemUPS, SEMBUF, 1) == -1) {
        log_event(ups, LOG_WARNING, _("write_lock: cannot assert write sem. ERR=%s"),
	    strerror(errno));
	return FAILURE;
    }

    /*
     * Note that the two operations above are _not_ atomic since when we
     * get the write lock, we can still have readers and threads that
     * may want to increment reader counter and we cannot allow new
     * readers when we are trying to acquire writer lock. After locking
     * for write, we can wait for reader counter to become zero because
     * no other reader increment is possible.
     *
     */

    return SUCCESS;
}

int write_unlock(UPSINFO *ups)
{
    /*
     * Release writer lock.
     */
    ups->semUPS[0].sem_num = WRITE_LCK;
    ups->semUPS[0].sem_op = +1;
    ups->semUPS[0].sem_flg = SEM_UNDO;

    if (semop(ups->idsemUPS, SEMBUF, 1) == -1) {
        log_event(ups, LOG_WARNING, _("write_unlock: cannot unlock write sem. ERR=%s"),
	    strerror(errno));
	return FAILURE;
    }
    return SUCCESS;
}

int create_semaphore(UPSINFO *ups)
{
    union semun apcsun;
    unsigned short int vals[NUM_SEM];
    int i;
    int found = FALSE;

    vals[READ_CNT] = 0;
    vals[WRITE_LCK] = 1;

    /*
     * Get the semaphores.
     */
    for (i=0; i<MAX_TRIES; i++) {
	if ((ups->idsemUPS =
		semget(ups->sem_id, NUM_SEM, IPC_CREAT | 0644)) == -1) {
            log_event(ups, LOG_WARNING, _("Cannot create semaphore for key=%x, retrying... ERR=%s\n"),
		ups->sem_id, strerror(errno));
	    ups->sem_id++;		/* try another one */
	    continue;
	}
	found = TRUE;
	break;
    }
    if (!found) {
        log_event(ups, LOG_ERR, _("Unable to create semaphore. ERR=%s\n"),
	      ups->sem_id, strerror(errno));
	return FAILURE;
    }

    /*
     * Now set them up.
     */
    apcsun.array = vals;
    if (semctl(ups->idsemUPS, 0, SETALL, apcsun) == -1) {
        log_event(ups, LOG_ERR, _("create_semaphore: cannot set up sem. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }

    return SUCCESS;
}

int attach_semaphore(UPSINFO *ups)
{
    if ((ups->idsemUPS = semget(ups->sem_id, NUM_SEM, 0)) == -1) {
        log_event(ups, LOG_ERR, _("attach_semaphore: cannot attach sem. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }
    return SUCCESS;
}

int destroy_semaphore(UPSINFO *ups)
{
    union semun arg;

    if (ups->idsemUPS == -1)
	return FAILURE; 	/* already destroyed */
    if (semctl(ups->idsemUPS, 0, IPC_RMID, arg) == -1) {
        log_event(ups, LOG_ERR, _("destroy_semaphore: cannot destroy sem. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }
    ups->idsemUPS = -1;
    return SUCCESS;
}

/*
 * Theory of operation:
 *
 * we create an shm for the size of UPSINFO. This area will be used by all the
 * threads to get the data from serial polling thread.
 * To make sure to have a valid UPSINFO struct we use a semaphore that will be
 * asserted only when the serial polling thread is writing to the shm area so
 * that no other thread may read from that area until the write is done.
 * Multiple read lock is permitted.
 */
static int create_shmarea(UPSINFO *ups, int shmperm)
{
    int i;
    int found = FALSE;

    for (i=0; i<MAX_TRIES; i++) {
	if ((ups->idshmUPS = shmget(ups->shm_id, sizeof(UPSINFO),
		    IPC_CREAT | shmperm)) == -1) {
	    log_event(ups, LOG_WARNING,
               _("Cannot create shared memory area for key=%x, retrying... ERR=%s\n"),
		  ups->shm_id, strerror(errno));
	    ups->shm_id++;		/* try another id */
	    ups->sem_id++;		/* increment semaphore id too */
	    continue;
	}
	found = TRUE;
	break;
    }
    if (!found) {
       log_event(ups, LOG_WARNING, 
           _("Unable to create shared memory area. ERR=%s\n"),
	      strerror(errno));
       return FAILURE;
    }

    if ((ups->shmUPS = (UPSINFO *)shmat(ups->idshmUPS, 0, 0))
	== (UPSINFO *)-1) {
        log_event(ups, LOG_ERR, _("create_shmarea: cannot attach shm area. ERR=%s\n"),
	    strerror(errno));
	destroy_shmarea(ups);
	return FAILURE;
    }

    return SUCCESS;
}

/*
 * This routine is used by any other program or thread to attach to
 * the shared memory.
 */
static int attach_shmarea(UPSINFO *ups, int shmperm)
{
    if ((ups->idshmUPS = shmget(ups->shm_id, sizeof(UPSINFO), 0)) == -1) {
        log_event(ups, LOG_ERR, _("attach_shmarea: cannot get shm area. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }
    if ((ups->shmUPS = (UPSINFO *)shmat(ups->idshmUPS, 0, shmperm))
	== (UPSINFO *)-1) {
        log_event(ups, LOG_ERR, _("attach_shmarea: cannot attach shm area. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }
    /*
     * Make sure that our layout of the shared memory is the
     * * same as that to which we attached.
     */
    if (strncmp(ups->shmUPS->id, UPSINFO_ID, sizeof(ups->shmUPS->id)) != 0
	|| ups->shmUPS->version != UPSINFO_VERSION
	|| ups->shmUPS->size != sizeof(UPSINFO)) {
        Error_abort0(_("attach_shmarea: shared memory version mismatch "
                "(or UPS not yet ready to report)\n"));
    }

    return SUCCESS;
}

static int detach_shmarea(UPSINFO *ups)
{
    if (shmdt((char *)ups->shmUPS) != 0)
	return FAILURE;
    return SUCCESS;
}

static int destroy_shmarea(UPSINFO *ups)
{
    if (ups->idshmUPS == -1)
	return FAILURE; 	/* already destroyed */
    if (shmctl(ups->idshmUPS, IPC_RMID, NULL) == -1) {
        log_event(ups, LOG_ERR, _("destroy_shmarea: cannot destroy shm area. ERR=%s\n"),
	    strerror(errno));
	return FAILURE;
    }
    ups->idshmUPS = -1;
    return SUCCESS;
}

/* Set lock TRUE if you want the area locked before read
 * otherwise it is not locked. Not locking, permits
 * non-root programs to read the shared memory area.
 */
int read_shmarea(UPSINFO *ups, int lock)
{
    if (lock) {
	if (read_lock(ups) == FAILURE)
	    return FAILURE;
    }
    shmcpyUPS(ups, ups->shmUPS);
    if (lock)
	return read_unlock(ups);
    return SUCCESS;
}

int write_shmarea(UPSINFO *ups)
{
    if (write_lock(ups) == FAILURE)
	return FAILURE;
    shmcpyUPS(ups->shmUPS, ups);
    return write_unlock(ups);
}

/*
 * To do at the very start of a thread, to sync data from the common data
 * structure.
 */
int read_andlock_shmarea(UPSINFO *ups)
{
    if (write_lock(ups) == FAILURE)
	return FAILURE;
    Dmsg1(80, "RLS1 OB:%d\n", ups->OnBatt);
    shmcpyUPS(ups, ups->shmUPS);
    Dmsg1(80, "RLS2 OB:%d\n", ups->OnBatt);
    return SUCCESS;
}

/*
 * To do at the very end of a thread, to sync data to the common data
 * structure.
 */
int write_andunlock_shmarea(UPSINFO *ups)
{
    Dmsg1(80, "RUS1 OB:%d\n", ups->OnBatt);
    shmcpyUPS(ups->shmUPS, ups);
    Dmsg1(80, "RUS2 OB:%d\n", ups->OnBatt);
    return write_unlock(ups);
}

/*
 * For read-only requests.  No copying is done.
 * Danger! Use with caution:
 *
 * 1) Don't modify anything in the shmUPS-structure while holding this lock.
 * 2) Don't hold the lock for too long.
 */
UPSINFO *lock_shmups(UPSINFO *ups)
{
    if (read_lock(ups) == FAILURE)
	return NULL;
    return ups->shmUPS;
}

void unlock_shmups(UPSINFO *ups)
{
    read_unlock(ups);
}

#endif
