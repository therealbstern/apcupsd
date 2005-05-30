/*
 * apcipc.c
 *
 * IPC functions
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999 Jonathan H N Chin <jc254@newton.cam.ac.uk>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"

#ifdef HAVE_PTHREADS

UPSINFO *new_ups()
{
   int stat;
   UPSINFO *ups;

   ups = (UPSINFO *) malloc(sizeof(UPSINFO));
   if (!ups)
      Error_abort0(_("Could not allocate ups memory\n"));

   memset(ups, 0, sizeof(UPSINFO));
   if ((stat = pthread_mutex_init(&ups->mutex, NULL)) != 0) {
      Error_abort1("Could not create pthread mutex. ERR=%s\n", strerror(stat));
      free(ups);
      return NULL;
   }

   ups->refcnt = 1;
   return ups;
}

/* Attach to ups structure */
UPSINFO *attach_ups(UPSINFO *ups, int shmperm)
{
   if (!ups)
      return new_ups();

   P(ups->mutex);
   ups->refcnt++;
   V(ups->mutex);

   return ups;
}

int detach_ups(UPSINFO *ups)
{
   ups->refcnt--;
   if (ups->refcnt == 0)
      return destroy_ups(ups);

   return SUCCESS;
}

int destroy_ups(UPSINFO *ups)
{
   pthread_mutex_destroy(&ups->mutex);
   if (ups->refcnt == 0)
      free(ups);

   return SUCCESS;
}

int _read_lock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_lock at %s:%d\n", file, line);
   P(ups->mutex);
   return SUCCESS;
}

int _read_unlock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_unlock at %s:%d\n", file, line);
   V(ups->mutex);
   return SUCCESS;
}

int _write_lock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_lock at %s:%d\n", file, line);
   P(ups->mutex);
   return SUCCESS;
}

int _write_unlock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_unlock at %s:%d\n", file, line);
   V(ups->mutex);
   return SUCCESS;
}

#else

#define SEMBUF ((struct sembuf *)(&(ups->semUPS)))

/* Max re-tries for obtaining a shm segment */
#define MAX_TRIES 1000


/* Forward referenced subroutines */
static UPSINFO *create_shmarea(int shmperm);
static UPSINFO *attach_shmarea(int shm_id, int shmperm);
static int attach_semaphore(UPSINFO *ups);
static int create_semaphore(UPSINFO *ups);
static int destroy_semaphore(UPSINFO *ups);
static int detach_shmarea(UPSINFO *ups);
static int destroy_shmarea(UPSINFO *ups);


static int sem_id = SEM_ID;        /* Semaphore ID */
static int shm_id = SHM_ID;        /* Shared memory ID */


UPSINFO *new_ups()
{
   UPSINFO *ups;

   /*
    * If we fail initializing IPC structures it is better destroy any
    * semi-initialized IPC.
    */
   ups = create_shmarea(0644);
   if (!ups)
      return NULL;

   if (create_semaphore(ups) == FAILURE) {
      destroy_shmarea(ups);
      return NULL;
   }

   return ups;
}

/* Another process can attach to our ups structure */
UPSINFO *attach_ups(UPSINFO *ups, int shmperm)
{
   /*
    * If we fail attaching IPC structures, the problem is local so return
    * failure but don't touch already initialized IPC structures.
    */
   ups = attach_shmarea(shm_id, shmperm);
   if (!ups)
      return NULL;

   Dmsg4(100, "ups=%x shm_id=%x sem_id=%x idshmUPS=%d\n",
      ups, ups->shm_id, ups->sem_id, ups->idshmUPS);

   if (attach_semaphore(ups) == FAILURE)
      return NULL;

   return ups;
}

int detach_ups(UPSINFO *ups)
{
   if (detach_shmarea(ups) == FAILURE)
      return FAILURE;

   return SUCCESS;
}

int destroy_ups(UPSINFO *ups)
{
   if (destroy_shmarea(ups) == FAILURE)
      return FAILURE;

   if (destroy_semaphore(ups) == FAILURE)
      return FAILURE;

   return SUCCESS;
}

