/*
 * apcevents.c
 *
 * Output EVENTS information.
 */

/*
 * Copyright (C) 1999-2004 Kern Sibbald
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

#define NLE   50                   /* number of events to send and keep */

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
   int nbytes, maxb, i, rwerror = 0, status = -1;
   struct stat statbuf;
   unsigned char *buf;

   if (ups->eventfilemax == 0 || ups->event_fd < 0 || ups->eventfile[0] == 0)
      return 0;

   maxb = ups->eventfilemax * 1024;
   if (fstat(ups->event_fd, &statbuf) < 0)
      return -1;
   if (statbuf.st_size <= maxb)
      return 0;                    /* file is not yet too large - nothing to do */

   maxb = (maxb * 80) / 100;       /* file is too large - reduce to 80% of max */
   buf = (unsigned char *)malloc(maxb);
   if (!buf)
      return -1;

   if (write_lock(ups)) {
      log_event(ups, LOG_CRIT, _("Failed to acquire shm lock in trim_eventfile."));
      goto trim_done;
   }

   /* Read the desired number of bytes from end of file */
   if (lseek(ups->event_fd, -maxb, SEEK_END) < 0) {
      log_event(ups, LOG_CRIT, _("lseek failed in trim_eventfile."));
      goto trim_done;
   }

   nbytes = 0;
   while (nbytes < maxb) {
      int r = read(ups->event_fd, buf + nbytes, maxb - nbytes);

      if (r < 0)
         rwerror++;
      if (r <= 0)
         break;
      nbytes += r;
   }

   /* Skip to the first new line */
   for (i = 0; i < nbytes; i++) {
      if (buf[i] == '\n') {
         i++;
         break;
      }
   }
   nbytes -= i;

   /* Truncate the file and write the buffer */
   lseek(ups->event_fd, 0, SEEK_SET);
   ftruncate(ups->event_fd, 0);
   while (nbytes) {
      int r = write(ups->event_fd, buf + i, nbytes);

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
   free(buf);

   if (rwerror)
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

   for (i = 0; i < NLE; i++)
      le[i] = NULL;

   for (i = 0; i < NLE; i++)
      if ((le[i] = (char *)malloc(MAXSTRING)) == NULL)
         goto bailout;

   i = 0;
   while (fgets(le[i], MAXSTRING, events_file) != NULL) {
      nrec++;
      i++;
      if (i >= NLE)                /* wrap */
         i = 0;
   }

   if (nrec > NLE) {
      nrec = NLE;                  /* number of records to output */
      i -= (i / NLE) * NLE;        /* first record to output */
   } else {
      i = 0;
   }

   for (j = 0; j < nrec; j++) {
      if (net_send(sockfd, le[i], strlen(le[i])) <= 0)
         goto bailout;

      if (++i >= NLE)
         i = 0;
   }

   goto goodout;

bailout:
   stat = -1;

goodout:
   if (net_send(sockfd, NULL, 0) < 0)   /* send eof */
      stat = -1;

   for (i = 0; i < NLE; i++)
      if (le[i] != NULL)
         free(le[i]);

   return stat;
}

#endif                             /* HAVE_NISSERVER */

#ifdef HAVE_CYGWIN

#include <windows.h>

extern UPSINFO *core_ups;
extern int shm_OK;

#define MAX_READ 20000

/* Fill the Events list box with the last events */
void FillEventsBox(HWND hwnd, int id_list)
{
   off_t flen, fpos;
   ssize_t len;
   char *buf, *p, *l;

   if (!shm_OK || core_ups->event_fd > 0) {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0,
         (LONG) "Events not available");
      return;
   }

   /* Get size of file, but read only 20000 bytes at most */
   flen = lseek(core_ups->event_fd, (off_t) 0, SEEK_END);
   if (flen > MAX_READ) {
      fpos = flen - MAX_READ;
      flen = MAX_READ;
   } else {
      fpos = 0;
   }

   buf = (char *)malloc(flen + 1);
   lseek(core_ups->event_fd, fpos, SEEK_SET);

   len = read(core_ups->event_fd, buf, flen);
   lseek(core_ups->event_fd, (off_t) 0, SEEK_END);
   if (len < 0) {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0,
         (LONG) "Events not available");
      return;
   }

   buf[len] = 0;
   len = 0;
   for (l = p = buf; *p; p++) {
      if (*p == '\n' || *p == '\r') {
         *p = 0;

         if (len > 1)
            SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) l);

         l = p + 1;
         len = 0;
      } else {
         len++;
      }
   }

   free(buf);
   return;
}

#endif   /* HAVE_CYGWIN */
