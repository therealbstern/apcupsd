/*
 *  apclog.c -- logging functions.
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

void log_event(UPSINFO *ups, int level, char *fmt, ...)
{
    va_list  arg_ptr;
    char msg[2*MAXSTRING];
    char datetime[MAXSTRING];
    int event_fd = ups->event_fd;
    char *p;

#if AVERSION==4 
    /*
     * If this is a generic message, not belonging to
     * any UPS, use the generic log file.
     *
     *	  VERSION 4.0 stupidities
     */
     if (ups == (UPSINFO *)&gcfg)
	 event_fd = gcfg.event_fd;
     else
	 event_fd = ups->event_fd; /* on version 3 this is true */
#else
    event_fd = ups->event_fd;
#endif

    va_start(arg_ptr, fmt);
    avsnprintf(msg, sizeof(msg), fmt, arg_ptr);
    va_end(arg_ptr);
     
    /*
     * Sanitize message to be sent to syslog to 
     * eliminate all %s which can be used as exploits.
     */
    for (p=msg; p=strchr(p, '%'); ) {
       *p = '\\';
    }

    syslog(level, msg); 	  /* log the event */

    /* Write out to our temp file. LOG_INFO is DATA logging, so
       do not write it to our temp events file. */
    if (event_fd >= 0 && level != LOG_INFO) {
	int lm;
	time_t nowtime;
	struct tm tm;

	time(&nowtime);
	localtime_r(&nowtime, &tm);
        strftime(datetime, 100, "%a %b %d %X %Z %Y  ", &tm);
	write(event_fd, datetime, strlen(datetime));
	lm = strlen(msg);
        if (msg[lm-1] != '\n') 
           msg[lm++] = '\n';
	write(event_fd, msg, lm);
    }
}

/*********************************************************************
 *
 *  subroutine prints a debug message if the level number
 *  is less than or equal the debug_level. File and line numbers
 *  are included for more detail if desired, but not currently
 *  printed.
 *  
 *  If the level is negative, the details of file and line number
 *  are not printed.
 */

int debug_level = 0;

#define FULL_LOCATION 1
void 
d_msg(char *file, int line, int level, char *fmt,...)
{
#ifdef DEBUG
    char      buf[4096];
    int       i;
    va_list   arg_ptr;
    int       details = TRUE;
    char      *my_name = "apcupsd";

    if (level < 0) {
       details = FALSE;
       level = -level;
    }

/*  printf("level=%d debug=%d fmt=%s\n", level, debug_level, fmt); */

    if (level <= debug_level) {
#ifdef FULL_LOCATION
       if (details) {
          sprintf(buf, "%s: %s:%d ", my_name, file, line);
	  i = strlen(buf);
       } else {
	  i = 0;
       }
#else
       i = 0;
#endif
       va_start(arg_ptr, fmt);
       avsnprintf(buf+i, sizeof(buf)-i, (char *)fmt, arg_ptr);
       va_end(arg_ptr);

       fprintf(stdout, buf);
    }
#endif
}
