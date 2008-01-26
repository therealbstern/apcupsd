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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
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
bool ApcSmartDriver::Open()
{
   int cmd;

   if ((_ups->fd = open(_ups->device, O_RDWR | O_NOCTTY | O_NDELAY | O_BINARY)) < 0)
      Error_abort2(_("Cannot open UPS port %s: %s\n"), _ups->device, strerror(errno));

   /* Cancel the no delay we just set */
   cmd = fcntl(_ups->fd, F_GETFL, 0);
   fcntl(_ups->fd, F_SETFL, cmd & ~O_NDELAY);

   /* Save old settings */
   tcgetattr(_ups->fd, &_oldtio);

   memset(&_newtio, 0, sizeof(_newtio));
   _newtio.c_cflag = DEFAULT_SPEED | CS8 | CLOCAL | CREAD;
   _newtio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   _newtio.c_oflag = 0;         /* Raw output */
   _newtio.c_lflag = 0;         /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
    defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_NETBSD_OS)
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

   // Perform initial configuration and discovery
   bool rc = Setup() && GetCapabilities() && ReadStaticData();

   // If all is well so far, start monitoring thread
   if (rc)
      run();

   return rc;
}

/*
 * This routine is the last one called before apcupsd
 * terminates.
 */
bool ApcSmartDriver::Close()
{
   /* Reset serial line to old values */
   if (_ups->fd >= 0) {
      tcflush(_ups->fd, TCIFLUSH);
      tcsetattr(_ups->fd, TCSANOW, &_oldtio);
      tcflush(_ups->fd, TCIFLUSH);

      close(_ups->fd);
   }

   _ups->fd = -1;

   return true;
}

bool ApcSmartDriver::Setup()
{
   int attempts;
   int rts_bit = TIOCM_RTS;
   char a = 'Y';

   /*
    * The following enables communcations with the
    * BackUPS Pro models in Smart Mode.
    */
   switch (_ups->cable.type) {
   case APC_940_0095A:
   case APC_940_0095B:
   case APC_940_0095C:
      /* Have to clear RTS line to access the serial cable mode PnP */
      (void)ioctl(_ups->fd, TIOCMBIC, &rts_bit);
      break;

   default:
      break;
   }

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
   Error_abort0(
      _("PANIC! Cannot communicate with UPS via serial port.\n"
        "Please make sure the port specified on the DEVICE directive is correct,\n"
        "and that your cable specification on the UPSCABLE directive is correct.\n"));

 out:
   return true;
}

void ApcSmartDriver::body()
{
   _ups->wait_time = 60;
   while (1)
   {
      // Repeat ReadVolatileData() until no interrupts are issued during it.
      // Normally this means we simply run it once. But if an async event
      // ocurrs during the call, we will poll again to ensure we pick up
      // whatever status has changed before going into CheckState().
      do {
         _interrupt = false;
         ReadVolatileData();
      } while (_interrupt);

      // No more async events, sleep until another one comes in or until
      // it is time to poll again.
      CheckState();
   }
}
