/*
 * apcupsd.c
 *
 * APC UPS monitoring daemon.
 */

/*
 * Copyright (C) 1999-2005 Kern Sibbald
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#include "apc.h"

UPSINFO *core_ups = NULL;

static void daemon_start(void);

int pidcreated = 0;
extern int kill_on_powerfail;
extern FILE *trace_fd;
extern char *pidfile;

/*
 * The terminate function and trapping signals allows apcupsd
 * to exit and cleanly close logfiles, and reset the tty back
 * to its original settings. You may want to add this
 * to all configurations.  Of course, the file descriptors
 * must be global for this to work, I also set them to
 * NULL initially to allow terminate to determine whether
 * it should close them.
 */
void apcupsd_terminate(int sig)
{
   UPSINFO *ups = core_ups;

   if (sig != 0)
      log_event(ups, LOG_WARNING, "apcupsd exiting, signal %u", sig);

   clean_threads();
   clear_files();
   if (ups->driver)
      device_close(ups);
   delete_lockfile(ups);
   if (pidcreated)
      unlink(pidfile);
   log_event(ups, LOG_NOTICE, "apcupsd shutdown succeeded");
   destroy_ups(ups);
   closelog();
   _exit(0);
}

void apcupsd_error_cleanup(UPSINFO *ups)
{
   if (ups->driver)
      device_close(ups);
   delete_lockfile(ups);
   if (pidcreated)
      unlink(pidfile);
   clean_threads();
   log_event(ups, LOG_ERR, "apcupsd error shutdown completed");
   destroy_ups(ups);
   closelog();
   exit(1);
}

/*
 * Subroutine error_out prints FATAL ERROR with file,
 * line number, and the error message then cleans up 
 * and exits. It is normally called from the Error_abort
 * define, which inserts the file and line number.
 */
void apcupsd_error_out(const char *file, int line, const char *fmt, va_list arg_ptr)
{
   char buf[256];
   int i;

   asnprintf(buf, sizeof(buf),
      "apcupsd FATAL ERROR in %s at line %d\n", file, line);

   i = strlen(buf);
   avsnprintf((char *)&buf[i], sizeof(buf) - i, (char *)fmt, arg_ptr);

   fprintf(stderr, "%s", buf);
   log_event(core_ups, LOG_ERR, "%s", buf);
   apcupsd_error_cleanup(core_ups);     /* finish the work */
}

/*
 * ApcupsdMain is called from win32/winmain.cpp
 * we need to eliminate "main" as an entry point,
 * otherwise, it interferes with the Windows
 * startup.
 */
#ifdef HAVE_MINGW
# define main ApcupsdMain
#endif

