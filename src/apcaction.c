/*
 *  apcaction.c -- Actions taken when something is happen to UPS.
 *
 *  apcupsd.c	     -- Simple Daemon to catch power failure signals from a
 *		     BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		  -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  All rights reserved.
 *
 */

/*
 *			 GNU GENERAL PUBLIC LICENSE
 *			    Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			       675 Mass Ave, Cambridge, MA 02139, USA
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

#include "apc.h"

extern int kill_on_powerfail;
static void do_shutdown(UPSINFO *ups, int cmdtype);

/*
 * These are the commands understood by the apccontrol shell script.
 * You _must_ keep the the commands[] array in sync with the defines in
 * include/apc_defines.h
 */

UPSCOMMANDS cmd[] = {
    {"powerout",      0},             /* CMDPOWEROUT */
    {"onbattery",     0},             /* CMDONBATTERY */
    {"failing",       0},             /* CMDFAILING */
    {"timeout",       0},             /* CMDTIMEOUT */
    {"loadlimit",     0},             /* CMDLOADLIMIT */
    {"runlimit",      0},             /* CMDRUNLIMIT */
    {"doreboot",      0},             /* CMDDOREBOOT */
    {"doshutdown",    0},             /* CMDDOSHUTDOWN */
    {"mainsback",     0},             /* CMDMAINSBACK */
    {"annoyme",       0},             /* CMDANNOYME */
    {"emergency",     0},             /* CMDEMERGENCY */
    {"changeme",      0},             /* CMDCHANGEME */
    {"remotedown",    0},             /* CMDREMOTEDOWN */
    {"commfailure",   0},             /* CMDCOMMFAILURE */
    {"commok",        0},             /* CMDCOMMOK */
    {"startselftest", 0},             /* CMDSTARTSELFTEST */
    {"endselftest",   0},             /* CMDENDSELFTEST */
    {"mastertimeout", 0},             /* CMDMASTERTIMEOUT */
    {"masterconnect", 0}              /* CMDMASTERCONN */
};

/*
 * These messages must be kept in sync with the above array
 * and the defines in include/apc_defines.h 
 */
UPSCMDMSG cmd_msg[] = {
    {LOG_CRIT,    N_("Power failure.")},
    {LOG_CRIT,    N_("Running on UPS batteries.")},
    {LOG_ALERT,   N_("Battery power exhausted.")},
    {LOG_ALERT,   N_("Reached run time limit on batteries.")},
    {LOG_ALERT,   N_("Battery charge below low limit.")},
    {LOG_ALERT,   N_("Reached remaining time percentage limit on batteries.")},
    {LOG_ALERT,   N_("Failed to kill the power! Attempting a REBOOT!")}, 
    {LOG_ALERT,   N_("Initiating system shutdown!")},
    {LOG_ALERT,   N_("Power is back. UPS running on mains.")},
    {LOG_ALERT,   N_("Users requested to logoff.")},
    {LOG_ALERT,   N_("Battery failure. Emergency.")},
    {LOG_CRIT,    N_("UPS battery must be replaced.")},
    {LOG_CRIT,    N_("Remote shutdown requested")},
    {LOG_WARNING, N_("Communications with UPS lost.")},
    {LOG_WARNING, N_("Communications with UPS restored.")},
    {LOG_ALERT,   N_("UPS Self Test switch to battery.")},
    {LOG_ALERT,   N_("UPS Self Test completed.")},
    {LOG_CRIT,    N_("Master not responding.")},
    {LOG_WARNING, N_("Connect from master.")}
};

