/*
 *  apcreports.c --
 *
 *  apcupsd.c	 -- Simple Daemon to catch power failure signals from a
 *		    BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		 -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick
 *			  <hedrick@astro.dyer.vanderbilt.edu>
 *  All rights reserved.
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "apc.h"

static char largebuf[4096];
static int  stat_recs;
static int  logstats = 0;
static time_t last_time_status;
static time_t last_time_logging;
static FILE *statusfile = NULL;  /* initial value set to NULL for terminate */


/*********************************************************************/
void clear_files(void)
{
    if (statusfile != NULL) {
	fflush(statusfile);
	fclose(statusfile);
    }
}

static void log_status_open(UPSINFO *ups)
{
    largebuf[0] = 0;
    stat_recs = 0;
    rewind(statusfile);
    logstats = ups->logstats;
}

#define STAT_REV 1
static int log_status_close(UPSINFO *ups, int fd)
{
    int i;
    char buf[MAXSTRING];
    char *sptr, *eptr;

    i = strlen(largebuf);
    if (i > (int)sizeof(largebuf)-1) {
        log_event(ups, LOG_ERR, _("Status buffer overflow %d bytes\n"), i-sizeof(largebuf));
	return -1;
    }
    asnprintf(buf, sizeof(buf), "APC      : %03d,%03d,%04d\n", STAT_REV, stat_recs, i);
    fputs(buf, statusfile);
    fputs(largebuf, statusfile);
    fflush(statusfile);
    
    /*
     * Write out the status log to syslog() one record at a 
     * time.
     */
    if (logstats) {
	buf[strlen(buf)-1] = 0;
	log_event(ups, LOG_NOTICE, buf);
	sptr = eptr = largebuf;
	for ( ; i > 0; i--) {
            if (*eptr == '\n') {
	       *eptr++ = 0;
	       log_event(ups, LOG_NOTICE, sptr);
	       sptr = eptr;
	    } else {
	       eptr++;
	    }
	}
    }		     
    return 0;
}



/********************************************************************* 
 * log one line of the status file
 * also send it to system log
 *
 */
static void log_status_write(UPSINFO *ups, char *fmt, ...)
{
    va_list ap;
    char buf[MAXSTRING];

    va_start(ap, fmt);

    avsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    strncat(largebuf, buf, sizeof(largebuf));
    largebuf[sizeof(largebuf)-1] = 0;
    stat_recs++;
}


/********************************************************************
 *
 * Log current UPS data
 *
 *  Format of output for SmartUPS
 *    vmin, vmax, vout, vbat, freq, load%,temp,amb-temp,humid, vline   
 *    223.6,230.1,227.5,27.74,50.00,020.8,034.6,,,229.8
 *  currently ambient temp and humidity are not included
 */ 

static void log_data(UPSINFO *ups)
{
    char *ptr;
    char msg[MAXSTRING];
    static int toggle = 0;

    if (ups->datatime == 0)
	return;

    switch (ups->mode.type) {
    case BK:
    case SHAREBASIC:
    case NETUPS:
	if (!UPS_ISSET(UPS_ONBATT)) {
	    if (!UPS_ISSET(UPS_REPLACEBATT)) {
               ptr = "OK";
	    } else {
               ptr = "REPLACE";
	    }
            log_event(ups, LOG_INFO,"LINEFAIL:OK BATTSTAT:%s", ptr);
	} else {
	    if (!UPS_ISSET(UPS_BATTLOW)) { 
                ptr = "RUNNING";
	    } else {
                ptr = "FAILING";
	    }
            log_event(ups, LOG_INFO, "LINEFAIL:DOWN BATTSTAT:%s", ptr);
	}
	break;
    case BKPRO:
    case VS:
	if (!UPS_ISSET(UPS_ONBATT)) {
	    if (UPS_ISSET(UPS_SMARTBOOST)) 
                ptr = "LOW";
	    else if (UPS_ISSET(UPS_SMARTTRIM)) 
                ptr = "HIGH";
	    else 
                ptr = "OK";
            log_event(ups, LOG_INFO, "LINEFAIL:OK BATTSTAT:OK MAINS:%s", ptr);
	} else {
	    if (!UPS_ISSET(UPS_BATTLOW)) 
                ptr = "RUNNING";
	    else 
                ptr = "FAILING";
            log_event(ups, LOG_INFO, "LINEFAIL:DOWN BATTSTAT:%s", ptr);
	}

	switch ((*ups).G[0]) {
        case 'O':
            ptr = "POWER UP";
	    break;
        case 'S':
            ptr = "SELF TEST";
	    break;
        case 'L':
            ptr = "LINE VOLTAGE DECREASE";
	    break;
        case 'H':
            ptr = "LINE VOLTAGE INCREASE";
	    break;
        case 'T':
            ptr = "POWER FAILURE";
	    break;
        case 'R':
            ptr = "R-EVENT";
	    break;
	default :
            asnprintf(msg, sizeof(msg), "UNKNOWN EVENT %c %c", (*ups).G[0], (*ups).G[1]);
	    ptr = msg;
	    break;
	}
        log_event(ups, LOG_INFO, "LASTEVENT: %s", ptr);
	break;
    case NBKPRO:
    case SMART:
    case SHARESMART:
    case MATRIX:
    case USB_UPS:
    case TEST_UPS:
    case DUMB_UPS:
    case NETWORK_UPS:
    case SNMP_UPS:
    case APCSMART_UPS:
	toggle = !toggle;	      /* flip bit */
	log_event(ups, LOG_INFO, 
                "%05.1f,%05.1f,%05.1f,%05.2f,%05.2f,%04.1f,%04.1f,%05.1f,%05.1f,%05.1f,%05.1f,%d",
		 ups->LineMin,
		 ups->LineMax,
		 ups->OutputVoltage,
		 ups->BattVoltage,
		 ups->LineFreq,
		 ups->UPSLoad,
		 ups->UPSTemp,
		 ups->ambtemp,
		 ups->humidity,
		 ups->LineVoltage,
		 ups->BattChg,
		 toggle);
	break;
    default:
	break;
    }
    return;
}


void do_reports(UPSINFO *ups)
{
    static int first_time = TRUE;
    time_t now = time(NULL);

    if (first_time) {
	first_time = FALSE;
	/*
	 * Set up logging and status timers.
	 */
	last_time_logging = 0;
	last_time_status  = 0;

	if (ups->stattime != 0) {
            if ((statusfile = fopen(ups->statfile,"w")) == NULL)
                log_event(ups, LOG_ERR, "Cannot open STATUS file %s: %s\n",
			  ups->statfile, strerror(errno));
	}  else {
	    unlink(ups->statfile);
	}
    }

    /* Check if it is time to log DATA record */
    if ((ups->datatime > 0) && (now - last_time_logging) > ups->datatime) {
	time(&last_time_logging);
	log_data(ups);
    }

    /* Check if it is time to write STATUS file */
    if ((statusfile != NULL) && (now - last_time_status) > ups->stattime) {
	time(&last_time_status);
	output_status(ups, 0, log_status_open, log_status_write, 
	   log_status_close);
    }

    /* Trim the EVENTS file */
    trim_eventfile(ups);
}
