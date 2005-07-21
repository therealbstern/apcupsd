/*
 * Miscellaneous apcupsd memory and thread safe routines
 *   Generally, these are interfaces to system or standard
 *   library routines. 
 * 
 *
 *   Version $Id: asys.c,v 1.5 2004-07-17 21:38:01 kerns Exp $
 */
/*
   Copyright (C) 2004 Kern Sibbald

    Adapted from Bacula source code

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

/*
 * Guarantee that the string is properly terminated */
char *astrncpy(char *dest, const char *src, int maxlen)
{
   strncpy(dest, src, maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}

char *astrncat(char *dest, const char *src, int maxlen)
{
   strncat(dest, src, maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}


#ifndef DEBUG
void *amalloc(size_t size)
{
  void *buf;

  buf = malloc(size);
  if (buf == NULL) {
     Error_abort1(_("Out of memory: ERR=%s\n"), strerror(errno));
  }
  return buf;
}
#endif

void *arealloc (void *buf, size_t size)
{
   buf = realloc(buf, size);
   if (buf == NULL) {
      Error_abort1(_("Out of memory: ERR=%s\n"), strerror(errno));
   }
   return buf;
}


void *acalloc (size_t size1, size_t size2)
{
  void *buf;

   buf = calloc(size1, size2);
   if (buf == NULL) {
      Error_abort1(_("Out of memory: ERR=%s\n"), strerror(errno));
   }
   return buf;
}


#define BIG_BUF 5000
/*
 * Implement snprintf
 */
int asnprintf(char *str, size_t size, const char *fmt,	...) 
{
#ifdef HAVE_VSNPRINTF
   va_list   arg_ptr;
   int len;

   va_start(arg_ptr, fmt);
   len = vsnprintf(str, size, fmt, arg_ptr);
   va_end(arg_ptr);
   str[size-1] = 0;
   return len;

#else

   va_list   arg_ptr;
   int len;
   char *buf;

   buf = (char *)malloc(BIG_BUF);
   va_start(arg_ptr, fmt);
   len = vsprintf(buf, fmt, arg_ptr);
   va_end(arg_ptr);
   if (len >= BIG_BUF) {
      Error_abort0(_("Buffer overflow.\n"));
   }
   memcpy(str, buf, size);
   str[size-1] = 0;
   free(buf);
   return len;
#endif
}

/*
 * Implement vsnprintf()
 */
int avsnprintf(char *str, size_t size, const char  *format, va_list ap)
{
#ifdef HAVE_VSNPRINTF
   int len;
   len = vsnprintf(str, size, format, ap);
   str[size-1] = 0;
   return len;

#else

   int len;
   char *buf;
   buf = (char *)malloc(BIG_BUF);
   len = vsprintf(buf, format, ap);
   if (len >= BIG_BUF) {
      Error_abort0(_("Buffer overflow.\n"));
   }
   memcpy(str, buf, size);
   str[size-1] = 0;
   free(buf);
   return len;
#endif
}

#ifndef HAVE_LOCALTIME_R

struct tm *localtime_r(const time_t *timep, struct tm *tm)
{
   struct tm *ltm;
#ifdef HAVE_PTHREADS
   static pthread_mutex_t mutex;
   static int first = 1;

   if (first) {
      pthread_mutex_init(&mutex, NULL);
      first = 0;
   }
#endif 
   P(mutex);
   ltm = localtime(timep);
   if (ltm) {
      memcpy(tm, ltm, sizeof(struct tm));
   }
   V(mutex);
   return ltm ? tm : NULL;
}
#endif /* HAVE_LOCALTIME_R */

/*
 * These are mutex routines that do error checking
 *  for deadlock and such.  Normally not turned on.
 */
#ifdef DEBUG_MUTEX

#ifndef HAVE_PTHREADS
#error pthreads must be enabled when enabling DEBUG_MUTEX
#endif

void _p(char *file, int line, pthread_mutex_t *m)
{
   int errstat;
   if ((errstat = pthread_mutex_trylock(m))) {
      e_msg(file, line, M_ERROR, 0, _("Possible mutex deadlock.\n"));
      /* We didn't get the lock, so do it definitely now */
      if ((errstat=pthread_mutex_lock(m))) {
         e_msg(file, line, M_ABORT, 0, _("Mutex lock failure. ERR=%s\n"),
	       strerror(errstat));
      } else {
         e_msg(file, line, M_ERROR, 0, _("Possible mutex deadlock resolved.\n"));
      }
	 
   }
}

void _v(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat=pthread_mutex_trylock(m)) == 0) {
      e_msg(file, line, M_ERROR, 0, _("Mutex unlock not locked. ERR=%s\n"),
	   strerror(errstat));
    }
    if ((errstat=pthread_mutex_unlock(m))) {
       e_msg(file, line, M_ABORT, 0, _("Mutex unlock failure. ERR=%s\n"),
	      strerror(errstat));
    }
}
#endif /* DEBUG_MUTEX */