void generate_event(UPSINFO *ups, int event)
{
    /* Log message and execute script for this event */
    log_event(ups, cmd_msg[event].level, _(cmd_msg[event].msg));
    Dmsg2(80, "calling execute_cmd %s event=%d\n", cmd[event], event);
    execute_command(ups, cmd[event]);

    /*
     * Additional possible actions. For certain, we now do a
     * shutdown   
     */
    switch (event) {
    /*
     * For the following, in addition to the basic,
     * message logged and executed above, we do a 
     * system shutdown.
     */
    case CMDFAILING:
    case CMDTIMEOUT:
    case CMDRUNLIMIT:
    case CMDLOADLIMIT:
    case CMDEMERGENCY:
    case CMDREMOTEDOWN:
       log_event(ups, cmd_msg[CMDDOSHUTDOWN].level, _(cmd_msg[CMDDOSHUTDOWN].msg));
       do_shutdown(ups, CMDDOSHUTDOWN);
       break;

    case CMDDOREBOOT:
       /* This should be deprecated. */
       do_shutdown(ups, event);
       break;

    /* For the following, everything is already done. */
    case CMDSTARTSELFTEST:
    case CMDENDSELFTEST:
    case CMDCOMMFAILURE:
    case CMDCOMMOK:
    case CMDCHANGEME:
    case CMDANNOYME:
    case CMDMAINSBACK:
    case CMDDOSHUTDOWN:               /* Already shutdown, don't recall */
    case CMDPOWEROUT:
    case CMDONBATTERY:
    case CMDMASTERTIMEOUT:
    case CMDMASTERCONN:
    default:
	break;

   }
}

/*********************************************************************/
void powerfail (int ok)
{
    /*	      If apcupsd terminates here, it will never get a chance to
     *	      report the event of returning mains-power.
     *	      I think apcupsd has no need to force terminate() by itself.
     *	      It will receive a SIGTERM from init, when system goes down.
     *  This signal is trapped and will trigger apcupsd's terminate()
     *	      function.
     *	      (Was already reported/fixed in my 2.7.1 diff,)
     *	      (so I do it again now) w.p.
     *
     *		   if (ok == 2) terminate(0);
     *
     *	      Closes procfile and logfile to preserve information.
     *
     *	       ok = 1  => power is back
     *	       ok = 2  => power failure
     *	       ok = 3  => remote shutdown
     */

    if (ok == 2) {
	clear_files();
	if (terminate_on_powerfail)
	/*
	 * This sends a SIGTERM signal to itself.
	 * The SIGTERM is bound to apcupsd_ or apctest_terminate(),
	 * depending on which program is running this code, so it will
	 * do in anyway the right thing.
	 */
	    sendsig_terminate();
    }

    /*
     *	      The network slaves apcupsd needs to terminate here for now.
     *	      This sloppy, but it works. If you are networked, then the
     *	      master must fall also. This is required so that the UPS
     *	      can reboot the slaves.
     */

    if (ok == 3)
	sendsig_terminate();
}
	
/********************************************************************* 
 * if called with zero, prevent users from logging in. 
 * if called with 1   , allow users to login.
 */
void logonfail(int ok)
{
    int lgnfd;

    unlink(NOLOGIN);
    if (ok == 0 && ((lgnfd = open(NOLOGIN, O_CREAT|O_WRONLY, 0644)) >= 0)) {
	write(lgnfd, POWERFAIL, strlen(POWERFAIL));
	close(lgnfd);
    }
}

static void prohibit_logins(UPSINFO *ups)
{
    if (ups->nologin_file)
	return; 		      /* already done */
    make_file(ups, NOLOGIN);
    ups->nologin_file = TRUE;
    logonfail(0);
    log_event(ups, LOG_ALERT, _("User logins prohibited"));
}

static void do_shutdown(UPSINFO *ups, int cmdtype)
{
    if (is_ups_set(UPS_SHUTDOWN)) {
	return; 		      /* already done */
    }
    ups->ShutDown = time(NULL);
    set_ups(UPS_SHUTDOWN);
    delete_lockfile(ups);
    set_ups(UPS_FASTPOLL);
    make_file(ups, PWRFAIL);
    prohibit_logins(ups);

    if (!is_ups_set(UPS_SLAVE)) {
	/*
	 * Note, try avoid using this option if at all possible
	 * as it will shutoff the UPS power, and you cannot
	 * be guaranteed that the shutdown command will have
	 * succeeded. This PROBABLY should be executed AFTER
	 * the shutdown command is given (the execute_command below).
	 */
	if (kill_on_powerfail) {
	    kill_power(ups);
	}
    } 

    /* Now execute the shutdown command */
    execute_command(ups, cmd[cmdtype]);

    /* On some systems we may stop on the previous
     * line if a SIGTERM signal is sent to us.	      
     */

    if (cmdtype == CMDREMOTEDOWN) 
	powerfail(3);
    else
	powerfail(2);
}

