/*
 *  apcexec.c -- fork/exec functions
 *
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
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
 *  Added child_pid array for clean shutdown of apcupsd's processes.
 *  -RF
 *
 */

#include "apc.h"

#ifdef HAVE_PTHREADS

static pthread_t thread_id[MAX_THREADS];
static char *thread_name[MAX_THREADS];
static int num_threads = 0;

/* 
 * Start a "real" POSIX thread
 */
int start_thread(UPSINFO *ups, void (*action)(UPSINFO *ups),
		char *proctitle, char *argv0)
{
    pthread_t tid;
    int status, t_index;

    set_thread_concurrency();
    if ((status=pthread_create(&tid, NULL, (void * (*)(void *))action, (void *)ups)) != 0) {
       log_event(ups, LOG_WARNING, _("Unable to start thread: ERR=%s\n"), 
	  strerror(status));
       return 0;
    }

    t_index = num_threads;
    if (num_threads < MAX_THREADS) {
	thread_id[num_threads] = tid;
	thread_name[num_threads] = proctitle;
	num_threads++;
    } else {
	log_event(ups, LOG_ERR, 
"Something is wrong, we have %d threads, max is %d\n", num_threads+1, MAX_THREADS);
    }

    return t_index;	 
}

/*
 * Cancel all running threads except ourselves
 */
void clean_threads(void)
{
   int i;
   pthread_t my_tid;

   my_tid = pthread_self();
   for (i=0; i < num_threads; i++) {
      if (!pthread_equal(my_tid, thread_id[i])) {
	 pthread_cancel(thread_id[i]);
	 pthread_detach(thread_id[i]);
      }
   }
}

#else

static int child_pid[MAX_THREADS] = { 0, 0, 0, 0, 0, 0, 0};

#if AVERSION==4
#define core_ups ((UPSINFO *)&gcfg)    /* gross kludge !!! */
#endif

static char *child_name[MAX_THREADS];

/*
 * The "main" process comes here after starting the serial port
 * work horse process. Here we simply wait for a child to die or
 * exit. If it is our work horse process (serial_pid), we
 * return so that the "main" process can kill all the other
 * children and exit.  For all other processes, we simply
 * report the fact that the child died and continue waiting.
 */
void wait_for_termination(int serial_pid)
{
    int status, pid, i;

    for (;;) {
	pid = wait(&status);
	for (i=0; i < MAX_THREADS; i++) {
	    if (child_pid[i] == 0)
		break;
	    else if (child_pid[i] == pid) {
		if (WIFSIGNALED(status))
		    log_event(core_ups, LOG_ERR, 
                        _("Unexpected termination of child %s by signal %d"),
			 child_name[i], WTERMSIG(status));
		else
		    log_event(core_ups, LOG_ERR, 
                        _("Unexpected termination of child %s. Status = %d"), 
			 child_name[i], WEXITSTATUS(status));
		/* If our serial port process died, return to exit */
		if (pid == serial_pid)
		    return;
	    }
	}
    }
}

int start_thread(UPSINFO *ups, void (*action)(UPSINFO *ups),
		char *proctitle, char *argv0)
{
    int pid;
    static int this_child = 0;

    switch(pid = fork()) {
    case 0:		  /* child process */
	setproctitle(proctitle);
	action(ups);
	break;
    case -1:		  /* error */
        log_event(ups, LOG_WARNING,_("start_thread: cannot fork. ERR=%s"),
	    strerror(errno));
	break;
    default:		  /* parent continues here */
	/*
	 * Keep process id of child
	 */
	child_pid[this_child++] = pid;
	child_name[this_child-1] = proctitle;
	child_pid[this_child] = 0;
	if (debug_level > 0)
	    log_event(ups, LOG_DEBUG,
                _("%s: start_thread(%s).\n"), argv0, proctitle);
	return pid;
    }
    return 0;
}

/**********************************************************************
 * the thread_terminate function and trapping signals allows threads
 * to cleanly exit.
 *********************************************************************/