int _read_lock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_lock at %s:%d\n", file, line);

   /* Acquire writer lock to be sure no other write is in progress. */
   ups->semUPS[0].sem_num = WRITE_LCK;
   ups->semUPS[0].sem_op = -1;
   ups->semUPS[0].sem_flg = SEM_UNDO;

   /* Increment reader counter. */
   ups->semUPS[1].sem_num = READ_CNT;
   ups->semUPS[1].sem_op = +1;
   ups->semUPS[1].sem_flg = SEM_UNDO;

   /* Release writer lock: we don't need it any more. */
   ups->semUPS[2].sem_num = WRITE_LCK;
   ups->semUPS[2].sem_op = +1;
   ups->semUPS[2].sem_flg = SEM_UNDO;

   /* Note that the above three operations are atomic. */
   if (semop(ups->idsemUPS, SEMBUF, 3) == -1) {
      log_event(ups, LOG_WARNING, _("read_lock: cannot increment read cnt. ERR=%s"),
         strerror(errno));
      return FAILURE;
   }

   return SUCCESS;
}

int _read_unlock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_unlock at %s:%d\n", file, line);

   /* Reader counter must be decremented */
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

int _write_lock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_lock at %s:%d\n", file, line);

   /* Acquire writer lock to be sure no other write is in progress. */
   ups->semUPS[0].sem_num = WRITE_LCK;
   ups->semUPS[0].sem_op = -1;
   ups->semUPS[0].sem_flg = SEM_UNDO;

   if (semop(ups->idsemUPS, SEMBUF, 1) == -1) {
      log_event(ups, LOG_WARNING, _("write_lock: cannot acquire write lock. ERR=%s"),
         strerror(errno));
      return FAILURE;
   }

   /* Reader counter must be zero. */
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
    */

   return SUCCESS;
}