/* Main program */
int main(int argc, char *argv[])
{
   UPSINFO *ups;
   int tmp_fd, i;

   /* Set specific error_* handlers. */
   error_out = apcupsd_error_out;

   /*
    * Default config file. If we set a config file in startup switches, it
    * will be re-filled by parse_options()
    */
   cfgfile = APCCONF;

   /*
    * Create fake stdout in order to circumvent problems
    * which occur if apcupsd doesn't have a valid stdout
    * Fix by Alexander Schremmer <alex at alexanderweb dot de>
    */
   tmp_fd = open("/dev/null", O_RDONLY);
   if (tmp_fd > 2) {
      close(tmp_fd);
   } else if (tmp_fd >= 0) {
      for (i = 1; tmp_fd + i <= 2; i++)
         dup2(tmp_fd, tmp_fd + i);
   }

   /*
    * If there's not one in libc, then we have to use our own version
    * which requires initialization.
    */
   strlcpy(argvalue, argv[0], sizeof(argvalue));

   ups = new_ups();                /* get new ups */
   if (!ups)
      Error_abort("%s: init_ipc failed.\n", argv[0]);

   init_ups_struct(ups);
   core_ups = ups;                 /* this is our core ups structure */

   /* parse_options is self messaging on errors, so we need only to exit() */
   if (parse_options(argc, argv))
      exit(1);

   Dmsg(10, "Options parsed.\n");

   if (show_version) {
      printf("apcupsd " APCUPSD_RELEASE " (" ADATE ") " APCUPSD_HOST "\n");
      exit(0);
   }

   /*
    * We open the log file early to be able to write to it
    * it at any time. However, please note that if the user
    * specifies a different facility in the config file
    * the log will be closed and reopened (see match_facility())
    * in apcconfig.c.  Any changes here should also be made
    * to the code in match_facility().
    */
   openlog("apcupsd", LOG_CONS | LOG_PID, ups->sysfac);

#ifndef DEBUG
   if ((getuid() != 0) && (geteuid() != 0))
      Error_abort("Needs super user privileges to run.\n");
#endif

   check_for_config(ups, cfgfile);
   Dmsg(10, "Config file %s processed.\n", cfgfile);

   /*
    * Disallow --kill-on-powerfail in conjunction with simple signaling
    * UPSes. Such UPSes have no shutdown grace period so using --kill-on-
    * powerfail would guarantee an unclean shutdown.
    */
   if (kill_on_powerfail && ups->mode.type == DUMB_UPS) {
      kill_on_powerfail = 0;
      log_event(ups, LOG_WARNING,
         "Ignoring --kill-on-powerfail since it is unsafe "
            "on Simple Signaling UPSes");
   }

   /* Attach the correct driver. */
   attach_driver(ups);
   if (ups->driver == NULL)
      Error_abort("Apcupsd cannot continue without a valid driver.\n");

   ups->start_time = time(NULL);

   if (!hibernate_ups && !shutdown_ups && go_background) {
      daemon_start();

      /* Reopen log file, closed during becoming daemon */
      openlog("apcupsd", LOG_CONS | LOG_PID, ups->sysfac);
   }

   init_signals(apcupsd_terminate);

   /* Create temp events file if we are not doing a hibernate or shutdown */
   if (!hibernate_ups && !shutdown_ups && ups->eventfile[0] != 0) {
      ups->event_fd = open(ups->eventfile, O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
      if (ups->event_fd < 0) {
         log_event(ups, LOG_WARNING, "Could not open events file %s: %s\n",
            ups->eventfile, strerror(errno));
      }
   }

   if (create_lockfile(ups) == LCKERROR) {
      Error_abort("Unable to create UPS lock file.\n"
                   "  If apcupsd or apctest is already running,\n"
                   "  please stop it and run this program again.\n");
   }

   make_pid_file();
   pidcreated = 1;

   if (hibernate_ups || shutdown_ups)
   {
      // If we're hibernating or shutting down the UPS, setup is a one-shot.
      // If it fails, we're toast; no retries
      if (!setup_device(ups))
         Error_abort("Unable to open UPS device for hibernate or shutdown");

      if (hibernate_ups)
         initiate_hibernate(ups);
      else
         initiate_shutdown(ups);

      apcupsd_terminate(0);
   }

   /*
    * From now ... we must _only_ start up threads!
    * No more unchecked writes to UPSINFO because the threads must rely
    * on write locks and up to date data in the shared structure.
    */

   /* Network status information server */
   if (ups->netstats) {
      start_thread(ups, do_server, "apcnis", argv[0]);
      Dmsg(10, "NIS thread started.\n");
   }

   log_event(ups, LOG_NOTICE,
      "apcupsd " APCUPSD_RELEASE " (" ADATE ") " APCUPSD_HOST " startup succeeded");

   /* main processing loop */
   do_device(ups);

   apcupsd_terminate(0);
   return 0;                       /* to keep compiler happy */
}

extern int debug_level;

/*
 * Initialize a daemon process completely detaching us from
 * any terminal processes.
 */
static void daemon_start(void)
{
#if !defined(HAVE_MINGW)
   int i, fd;
   pid_t cpid;
   mode_t oldmask;

#ifndef HAVE_QNX_OS
   if ((cpid = fork()) < 0)
      Error_abort("Cannot fork to become daemon\n");
   else if (cpid > 0)
      exit(0);                     /* parent exits */

   /* Child continues */
   setsid();                       /* become session leader */
#else
   if (procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NOCHDIR
                                    | PROCMGR_DAEMON_NOCLOSE
                                    | PROCMGR_DAEMON_NODEVNULL
                                    | PROCMGR_DAEMON_KEEPUMASK) == -1)
   {
      Error_abort("Couldn't become daemon\n");
   }
#endif   /* HAVE_QNX_OS */

   /* Call closelog() to close syslog file descriptor */
   closelog();

   /*
    * In the PRODUCTION system, we close ALL file descriptors unless
    * debugging is on then we leave stdin, stdout, and stderr open.
    * We also take care to leave the trace fd open if tracing is on.
    * Furthermore, if tracing we redirect stdout and stderr to the
    * trace log.
    */
   for (i=0; i<sysconf(_SC_OPEN_MAX); i++) {
      if (debug_level && i == STDIN_FILENO)
         continue;
      if (trace_fd && i == fileno(trace_fd))
         continue;
      if (debug_level && (i == STDOUT_FILENO || i == STDERR_FILENO)) {
         if (trace_fd)
            dup2(fileno(trace_fd), i);
         continue;
      }
      close(i);
   }

   /* move to root directory */
   chdir("/");

   /* 
    * Avoid creating files 0666 but don't override a
    * more restrictive umask set by the caller.
    */
   oldmask = umask(022);
   oldmask |= 022;
   umask(oldmask);

   /*
    * Make sure we have fd's 0, 1, 2 open
    * If we don't do this one of our sockets may open
    * there and if we then use stdout, it could
    * send total garbage to our socket.
    */
   fd = open("/dev/null", O_RDONLY);
   if (fd > 2) {
      close(fd);
   } else if (fd >= 0) {
      for (i = 1; fd + i <= 2; i++)
         dup2(fd, fd + i);
   }
#endif   /* HAVE_MINGW */
}