void thread_terminate(int sig)
{
    UPSINFO *ups;

    /*
     * Before doing anything else restore the signal handlers,
     * stopping daemon work.
     *
     * -RF
     */

    restore_signals();

    for (ups = NULL; (ups = getNextUps(ups)) != NULL;) {
	/*
	 * Jump over the fake ups.
	 */
	if (!strcmp(ups->upsname, CORENAME))
	    continue;
	detach_ups(ups);
    }

    /*
     * Nothing to do. This is a thread: cleanup is managed by the father
     * process.
     */
    _exit(0);
}

/*
 * Here we make sure all the child processes of the main apcupsd are
 * terminated. We use a sequence similar to that of shutdown:
 *
 * 1. send TERM
 * 2. wait
 * 3. send KILL
 * 4. clean zombies
 *
 */
void clean_threads(void)
{
    int i;
    int status;

    for (i=0; i < MAX_THREADS; i++) {
	/*
	 * Stop if there are no more children
	 */
	if (child_pid[i] == 0)
	    break;

	/*
	 * Terminate the child.
	 */
	kill(child_pid[i], SIGTERM);
    }

    /*
     * Give the processes some time to settle down: not too much.
     * This sequence is similar to killing processes in shutdown.
     */
    sleep(1);

    /*
     * Again, but now SIGKILL.
     */
    for (i=0; i < MAX_THREADS; i++) {
	/*
	 * This is the same as of SIGTERM,
	 * but now the child must exit.
	 */
	if (child_pid[i] == 0)
	    break;
	kill(child_pid[i], SIGKILL);

	/*
	 * Make sure the process is dead.
	 * Call with WNOHANG because at this
	 * point if a process is still alive
         * means that it's hanging and is
	 * unkillable.
	 */
	waitpid(child_pid[i], &status, WNOHANG);
    }
}


#endif /* HAVE_PTHREADS */

int execute_command(UPSINFO *ups, UPSCOMMANDS cmd)
{
    char *argv[6];
    char connected[20], powered[20];

    if (cmd.pid && (kill(cmd.pid, 0) == 0)) {
	/*
	 * Command is already running. No point in running it two
	 * times.
		 */
	 return SUCCESS;
    }
    sprintf(connected, "%d", !is_ups_set(UPS_SLAVE));
    sprintf(powered,   "%d", (int)is_ups_set(UPS_PLUGGED));

/*
 * fork() and exec()
 */
    switch (cmd.pid = fork()) {
    case -1:		    /* error */
        log_event(ups, LOG_WARNING, _("execute: cannot fork(). ERR=%s"),
	   strerror(errno));
	return FAILURE;
    case 0:		    /* child */
	argv[0] = APCCONTROL;	  /* Shell script to execute. */
	argv[1] = cmd.command;	  /* Parameter to script. */
	argv[2] = ups->upsname;   /* UPS name */
	argv[3] = connected;
	argv[4] = powered;
	argv[5] = (char *)NULL;
	execv(APCCONTROL, argv);
	/* NOT REACHED */
        log_event(ups, LOG_WARNING,_("Cannot exec %s %s: %s"), 
		 APCCONTROL, cmd.command, strerror(errno));
	/*
         * Child must exit if fails exec'ing.
	 */
	exit(-1);
	break;

    default:		  /* parent */
	/*
	 * NOTE, we do a nonblocking waitpid) here to
	 * pick up any previous children. We also]
	 * increment a counter, and then in do_action()
	 * in apcaction.c, we wait again each pass until
	 * all the children are reaped.  This is for
	 * BSD systems where SIG_IGN does not prevent
	 * zombies.
	 */
	if (ups->num_execed_children < 0) {
	    ups->num_execed_children = 1;
	} else {
	    ups->num_execed_children++;
	}
	while (ups->num_execed_children > 0 && waitpid(-1, NULL, WNOHANG) > 0) {
	    ups->num_execed_children--;
	}
	break;
    }
    return SUCCESS;
}