int _write_unlock(char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_unlock at %s:%d\n", file, line);

   /* Release writer lock. */
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

static int create_semaphore(UPSINFO *ups)
{
   union semun apcsun;
   unsigned short int vals[NUM_SEM];
   int i;
   int found = FALSE;

   vals[READ_CNT] = 0;
   vals[WRITE_LCK] = 1;

   /* Get the semaphores. */
   for (i = 0; i < MAX_TRIES; i++) {
      if ((ups->idsemUPS = semget(sem_id, NUM_SEM, IPC_CREAT | 0644)) == -1) {
         log_event(ups, LOG_WARNING,
            _("Cannot create semaphore for key=%x, retrying... ERR=%s\n"), sem_id,
            strerror(errno));
         sem_id++;                 /* try another one */
         continue;
      }
      found = TRUE;
      break;
   }

   if (!found) {
      log_event(ups, LOG_ERR, _("Unable to create semaphore. ERR=%s\n"),
         sem_id, strerror(errno));
      return FAILURE;
   }

   /* Now set them up. */
   apcsun.array = vals;
   if (semctl(ups->idsemUPS, 0, SETALL, apcsun) == -1) {
      log_event(ups, LOG_ERR, _("create_semaphore: cannot set up sem. ERR=%s\n"),
         strerror(errno));
      return FAILURE;
   }

   ups->sem_id = sem_id;
   Dmsg2(100, "ups=%x sem_id=%x\n", ups, ups->sem_id);

   return SUCCESS;
}

static int attach_semaphore(UPSINFO *ups)
{
   if (semget(ups->sem_id, NUM_SEM, 0) == -1) {
      log_event(ups, LOG_ERR, _("attach_semaphore: cannot attach sem. ERR=%s\n"),
         strerror(errno));
      return FAILURE;
   }

   return SUCCESS;
}

static int destroy_semaphore(UPSINFO *ups)
{
   union semun arg;

   if (ups->idsemUPS == -1)
      return FAILURE;              /* already destroyed */

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
 * We create an shm for the size of UPSINFO. This area will be used by all the
 * threads to get the data from serial polling thread.
 * To make sure to have a valid UPSINFO struct we use a semaphore that will be
 * asserted only when the serial polling thread is writing to the shm area so
 * that no other thread may read from that area until the write is done.
 * Multiple read lock is permitted.
 */
static UPSINFO *create_shmarea(int shmperm)
{
   int i;
   int found = FALSE;
   UPSINFO *ups = NULL;
   int idshmUPS = -1;

   for (i = 0; i < MAX_TRIES; i++) {
      if ((idshmUPS = shmget(shm_id, sizeof(UPSINFO), IPC_CREAT | shmperm)) == -1) {
         log_event(ups, LOG_WARNING,
            _("Cannot create shared memory area for key=%x, retrying... ERR=%s\n"),
            shm_id, strerror(errno));
         Dmsg2(99,
            _("Cannot create shared memory area for key=%x, retrying... ERR=%s\n"),
            shm_id, strerror(errno));
         shm_id++;                 /* try another id */
         sem_id++;                 /* increment semaphore id too */
         continue;
      }

      found = TRUE;
      break;
   }

   if (!found) {
      log_event(ups, LOG_WARNING,
         _("Unable to create shared memory area. ERR=%s\n"), strerror(errno));
      return NULL;
   }

   if ((ups = (UPSINFO *) shmat(idshmUPS, 0, 0)) == (UPSINFO *) - 1) {
      log_event(ups, LOG_ERR, _("create_shmarea: cannot attach shm area. ERR=%s\n"),
         strerror(errno));
      Dmsg1(99, _("create_shmarea: cannot attach shm area. ERR=%s\n"),
         strerror(errno));
      destroy_shmarea(ups);
      return NULL;
   }

   memset(ups, 0, sizeof(UPSINFO));
   ups->shm_id = shm_id;
   ups->sem_id = sem_id;
   ups->idshmUPS = idshmUPS;
   ups->idsemUPS = -1;
   Dmsg4(100, "ups=%x shm_id=%x sem_id=%x idshmUPS=%d\n",
      ups, ups->shm_id, ups->sem_id, ups->idshmUPS);

   return ups;
}

/*
 * This routine is used by any other program or thread to attach to
 * the shared memory.
 */
static UPSINFO *attach_shmarea(int shm_id, int shmperm)
{
   int idshmUPS;
   UPSINFO *ups = NULL;

   if ((idshmUPS = shmget(shm_id, sizeof(UPSINFO), 0)) == -1) {
      log_event(ups, LOG_ERR, _("attach_shmarea: cannot get shm area. ERR=%s\n"),
         strerror(errno));
      Dmsg2(99, _("attach_shmarea: cannot get shm area id=%x. ERR=%s\n"),
         shm_id, strerror(errno));

      return NULL;
   }

   ups = (UPSINFO *)shmat(idshmUPS, 0, shmperm);
   if (ups == (UPSINFO *)-1) {
      log_event(ups, LOG_ERR, _("attach_shmarea: cannot attach shm area. ERR=%s\n"),
         strerror(errno));

      Dmsg1(99, _("attach_shmarea: cannot attach shm area. ERR=%s\n"),
         strerror(errno));

      return NULL;
   }

   /*
    * Make sure that our layout of the shared memory is the
    * same as that to which we attached.
    */
   if (strcmp(ups->id, UPSINFO_ID) != 0
      || ups->version != UPSINFO_VERSION || ups->size != sizeof(UPSINFO)) {
      Error_abort0(_("attach_shmarea: shared memory version mismatch "
            "(or UPS not yet ready to report)\n"));
   }

   return ups;
}

static int detach_shmarea(UPSINFO *ups)
{
   if (shmdt((char *)ups) != 0)
      return FAILURE;

   return SUCCESS;
}

static int destroy_shmarea(UPSINFO *ups)
{
   if (ups->idshmUPS == -1)
      return FAILURE;              /* already destroyed */

   if (shmctl(ups->idshmUPS, IPC_RMID, NULL) == -1) {
      log_event(ups, LOG_ERR,
         _("destroy_shmarea: cannot destroy shm area. ERR=%s\n"),
         strerror(errno));
      return FAILURE;
   }

   ups->idshmUPS = -1;
   return SUCCESS;
}

#endif
