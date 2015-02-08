/*
 * Copyright (C) 2006 Adam Kropelin
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

#include <errno.h>
#include <winsock2.h>
#include <io.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include "winapi.h"

static int tiocmbic(int fd, int bits)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   if (bits & TIOCM_ST)
   {
      // Win32 API does not allow manipulating ST
      errno = EINVAL;
      return -1;
   }

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);
   
   if (bits & TIOCM_DTR)
      dcb.fDtrControl = DTR_CONTROL_DISABLE;
   if (bits & TIOCM_RTS)
      dcb.fRtsControl = RTS_CONTROL_DISABLE;

   SetCommState(h, &dcb);
   return 0;
}

static int tiocmbis(int fd, int bits)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   if (bits & TIOCM_SR)
   {
      // Win32 API does not allow manipulating SR
      errno = EINVAL;
      return -1;
   }

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);
   
   if (bits & TIOCM_DTR)
      dcb.fDtrControl = DTR_CONTROL_ENABLE;
   if (bits & TIOCM_RTS)
      dcb.fRtsControl = RTS_CONTROL_ENABLE;

   SetCommState(h, &dcb);
   return 0;
}

static int tiocmget(int fd, int *bits)
{
   DWORD status;

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }
   
   GetCommModemStatus(h, &status);

   *bits = 0;

   if (status & MS_CTS_ON)
      *bits |= TIOCM_CTS;
   if (status & MS_DSR_ON)
      *bits |= TIOCM_DSR;
   if (status & MS_RING_ON)
      *bits |= TIOCM_RI;
   if (status & MS_RLSD_ON)
      *bits |= TIOCM_CD;
   
   return 0;
}

int ioctl(int fd, int request, ...)
{
   int rc;
   u_long v;
   va_list list;
   va_start(list, request);

   /* We only know how to emulate a few ioctls */
   switch (request) {
   case TIOCMBIC:
      rc = tiocmbic(fd, *va_arg(list, int*));
      break;
   case TIOCMBIS:
      rc = tiocmbis(fd, *va_arg(list, int*));
      break;
   case TIOCMGET:
      rc = tiocmget(fd, va_arg(list, int*));
      break;
   case FIONBIO:
      v = *va_arg(list, int*);
      rc = ioctlsocket(fd, request, &v);
      break;
   default:
      rc = -1;
      errno = EINVAL;
      break;
   }

   va_end(list);
   return rc;
}
