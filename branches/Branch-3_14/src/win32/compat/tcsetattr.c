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

/* Convert POSIX baud constants to Win32 constants */
static DWORD winbaud(int baud)
{
   switch(baud) {
   case B110:
      return CBR_110;
   case B300:
      return CBR_300;
   case B600:
      return CBR_600;
   case B1200:
      return CBR_1200;
   case B2400:
   default:
      return CBR_2400;
   case B4800:
      return CBR_4800;
   case B9600:
      return CBR_9600;
   case B19200:
      return CBR_19200;
   case B38400:
      return CBR_38400;
   case B57600:
      return CBR_57600;
   case B115200:
      return CBR_115200;
   case B128000:
      return CBR_128000;
   case B256000:
      return CBR_256000;
   }
}

/* Convert POSIX bytesize constants to Win32 constants */
static BYTE winsize(int size)
{
   switch(size) {
   case CS5:
      return 5;
   case CS6:
      return 6;
   case CS7:
      return 7;
   case CS8:
   default:
      return 8;
   }
}

int tcsetattr (int fd, int optional_actions, const struct termios *in)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);

   dcb.fBinary = 1;
   dcb.BaudRate = winbaud(in->c_cflag & CBAUD);
   dcb.ByteSize = winsize(in->c_cflag & CSIZE);
   dcb.StopBits = in->c_cflag & CSTOPB ? TWOSTOPBITS : ONESTOPBIT;

   if (in->c_cflag & PARENB) {
      dcb.fParity = 1;
      dcb.Parity = in->c_cflag & PARODD ? ODDPARITY : EVENPARITY;
   } else {
      dcb.fParity = 0;
      dcb.Parity = NOPARITY;
   }

   if (in->c_cflag & CLOCAL) {
      dcb.fOutxCtsFlow = 0;
      dcb.fOutxDsrFlow = 0;
      dcb.fDsrSensitivity = 0;
   }

   dcb.fOutX = !!(in->c_iflag & IXON);
   dcb.fInX = !!(in->c_iflag & IXOFF);

   SetCommState(h, &dcb);

   /* If caller wants a read() timeout, set that up */
   if (in->c_cc[VMIN] == 0 && in->c_cc[VTIME] != 0) {
      COMMTIMEOUTS ct;
      ct.ReadIntervalTimeout = MAXDWORD;
      ct.ReadTotalTimeoutMultiplier = MAXDWORD;
      ct.ReadTotalTimeoutConstant = in->c_cc[VTIME] * 100;
      ct.WriteTotalTimeoutMultiplier = 0;
      ct.WriteTotalTimeoutConstant = 0;
      SetCommTimeouts(h, &ct);
   }

   return 0;
}
