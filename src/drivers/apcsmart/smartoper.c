/*
 *  operations.c -- Functions for UPS operations
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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

#include "apc.h"
#include "apcsmart.h"

int apcsmart_ups_kill_power(UPSINFO *ups) {
    char response[32];
    int shutdown_delay = 0;
    int errflag = 0;
    char a;

    response[0] = '\0';

    a = ups->UPS_Cmd[CI_DSHUTD]; /* shutdown delay */
    write(ups->fd, &a, 1);
    getline(response, sizeof(response), ups);
    shutdown_delay = (int)atof(response);
    a = 'S';      /* ask for soft shutdown */
    write(ups->fd, &a, 1);
    /*
     * Check whether the UPS has acknowledged the power-off command.
     * This might not happen in rare cases, when mains-power returns
     * just after LINUX starts with the shutdown sequence.
     * interrupt the ouput-power. So LINUX will not come up without
     * operator intervention.  w.p.
     */
    sleep(5);
    getline(response, sizeof response, ups);
    if (strcmp(response, "OK") == 0) {
	if (shutdown_delay > 0)
	    log_event(ups, LOG_WARNING,
                    "UPS will power off after %d seconds ...\n",
		    shutdown_delay);
	else
	    log_event(ups, LOG_WARNING,
                    "UPS will power off after the configured delay  ...\n");
	log_event(ups, LOG_WARNING,
                _("Please power off your UPS before rebooting your computer ...\n"));
    
    } else { 
	/*
	 * Experiments show that the UPS needs
	 * delays between chars to accept
	 * this command
	 *
	 * w.p.
	 */
        a = '@';      /* Shutdown now */
	sleep(1);
	write(ups->fd, &a, 1);
	sleep(1);
        a = '0';
	write(ups->fd, &a, 1);
	sleep(1);
        a = '0';
	write(ups->fd, &a, 1);
	sleep(2);
	getline(response, sizeof(response), ups);
        if ((strcmp(response, "OK") == 0) || (strcmp(response,"*") == 0)) {
	    if (shutdown_delay > 0) {
		log_event(ups, LOG_WARNING,
                    "UPS will power off after %d seconds ...\n", shutdown_delay);
	    } else {
		log_event(ups, LOG_WARNING,
                    "UPS will power off after the configured delay  ...\n");
	    }
	    log_event(ups, LOG_WARNING,
                    _("Please power off your UPS before rebooting your computer ...\n"));
	} else {
	    errflag++;
	}
    }
    if (errflag) {
        a = '@';
	write(ups->fd, &a, 1);
	sleep(1);
        a = '0';
	write(ups->fd, &a, 1);
	sleep(1);
        a = '0';
	write(ups->fd, &a, 1);
	sleep(1);
	if ((ups->mode.type == BKPRO) || (ups->mode.type == VS)) {
            a = '1';
	    log_event(ups, LOG_WARNING,
                    _("BackUPS Pro and SmartUPS v/s sleep for 6 min"));
	} else {
            a = '0';
	}
	write(ups->fd, &a, 1);
	sleep(2);
	/* And yet another method !!! */
        a = 'K';
	write(ups->fd, &a, 1);
	sleep(2);
	write(ups->fd, &a, 1);
	getline(response, sizeof response, ups);
        if ((strcmp(response,"*") == 0) || (strcmp(response,"OK") == 0) ||
		(ups->mode.type >= BKPRO)) {
	    if (shutdown_delay > 0) {
		log_event(ups, LOG_WARNING,
                    "UPS will power off after %d seconds ...\n", shutdown_delay);
	    } else {
		log_event(ups, LOG_WARNING,
                        "UPS will power off after the configured delay  ...\n");
	    }
	    log_event(ups, LOG_WARNING,
                    _("Please power off your UPS before rebooting your computer ...\n"));
	    errflag = 0;
	} else {
	    errflag++;
	}
    }
    if (errflag) {
        log_event(ups, LOG_WARNING,_("Unexpected error!\n"));
        log_event(ups, LOG_WARNING,_("UPS in unstable state\n"));
	log_event(ups, LOG_WARNING,
                _("You MUST power off your UPS before rebooting!!!\n"));
	return 0;
    }
    return 1;
}

int apcsmart_ups_check_state(UPSINFO *ups) 
{
    return getline(NULL, 0, ups) == SUCCESS ? 1 : 0;
}
