/*
 *  apcdevice.c -- generic device handling
 *
 *  apcupsd.c	     -- Simple Daemon to catch power failure signals from a
 *		     BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		  -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

/* Forward referenced function */
static int device_wait_time(UPSINFO *ups);

/*********************************************************************/
void setup_device(UPSINFO *ups)
{
    if (!UPS_ISSET(UPS_DEV_SETUP)) {
	/*
	 * Marko Sakari Asplund <Marko.Asplund@cern.ch>
	 *    prevents double init of UPS device 9/25/98
	 */
	UPS_SET(UPS_DEV_SETUP);
    } else {
        Error_abort0(_("Serial port already initialized.\n"));
    }

    device_open(ups);

    /*
     * next 5 lines by BSC
     *
     * If create_lockfile fails there's no need to delete_lockfile.
     * -RF
     */ 		    
    if (create_lockfile(ups) == LCKERROR) {
	device_close(ups);
        Error_abort0(_("Unable to create UPS lock file.\n"));
    }

    /*
     * No need to init_signals here as it was inited by main().
     */
    /* init_signals(terminate); */

    device_setup(ups);

    return;
}

/*********************************************************************/
void kill_power(UPSINFO *ups)
{
    FILE *pwdf;
    int killcount;

    if (ups->mode.type <= SHAREBASIC) {
	/* Make sure we are on battery !! */
	for (killcount=0; killcount<3; killcount++) {
	    device_read_volatile_data(ups);
	}
    }
    /*
     *	      ****FIXME*****  This big if is BROKEN in the
     *			      case that there is no real power failure.
     *
     * We really need to find out if we are on batteries
     * and if not, delete the PWRFAIL file.  Note, the code
     * above only tests UPS_ONBATT flag for dumb UPSes.
     */
    if ((((pwdf = fopen(PWRFAIL, "r" )) == NULL) &&
	    (ups->mode.type != BK)) ||
            (((pwdf = fopen(PWRFAIL, "r" )) == NULL) &&
	    (UPS_ISSET(UPS_ONBATT)) && (ups->mode.type == BK))) {
	/*						    
	 * At this point, we really should not be attempting
	 * a kill power since either the powerfail file is
	 * not defined, or we are not on batteries.
	 */
	if (pwdf) {
	   fclose(pwdf);	      /* close the file if openned */
	}
	/*
	 * Now complain 
	 */
	log_event(ups, LOG_WARNING,
                 _("Cannot find " PWRFAIL " file.\n Killpower requested in \
non-power fail condition or bug.\n Killpower request ignored at %s:%d\n"),
		 __FILE__, __LINE__);
	Error_abort2(
                _("Cannot find " PWRFAIL " file.\n Killpower requested in \
non-power fail condition or bug.\n Killpower request ignored at %s:%d\n"),
		 __FILE__, __LINE__);
    } else {
	/*
	 * We are on batteries, so do the kill_power 
	 */
	if ((ups->class.type == SHAREMASTER) ||
	    (ups->class.type == SHARENETMASTER)) {
	    log_event(ups, LOG_WARNING,
                          _("Waiting 30 seconds for slave(s) to shutdown."));
		sleep(30);
	} else if (ups->class.type == NETMASTER) {
	    log_event(ups, LOG_WARNING,
                          _("Waiting 30 seconds for slave(s) to shutdown"));
	    sleep(30);
	}
	if (pwdf) {
	    fclose(pwdf);	      /* close the powerfail file */
	}

        log_event(ups, LOG_WARNING,_("Attempting to kill the UPS power!"));

	if (ups->mode.type == BK) {
	    device_kill_power(ups);
	    sleep(10);
	    /*
	     * ***FIXME*** this really should not do a reboot here,
	     * but rather a halt or nothing -- KES
	     */
	    /*	generate_event(ups, CMDDOREBOOT); */
            log_event(ups, LOG_WARNING, _("Perform CPU-reset or power-off"));
	    return;
	} else if (ups->mode.type == SHAREBASIC) {
	    sleep(10);
            log_event(ups, LOG_WARNING, _("Waiting For ShareUPS Master to shutdown"));
	    sleep(60);
            log_event(ups, LOG_WARNING, _("Failed to have power killed by Master!"));
	    /*
	     * ***FIXME*** this really should not do a reboot here,
	     * but rather a halt or nothing -- KES
	     */
	    /* generate_event(ups, CMDDOREBOOT); */
            log_event(ups, LOG_WARNING,_("Perform CPU-reset or power-off"));
	    return;
	} else {		      /* it must be a SmartUPS */
	    device_kill_power(ups);
	}
    }
    return;
}

/*********************************************************************
 *
 * After the device is initialized, we come here
 *  to read all the information we can about the UPS.
 */
void prep_device(UPSINFO *ups)
{

    device_get_capabilities(ups);
    device_read_static_data(ups);

    /* If no UPS name found, use hostname, or "default" */
    if (ups->upsname[0] == 0) {       /* no name given */
	gethostname(ups->upsname, sizeof(ups->upsname)-1);
	if (ups->upsname[0] == 0) {   /* error */
            strcpy(ups->upsname, "default");
	}
    }
}

/********************************************************************* 
 *
 *  Called once every 5 seconds to read all UPS
 *  info
 */
int fillUPS(UPSINFO *ups)
{
    device_read_volatile_data(ups);

    return 0;
}

/********************************************************************  
 *
 * NOTE! This is the starting point for a separate process (thread).
 * 
 */
void do_device(UPSINFO *ups)
{
    init_thread_signals();   

    fillUPS(ups);		       /* get all data so apcaccess is happy */

    while (1) {
	ups->wait_time = device_wait_time(ups); /* compute appropriate wait time */

        Dmsg2(70, "Before device_check_state: %d (OB:%d).\n", ups->Status,
	    UPS_ISSET(UPS_ONBATT));
	/*
	 * Check the UPS to see if has changed state.
	 * This routine waits a reasonable time to prevent
	 * consuming too much CPU time.
	 */
	device_check_state(ups);

        Dmsg2(70, "Before do_action: %d (OB:%d).\n", ups->Status,
	    UPS_ISSET(UPS_ONBATT));

	do_action(ups);    /* take event actions */
	
        Dmsg2(70, "Before fillUPS: %d (OB:%d).\n", ups->Status,
	    UPS_ISSET(UPS_ONBATT));

	/* Get all info available from UPS by asking it questions */
	fillUPS(ups);

        Dmsg2(70, "Before do_reports: %d (OB:%d).\n", ups->Status,
	    UPS_ISSET(UPS_ONBATT));

	do_reports(ups);
    }
}

/* 
 * Each device handler when called at device_check_state() waits
 *  a specified time for the next event. In general, we want this
 *  to be as long as possible (i.e. about a minute) to prevent
 *  excessive "polling" because when device_check_state() returns,
 *  repoll the UPS (fillUPS). 
 * This routine attempts to determine a reasonable "sleep" time
 *  for the device.
 */
static int device_wait_time(UPSINFO *ups)
{
    int wait_time;

    if (UPS_ISSET(UPS_FASTPOLL)) {
	 wait_time = TIMER_FAST;
     } else {
	 wait_time = TIMER_SELECT;     /* normally 60 seconds */
     }
     /* Make sure we do data and stats when asked */
     if (ups->datatime && ups->datatime < wait_time) {
	 wait_time = ups->datatime;
     }
     if (ups->stattime && ups->stattime < wait_time) {
	 wait_time = ups->stattime;
     }
     /* Sanity check */
     if (wait_time < TIMER_FAST) {
	 wait_time = TIMER_FAST;
     }
     return wait_time;
}
