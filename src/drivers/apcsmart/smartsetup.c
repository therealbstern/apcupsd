/*
 * smartrsetup.c
 *
 * Functions to open/setup/close the device
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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
#include "apcsmart.h"

/* Win32 needs O_BINARY; sane platforms have never heard of it */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * This is the first routine in the driver that is called.
 */
bool ApcSmartUpsDriver::Open()
{
   int cmd;

   char *opendev = _ups->device;

#ifdef HAVE_MINGW
   // On Win32 add \\.\ UNC prefix to COMx in order to correctly address
   // ports >= COM10.
   char device[MAXSTRING];
   if (!strnicmp(_ups->device, "COM", 3)) {
      snprintf(device, sizeof(device), "\\\\.\\%s", _ups->device);
      opendev = device;
   }
#endif

   Dmsg(50, "Opening port %s\n", opendev);
   if ((_ups->fd = open(opendev, O_RDWR | O_NOCTTY | O_NDELAY | O_BINARY | O_CLOEXEC)) < 0)
   {
      Dmsg(50, "Cannot open UPS port %s: %s\n", opendev, strerror(errno));
      return false;
   }

   /* Cancel the no delay we just set */
   cmd = fcntl(_ups->fd, F_GETFL, 0);
   fcntl(_ups->fd, F_SETFL, cmd & ~O_NDELAY);

   /* Save old settings */
   tcgetattr(_ups->fd, &_oldtio);

   _newtio.c_cflag = DEFAULT_SPEED | CS8 | CLOCAL | CREAD;
   _newtio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   _newtio.c_oflag = 0;         /* Raw output */
   _newtio.c_lflag = 0;         /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
    defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_NETBSD_OS) || \
    defined(HAVE_QNX_OS)
   _newtio.c_ispeed = DEFAULT_SPEED;    /* Set input speed */
   _newtio.c_ospeed = DEFAULT_SPEED;    /* Set output speed */
#endif   /* __openbsd__ || __freebsd__ || __netbsd__  */

   /* This makes a non.blocking read() with TIMER_READ (10) sec. timeout */
   _newtio.c_cc[VMIN] = 0;
   _newtio.c_cc[VTIME] = TIMER_READ * 10;

#if defined(HAVE_OSF1_OS) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
   (void)cfsetospeed(&_newtio, DEFAULT_SPEED);
   (void)cfsetispeed(&_newtio, DEFAULT_SPEED);
#endif  /* do it the POSIX way */

   tcflush(_ups->fd, TCIFLUSH);
   tcsetattr(_ups->fd, TCSANOW, &_newtio);
   tcflush(_ups->fd, TCIFLUSH);

   return 1;
}

/*
 * This routine is the last one called before apcupsd
 * terminates.
 */
bool ApcSmartUpsDriver::Close()
{
   /* Reset serial line to old values */
   if (_ups->fd >= 0) {
      Dmsg(50, "Closing port\n");
      tcflush(_ups->fd, TCIFLUSH);
      tcsetattr(_ups->fd, TCSANOW, &_oldtio);
      tcflush(_ups->fd, TCIFLUSH);

      close(_ups->fd);
   }

   _ups->fd = -1;

   return 1;
}

bool ApcSmartUpsDriver::setup()
{
   int attempts;
   int rts_bit = TIOCM_RTS;
   char a = 'Y';

   if (_ups->fd == -1)
      return 1;                    /* we must be a slave */

   /* Have to clear RTS line to access the serial cable mode PnP on BKPro */
   /* Shouldn't hurt on other cables, so just do it all the time. */
   ioctl(_ups->fd, TIOCMBIC, &rts_bit);

   write(_ups->fd, &a, 1);          /* This one might not work, if UPS is */
   sleep(1);                       /* in an unstable communication state */
   tcflush(_ups->fd, TCIOFLUSH);    /* Discard UPS's response, if any */

   /*
    * Don't use smart_poll here because it may loop waiting
    * on the serial port, and here we may be called before
    * we are a deamon, so we want to error after a reasonable
    * time.
    */
   for (attempts = 0; attempts < 5; attempts++) {
      char answer[10];

      *answer = 0;
      write(_ups->fd, &a, 1);       /* enter smart mode */
      getline(answer, sizeof(answer));
      if (strcmp("SM", answer) == 0)
         goto out;
      sleep(1);
   }
   Error_abort(
      "PANIC! Cannot communicate with UPS via serial port.\n"
      "Please make sure the port specified on the DEVICE directive is correct,\n"
      "and that your cable specification on the UPSCABLE directive is correct.\n");

 out:
   return 1;
}
