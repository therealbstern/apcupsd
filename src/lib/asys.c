/*
 * asys.c
 *
 * Miscellaneous apcupsd memory and thread safe routines
 * Generally, these are interfaces to system or standard
 * library routines. 
 *
 * Adapted from Bacula source code
 */

/*
 * Copyright (C) 2004 Kern Sibbald
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#include "apc.h"

#define BIG_BUF 5000

/* Implement snprintf */
int asnprintf(char *str, size_t size, const char *fmt, ...)
{
#ifdef HAVE_VSNPRINTF
   va_list arg_ptr;
   int len;

   va_start(arg_ptr, fmt);
   len = vsnprintf(str, size, fmt, arg_ptr);
   va_end(arg_ptr);

   str[size - 1] = 0;
   return len;

#else

   va_list arg_ptr;
   int len;
   char *buf;

   buf = (char *)malloc(BIG_BUF);

   va_start(arg_ptr, fmt);
   len = vsprintf(buf, fmt, arg_ptr);
   va_end(arg_ptr);

   if (len >= BIG_BUF)
      Error_abort("Buffer overflow.\n");

   memcpy(str, buf, size);
   str[size - 1] = 0;

   free(buf);
   return len;
#endif
}

/* Implement vsnprintf() */
int avsnprintf(char *str, size_t size, const char *format, va_list ap)
{
#ifdef HAVE_VSNPRINTF
   int len;

   len = vsnprintf(str, size, format, ap);
   str[size - 1] = 0;

   return len;

#else

   int len;
   char *buf;

   buf = (char *)malloc(BIG_BUF);

   len = vsprintf(buf, format, ap);
   if (len >= BIG_BUF)
      Error_abort("Buffer overflow.\n");

   memcpy(str, buf, size);
   str[size - 1] = 0;

   free(buf);
   return len;
#endif
}

/*
 * These are mutex routines that do error checking
 * for deadlock and such.  Normally not turned on.
 */
#ifdef DEBUG_MUTEX

void _p(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat = pthread_mutex_trylock(m))) {
      e_msg(file, line, M_ERROR, 0, "Possible mutex deadlock.\n");

      /* We didn't get the lock, so do it definitely now */
      if ((errstat = pthread_mutex_lock(m))) {
         e_msg(file, line, M_ABORT, 0,
            "Mutex lock failure. ERR=%s\n", strerror(errstat));
      } else {
         e_msg(file, line, M_ERROR, 0,
            "Possible mutex deadlock resolved.\n");
      }
   }
}

void _v(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat = pthread_mutex_trylock(m)) == 0) {
      e_msg(file, line, M_ERROR, 0,
         "Mutex unlock not locked. ERR=%s\n", strerror(errstat));
   }
   if ((errstat = pthread_mutex_unlock(m))) {
      e_msg(file, line, M_ABORT, 0,
         "Mutex unlock failure. ERR=%s\n", strerror(errstat));
   }
}
#endif   /* DEBUG_MUTEX */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size)
{
   size_t cnt = 0;
   if (size)
   {
      while (*src && cnt < size-1)
      {
         *dst++ = *src++;
         ++cnt;
      }
      *dst = '\0';
   }
   while (*src++)
      ++cnt;
   return cnt;
}
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t size)
{
   size_t cnt = 0;
   while (*dst && cnt < size)
   {
      ++dst;
      ++cnt;
   }
   cnt += strlcpy(dst, src, size-cnt);
   return cnt;
}
#endif
