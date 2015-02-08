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

/* Convert Win32 baud constants to POSIX constants */
static int posixbaud(DWORD baud)
{
   switch(baud) {
   case CBR_110:
      return B110;
   case CBR_300:
      return B300;
   case CBR_600:
      return B600;
   case CBR_1200:
      return B1200;
   case CBR_2400:
   default:
      return B2400;
   case CBR_4800:
      return B4800;
   case CBR_9600:
      return B9600;
   case CBR_19200:
      return B19200;
   case CBR_38400:
      return B38400;
   case CBR_57600:
      return B57600;
   case CBR_115200:
      return B115200;
   case CBR_128000:
      return B128000;
   case CBR_256000:
      return B256000;
   }
}

/* Convert Win32 bytesize constants to POSIX constants */
static int posixsize(BYTE size)
{
   switch(size) {
   case 5:
      return CS5;
   case 6:
      return CS6;
   case 7:
      return CS7;
   case 8:
   default:
      return CS8;
   }
}

int tcgetattr (int fd, struct termios *out)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);

   memset(out, 0, sizeof(*out));
   
   out->c_cflag |= posixbaud(dcb.BaudRate);
   out->c_cflag |= posixsize(dcb.ByteSize);

   if (dcb.StopBits == TWOSTOPBITS)
      out->c_cflag |= CSTOPB;
   if (dcb.fParity) {
      out->c_cflag |= PARENB;
      if (dcb.Parity == ODDPARITY)
         out->c_cflag |= PARODD;
   }

   if (!dcb.fOutxCtsFlow && !dcb.fOutxDsrFlow && !dcb.fDsrSensitivity)
      out->c_cflag |= CLOCAL;
      
   if (dcb.fOutX)
      out->c_iflag |= IXON;
   if (dcb.fInX)
      out->c_iflag |= IXOFF;

   return 0;
}
