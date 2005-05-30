/*
 * apcwinipc.c
 *
 * IPC functions for Windows.
 */

/*
 * Copyright (C) 1999 Kern Sibbald <kern@sibbald.com>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

#ifdef HAVE_CYGWIN

#include <windows.h>

#ifndef HAVE_PTHREADS
static int mapsize;
static SECURITY_ATTRIBUTES atts = {
   sizeof(SECURITY_ATTRIBUTES),
   NULL,
   1                               /* inherit */
};
static HANDLE hMyMappedFile;
#endif

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
void syslog(int type, const char *fmt, ...)
{
   UPSINFO *ups = core_ups;
   va_list arg_ptr;
   char msg[2 *MAXSTRING];
   char datetime[MAXSTRING];
   struct tm tm;

   va_start(arg_ptr, fmt);
   avsnprintf(msg, sizeof(msg), fmt, arg_ptr);
   va_end(arg_ptr);

#ifdef needed
   printf("syslog: %s\n", msg);
   fflush(stdout);
#endif

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

#ifndef HAVE_PTHREADS

int semop(int semid, struct sembuf *sops, unsigned nsops)
{
   return 0;
}

int semctl(int semid, int semnum, int cmd, ...)
{
   return 0;
}

int semget(key_t key, int nsems, int semflg)
{
   return 0;
}

int shmctl(int shmid, int cmd, void *buf)
{
   return 0;
}

int shmget(key_t key, int size, int shmflg)
{
   mapsize = size;
   if (shmflg & IPC_CREAT) {
      hMyMappedFile = CreateFileMapping((HANDLE) 0xFFFFFFFF,
         &atts, PAGE_READWRITE, 0, mapsize, "apcupsd_shared_memory");

      if (hMyMappedFile == (HANDLE) 0)
         Error_abort0("Could not create shared memory file\n");
   } else {
      hMyMappedFile = OpenFileMapping(FILE_MAP_WRITE,
         FALSE, "apcupsd_shared_memory");

      if (hMyMappedFile == (HANDLE) 0)
         Error_abort0("Could not create shared memory file\n");
   }

   return 0;
}

void *shmat(int shmid, const void *shmaddr, int shmflg)
{
   char *mapview;

   if (hMyMappedFile == (HANDLE) 0)
      Error_abort0("No handle to shared memory file\n");

   mapview = (char *)MapViewOfFile(hMyMappedFile,
      FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

   if (mapview == NULL)
      Error_abort0("Could not MapView of shared memory file\n");

   return (void *)mapview;
}

int shmdt(const void *shmaddr)
{
   UnmapViewOfFile((LPVOID) shmaddr);
   return 0;
}

#endif                             /* HAVE_PTHREADS */

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
   if (ltm)
      memcpy(tm, ltm, sizeof(struct tm));

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

#endif   /* HAVE_CYGWIN */
