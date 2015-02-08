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
#include <io.h>
#include <sys/termios.h>
#include "winapi.h"

int tcflush(int fd, int queue_selector)
{
   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   DWORD flags = 0;

   switch (queue_selector) {
   case TCIFLUSH:
      flags |= PURGE_RXCLEAR;
      break;
   case TCOFLUSH:
      flags |= PURGE_TXCLEAR;
      break;
   case TCIOFLUSH:
      flags |= PURGE_RXCLEAR;
      flags |= PURGE_TXCLEAR;
      break;
   }
   
   PurgeComm(h, flags);
   return 0;
}