#ifdef HAVE_PTHREADS
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t timer = PTHREAD_COND_INITIALIZER;
#endif

/*
 * This routine will sleep (sec, microsec).  Note, however, that if a 
 *   signal occurs, it will return early.  It is up to the caller
 *   to recall this routine if he/she REALLY wants to sleep the
 *   requested time.
 */
int amicrosleep(time_t sec, long usec)
{
   struct timespec timeout;
   int stat;
#ifdef HAVE_PTHREADS
   struct timeval tv;
   struct timezone tz;
#endif

   timeout.tv_sec = sec;
   timeout.tv_nsec = usec * 1000;

#ifdef HAVE_NANOSLEEP
   stat = nanosleep(&timeout, NULL);
   if (!(stat < 0 && errno == ENOSYS)) {
      return stat;		     
   }
   /* If we reach here it is because nanosleep is not supported by the OS */
#endif

#ifdef HAVE_PTHREADS
   /* Do it the old way */
   gettimeofday(&tv, &tz);
   timeout.tv_nsec += tv.tv_usec * 1000;
   timeout.tv_sec += tv.tv_sec;
   while (timeout.tv_nsec >= 1000000000) {
      timeout.tv_nsec -= 1000000000;
      timeout.tv_sec++;
   }

   Dmsg1(200, "pthread_cond_timedwait sec=%d\n", timeout.tv_sec);
   /* Note, this unlocks mutex during the sleep */
   P(timer_mutex);
   stat = pthread_cond_timedwait(&timer, &timer_mutex, &timeout);
   Dmsg1(200, "pthread_cond_timedwait stat=%d\n", stat);
   V(timer_mutex);
   return stat;
#endif

#ifdef HAVE_USLEEP
   usleep(usec + sec * 1000000);
   return 0;
#endif

   sleep(sec);
   return 0;
}

/* BSDI does not have this.  This is a *poor* simulation */
#ifndef HAVE_STRTOLL
long long int
strtoll(const char *ptr, char **endptr, int base)
{
   return (long long int)strtod(ptr, endptr);	
}
#endif

/*
 * apcupsd's implementation of fgets(). The difference is that it handles
 *   being interrupted by a signal (e.g. a SIGCHLD).
 */
#undef fgetc
char *afgets(char *s, int size, FILE *fd)
{
   char *p = s;
   int ch, i;	 
   *p = 0;
   for (i=0; i < size-1; i++) {
      do {
	 errno = 0;
	 ch = fgetc(fd);
      } while (ch == -1 && (errno == EINTR || errno == EAGAIN));
      if (ch == -1) {
	 if (i == 0) {
	    return NULL;
	 } else {
	    return s;
	 }
      }
      *p++ = ch;
      *p = 0;
      if (ch == '\n') {
	 break;
      }
   }
   return s;
}