/*
 * These are the different "states" that the UPS can be
 * in.
 */
enum a_state {
    st_PowerFailure,
    st_SelfTest,
    st_OnBattery,
    st_MainsBack,
    st_OnMains 
};

/*
 * Figure out what "state" the UPS is in and
 * return it for use in do_action()
 */
static enum a_state get_state(UPSINFO *ups, time_t now)
{
    enum a_state state;

    if (is_ups_set(UPS_ONBATT)) {
	if (is_ups_set(UPS_PREV_ONBATT)) {  /* if already detected on battery */
	    if (ups->SelfTest) {       /* see if UPS is doing self test */
		state = st_SelfTest;   /*   yes */
	    } else {
		state = st_OnBattery;  /* No, this must be real power failure */
	    }
	} else {
	    state = st_PowerFailure;   /* Power failure just detected */
	}
    } else {
	if (is_ups_set(UPS_PREV_ONBATT)) {		    /* if we were on batteries */
	    state = st_MainsBack;     /* then we just got power back */
	} else {
	    state = st_OnMains;       /* Solid on mains, normal condition */
	}
    }
    return state;
}

/*********************************************************************/
void do_action(UPSINFO *ups)
{
    time_t now;
    static int requested_logoff = 0; /* asked user to logoff */
    static int first = 1;
    enum a_state state;

    if (write_lock(ups)) {
	/*
	 * If failed to acquire shm lock, return and try again later.
	 */
        log_event(ups, LOG_CRIT, _("Failed to acquire shm lock in do_action."));
	return;
    }

    time(&now); 		  /* get current time */
    if (first) {
	ups->last_time_nologon = ups->last_time_annoy = now;
	ups->last_time_on_line = now;
	clear_ups(UPS_PREV_ONBATT);
	clear_ups(UPS_PREV_BATTLOW);
	first = 0;
    }

    if (is_ups_set(UPS_REPLACEBATT)) {	 /* Replace battery */
	/* Complain every 9 hours, this causes the complaint to
	 * cycle around the clock and hopefully be more noticable
	 * without being too annoying.	      Also, ignore all change battery
	 * indications for the first 10 minutes of running time to
	 * prevent false alerts.
	 * Finally, issue the event 5 times, then clear the flag
	 * to silence false alarms. If the battery is really dead, the
	 * flag will be reset in apcsmart.c
	 *
	 * UPS_REPLACEBATT is a flag. To count use a static local counter.
	 * The counter is initialized only one time at startup.
	 *
	 * -RF
	 */
	if (now - ups->start_time < 60 * 10 || ups->ChangeBattCounter > 5) {
	    clear_ups(UPS_REPLACEBATT);
	    ups->ChangeBattCounter = 0;
	} else if (now - ups->last_time_changeme > 60 * 60 * 9) {
	    generate_event(ups, CMDCHANGEME);
	    ups->last_time_changeme = now;
	    ups->ChangeBattCounter++;
	}
    }

    /*
     *	      Must SHUTDOWN Remote System Calls
     */
    if (is_ups_set(UPS_SHUT_REMOTE)) {
	clear_ups(UPS_ONBATT_MSG);
	generate_event(ups, CMDREMOTEDOWN);
	return;
    }

    state = get_state(ups, now);
    switch (state) {
    case st_OnMains:
	/*
	 * If power is good, update the timers.
	 */
	ups->last_time_nologon = ups->last_time_annoy = now;
	ups->last_time_on_line = now;
	clear_ups(UPS_FASTPOLL);
	break;

    case st_PowerFailure:
       /*
	*  This is our first indication of a power problem
	*/
	set_ups(UPS_FASTPOLL);		       /* speed up polling */
	/* Check if selftest */
        Dmsg1(80, "Power failure detected. 0x%x\n", ups->Status);
	device_entry_point(ups, DEVICE_CMD_CHECK_SELFTEST, NULL);
	if (ups->SelfTest) {
	    generate_event(ups, CMDSTARTSELFTEST);
	} else {
	    generate_event(ups, CMDPOWEROUT);
	}
	ups->last_time_nologon = ups->last_time_annoy = now;
	ups->last_time_on_line = now;
	ups->last_onbatt_time = now;
	ups->num_xfers++;
	/*
	 * Enable DTR on dumb UPSes with CUSTOM_SIMPLE cable.
	 */
	device_entry_point(ups, DEVICE_CMD_DTR_ENABLE, NULL);
	break;

    case st_SelfTest:
       /* allow 12 seconds max for selftest */
       if (now - ups->SelfTest < 12 && !is_ups_set(UPS_BATTLOW))
	   break;
       /* Cancel self test, announce power failure */
       ups->SelfTest = 0;
       Dmsg1(80, "UPS Self Test cancelled, fall-thru to On Battery. 0x%x\n", ups->Status);
       /* FALL-THRU to st_OnBattery */
    case st_OnBattery:
	/*
	 *  Did the second test verify the power is failing?
	 */
	if (!is_ups_set(UPS_ONBATT_MSG)) {
	    set_ups(UPS_ONBATT_MSG);   /* it is confirmed, we are on batteries */
	    generate_event(ups, CMDONBATTERY);
	    ups->last_time_nologon = ups->last_time_annoy = now;
	    ups->last_time_on_line = now;
	    break;
	} 

	/* shutdown requested but still running */
	if (is_ups_set(UPS_SHUTDOWN)) {
	   if (ups->killdelay && now - ups->ShutDown >= ups->killdelay) {
	       if (!is_ups_set(UPS_SLAVE))
		   kill_power(ups);
	       ups->ShutDown = now;   /* wait a bit before doing again */
	       set_ups(UPS_SHUTDOWN);
	   }
	} else {		/* not shutdown yet */
	    /*
	     * Did BattLow bit go high? Then the battery power is failing.
	     * Normal Power down during Power Failure
	     */
	if (!is_ups_set(UPS_PREV_BATTLOW) && is_ups_set(UPS_BATTLOW)) {
	    clear_ups(UPS_ONBATT_MSG);
	    generate_event(ups, CMDFAILING);
	    break;
	}

	    /*
	     * Did MaxTimeOnBattery Expire?  (TIMEOUT in apcupsd.conf)
	     * Normal Power down during Power Failure
	     */
	    if ((ups->maxtime > 0) && 
		((now - ups->last_time_on_line) > ups->maxtime)) {
		set_ups(UPS_SHUT_BTIME);
		generate_event(ups, CMDTIMEOUT);
		break;
	    }
	    /*
	     *	      Did Battery Charge or Runtime go below percent cutoff?
	     *	      Normal Power down during Power Failure
	     */
	    if (ups->UPS_Cap[CI_BATTLEV] && ups->BattChg <= ups->percent) {
		set_ups(UPS_SHUT_LOAD);
		generate_event(ups, CMDLOADLIMIT);
		break;
	    } else if (ups->UPS_Cap[CI_RUNTIM] && ups->TimeLeft <= ups->runtime) {
		set_ups(UPS_SHUT_LTIME);
		generate_event(ups, CMDRUNLIMIT);
		break;
	    }

	    /*
	     * We are on batteries, the battery is low, and the power is not
	     * down ==> the battery is dead.  KES Sept 2000
	     *
	     * Then the battery has failed!!!
	     * Must do Emergency Shutdown NOW
	     *
	     * Or of the UPS says he is going to shutdown, do it NOW!
	     */
	if (is_ups_set(UPS_SHUTDOWNIMM) ||
		(is_ups_set(UPS_BATTLOW) && is_ups_set(UPS_ONLINE))) {
	    clear_ups(UPS_ONBATT_MSG);
	    set_ups(UPS_SHUT_EMERG);
	    generate_event(ups, CMDEMERGENCY);
	}

	    /*
	     *	      Announce to LogOff, with initial delay ....
	     */
	    if (((now - ups->last_time_on_line) > ups->annoydelay) &&
		((now - ups->last_time_annoy) > ups->annoy) &&
		  ups->nologin_file) {
		    if (!requested_logoff) {
			/* generate log message once */
			generate_event(ups, CMDANNOYME);
		    } else {
			/* but execute script every time */
			execute_command(ups, cmd[CMDANNOYME]);
		    }
		    time(&ups->last_time_annoy);
		    requested_logoff = TRUE;
	    }
	    /*
	     *	      Delay NoLogons....
	     */
	    if (!ups->nologin_file) {
		switch(ups->nologin.type) {
		case NEVER:
		    break;
		case TIMEOUT:
		    if ((now - ups->last_time_nologon) > ups->nologin_time) {
			prohibit_logins(ups);
		    }
		    break;
		case PERCENT:
		    if (ups->UPS_Cap[CI_BATTLEV] && ups->nologin_time >= ups->BattChg) {
			prohibit_logins(ups);
		    }
		    break;
		case MINUTES:
		    if (ups->UPS_Cap[CI_RUNTIM] && ups->nologin_time >= ups->TimeLeft) {
			prohibit_logins(ups);
		    }
		    break;
		case ALWAYS:
		default:
		    prohibit_logins(ups);
		    break;
		}
	    }
	}      
	break;

    case st_MainsBack:
	/*
	 *  The the power is back after a power failure or a self test	       
	 */
	clear_ups(UPS_ONBATT_MSG);
	if (is_ups_set(UPS_SHUTDOWN)) {
	    /*
	     * If we have a shutdown to cancel, do it now.
	     */
	    ups->ShutDown = 0;
	    clear_ups(UPS_SHUTDOWN);
	    powerfail(1);
	    unlink(PWRFAIL);
            log_event(ups, LOG_ALERT, _("Cancelling shutdown"));
	}

	if (ups->SelfTest) {
	    ups->LastSelfTest = ups->SelfTest;
	    ups->SelfTest = 0;
	    /*
	     * Get last selftest results, only for smart UPSes.
	     */
	    device_entry_point(ups, DEVICE_CMD_GET_SELFTEST_MSG, NULL);
            log_event(ups, LOG_ALERT, _("UPS Self Test completed: %s"),
		ups->selftestmsg);
	    execute_command(ups, cmd[CMDENDSELFTEST]);
	} else {
	    generate_event(ups, CMDMAINSBACK);
	}

	if (ups->nologin_file) {
            log_event(ups, LOG_ALERT, _("Allowing logins"));
	}
	logonfail(1);
	ups->nologin_file = FALSE;
	requested_logoff = FALSE;
	device_entry_point(ups, DEVICE_CMD_DTR_ST_DISABLE, NULL);
	ups->last_offbatt_time = now;
	/* Sanity check. Sometimes only first power problem trips    
	 * thus last_onbatt_time is not set when we get here */
	if (ups->last_onbatt_time <= 0)
	   ups->last_onbatt_time = ups->last_offbatt_time;
	ups->cum_time_on_batt += (ups->last_offbatt_time - ups->last_onbatt_time);
	break;

    default:
	break;
    }

    /* Do a non-blocking wait on any exec()ed children */
    if (ups->num_execed_children > 0) {
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	    ups->num_execed_children--;
	}
    }

    /*
     *	      Remember status
     */

    if (is_ups_set(UPS_ONBATT)) {
	set_ups(UPS_PREV_ONBATT);
    } else {
	clear_ups(UPS_PREV_ONBATT);
    }
    if (is_ups_set(UPS_BATTLOW)) {
	set_ups(UPS_PREV_BATTLOW);
    } else {
	clear_ups(UPS_PREV_BATTLOW);
    }

    write_unlock(ups);
}
