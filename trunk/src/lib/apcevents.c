/*
 *  apcevents.c  -- Output EVENTS information
 *
 *   Copyright (C) 1999 Kern Sibbald
 *	 19 November 1999
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
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

#define NLE   50		      /* number of events to send and keep */

/*
 * If the ups->eventfile exceeds ups->eventfilemax kilobytes, trim it to
 * slightly less than that maximum, preserving lines at end of the file.
 *
 * Returns:
 *
 * -1 if any error occurred
 *  0 if file did not need to be trimmed
 *  1 if file was trimmed
 */
int trim_eventfile(UPSINFO *ups)
{
    int 		       nbytes, maxb, i, rwerror=0, status=-1;
    struct stat 	       statbuf;
    unsigned char      *buf;

    if (ups->eventfilemax==0 || ups->event_fd<0 || ups->eventfile[0]==0)
	return 0;

    maxb = ups->eventfilemax*1024;
    if (fstat(ups->event_fd, &statbuf) < 0)
       return -1;
    if (statbuf.st_size <= maxb)
       return 0;	       /* file is not yet too large - nothing to do */

    maxb = (maxb*80)/100;      /* file is too large - reduce to 80% of max */
    buf = (unsigned char *)malloc(maxb);
    if ( !buf )
	return -1;

    if (write_lock(ups)) {
	log_event(ups, LOG_CRIT,
               _("Failed to acquire shm lock in trim_eventfile."));
       goto trim_done;
    }

    /* Read the desired number of bytes from end of file */
    if (lseek(ups->event_fd, -maxb, SEEK_END) < 0) {
       log_event(ups, LOG_CRIT, _("lseek failed in trim_eventfile."));
       goto trim_done;
    }
    nbytes = 0;
    while ( nbytes < maxb ) {
	int r = read(ups->event_fd, buf+nbytes, maxb-nbytes);
       if (r < 0)
	   rwerror++;
       if ( r <= 0 )
	   break;
       nbytes += r;
    }

    /* Skip to the first new line */
    for ( i=0; i<nbytes; i++ )
       if ( buf[i] == '\n' ) {
	   i++;
	   break;
       }
    nbytes -= i;

    /* Truncate the file and write the buffer */
    lseek(ups->event_fd, 0, SEEK_SET);
    ftruncate(ups->event_fd, 0);
    while ( nbytes ) {
       int r = write(ups->event_fd, buf+i, nbytes);
       if (r <= 0) {
	   rwerror++;
	   break;
       }
       i += r;
       nbytes -= r;
    }
    status = 1;

trim_done:
    write_unlock(ups);
    free( buf );
    if ( rwerror )
       log_event(ups, LOG_CRIT, _("read/write failed in trim_eventfile."));
    return status;
}

#ifdef HAVE_NISSERVER

/*
 * Send the last events.
 * Returns:
 *	    -1 error or EOF
 *	     0 OK
 */
int output_events(int sockfd, FILE *events_file)
{
    char *le[NLE];
    int i, j;
    int nrec = 0;
    int stat = 0;

    for (i=0; i<NLE; i++)
	le[i] = NULL;
    for (i=0; i<NLE; i++)
	if ((le[i] = (char *)malloc(MAXSTRING)) == NULL)
	    goto bailout;
    i = 0;
    while (fgets(le[i], MAXSTRING, events_file) != NULL) {
	nrec++;
	i++;
	if (i >= NLE)		       /* wrap */
	    i = 0;
    }
    if (nrec > NLE) {
	nrec = NLE;		       /* number of records to output */
	i -= (i/NLE)*NLE;	       /* first record to output */
    } else
	i = 0;
    for (j=0; j < nrec; j++) {
	if (net_send(sockfd, le[i], strlen(le[i])) <= 0)  
	    goto bailout;
	if (++i >= NLE)
	    i = 0;
     }

    goto goodout;

bailout:
    stat = -1;

goodout:
    if (net_send(sockfd, NULL, 0) < 0) /* send eof */
	stat = -1;
    for (i=0; i<NLE; i++)
	if (le[i] != NULL)
	    free(le[i]);
    return stat;
}

#endif /* HAVE_NISSERVER */

#ifdef HAVE_CYGWIN

#include <windows.h>

extern UPSINFO *core_ups;
extern int shm_OK;

/*  
 * Fill the Events list box with the last events
 * 
 */
void FillEventsBox(HWND hwnd, int id_list)
{
    char buf[1000];
    int len;
    FILE *events_file;
    
    if (!shm_OK || core_ups->eventfile[0] == 0 ||
        (events_file = fopen(core_ups->eventfile, "r")) == NULL) {
	SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, 
           (LONG)"Events not available");
	return;
    }

    while (fgets(buf, sizeof(buf), events_file) != NULL) {
	len = strlen(buf);
	/* strip trailing cr/lfs */
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
	    buf[--len] = 0;
	SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG)buf);
    }
    return;
}

#endif /* HAVE_CYGWIN */
