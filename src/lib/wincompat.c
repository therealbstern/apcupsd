/*
 * wincompat.c
 *
 * Functions to make apcupsd work with Windows
 */

/*
 * Copyright (C) 1999-2006 Kern Sibbald <kern@sibbald.com>
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

#if  defined(HAVE_CYGWIN) || defined(HAVE_MINGW)

#include <windows.h>

extern UPSINFO *core_ups;

/*
 * Normally, when this routine is called,
 * the log event will also be record in the
 * event_fd file. However, if we have a problem
 * before the config file is read, this file will
 * NOT be open. To avoid loosing the error message,
 * we explicitly open the file here, which we hard code
 * as /apcupsd/etc/apcupsd/apcupsd.events
 *
 * All of this is necessary because Win32 doesn't have a syslog.
 * Actually WinNT does have a system log, but we don't use it
 * here.
 */
extern "C" void syslog(int type, const char *fmt, ...)
{
   UPSINFO *ups = core_ups;
   va_list arg_ptr;
   char msg[2 *MAXSTRING];
   char datetime[MAXSTRING];
   struct tm tm;

   va_start(arg_ptr, fmt);
   avsnprintf(msg, sizeof(msg), fmt, arg_ptr);
   va_end(arg_ptr);


   /*
    * Write out to our temp file. LOG_INFO is DATA logging, so
    * do not write it to our temp events file.
    */
   if (ups == NULL || ups->event_fd < 0) {
      int lm;
      time_t nowtime;
      int tmp_fd;

      /* Simulate syslog */
      tmp_fd =
         open("/apcupsd/etc/apcupsd/apcupsd.events",
            O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (tmp_fd < 0) {
         printf("Could not open events file: %s\n", strerror(errno));
         fflush(stdout);
      } else {
         time(&nowtime);
         localtime_r(&nowtime, &tm);
         strftime(datetime, 100, "%a %b %d %X %Z %Y  ", &tm);
         write(tmp_fd, datetime, strlen(datetime));

         lm = strlen(msg);
         if (msg[lm - 1] != '\n')
            msg[lm++] = '\n';

         write(tmp_fd, msg, lm);
         close(tmp_fd);
      }
   }
}

#ifndef HAVE_MINGW
struct tm *localtime_r(const time_t *timep, struct tm *tm)
{
   static pthread_mutex_t mutex;
   static int first = 1;
   struct tm *ltm;

   if (first) {
      pthread_mutex_init(&mutex, NULL);
      first = 0;
   }

   P(mutex);

   ltm = localtime(timep);
   if (ltm) {
      memcpy(tm, ltm, sizeof(struct tm));
   }

   V(mutex);

   return ltm ? tm : NULL;
}


void WinMessageBox(char *msg)
{
   MessageBox(NULL, msg, "apcupsd message", MB_OK);
}

extern "C" HANDLE get_osfhandle(int fd);

/*
 * Implement a very simple form of serial port
 * line status ioctl() for Win32 apcupds'.  
 *
 *  This routine can get:
 *    CTS, DSR, RNG, and CD
 *
 *  It can set/clear:
 *    DTR, and RTS
 *
 * All other requests are silently ignored.
 *
 * Returns: as for ioctl();
 */
int winioctl(int fd, int func, int *addr)
{
   HANDLE hComm;
   DWORD dwModemStatus;
   int lb = 0;

   /* Get Windows Handle from CYGWIN */
   hComm = get_osfhandle(fd);
   if (hComm == 0)
      return EBADF;

   switch (func) {
      
   case TIOCMGET:  /* Get the comm port status */
      if (!GetCommModemStatus(hComm, &dwModemStatus))
         return EINVAL;

      if (MS_CTS_ON & dwModemStatus)
         lb |= TIOCM_CTS;
      if (MS_DSR_ON & dwModemStatus)
         lb |= TIOCM_DSR;
      if (MS_RING_ON & dwModemStatus)
         lb |= TIOCM_RNG;
      if (MS_RLSD_ON & dwModemStatus)
         lb |= TIOCM_CD;

      *addr = lb;
      return 0;

   case TIOCMBIS:  /* Set a comm port bit */
      if (*addr & TIOCM_DTR)
         EscapeCommFunction(hComm, SETDTR);
      if (*addr & TIOCM_RTS)
         EscapeCommFunction(hComm, SETRTS);
      return 0;

   case TIOCMBIC:  /* Clear a comm port bit or two */
      if (*addr & TIOCM_DTR)
         EscapeCommFunction(hComm, CLRDTR);
      if (*addr & TIOCM_RTS)
         EscapeCommFunction(hComm, CLRRTS);
      return 0;
   }   /* end switch */

   return EINVAL;
}
#endif

#endif   /* HAVE_CYGWIN */
