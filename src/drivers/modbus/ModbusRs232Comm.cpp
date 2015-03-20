/*
 * modbus.cpp
 *
 * Driver for APC MODBUS protocol
 */

/*
 * Copyright (C) 2013 Adam Kropelin
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

/*
 * Thanks go to APC/Schneider for providing the Apcupsd team with early access
 * to MODBUS protocol information to facilitate an Apcupsd driver.
 *
 * APC/Schneider has published the following relevant application notes:
 *
 * AN176: Modbus Implementation in APC Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=176>
 * AN177: Software interface for Switched Outlet and UPS Management in Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=177>
 * AN178: USB HID Implementation in Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=178>
 */

#include "apc.h"
#include "ModbusRs232Comm.h"

/* Win32 needs O_BINARY; sane platforms have never heard of it */
#ifndef O_BINARY
#define O_BINARY 0
#endif

ModbusRs232Comm::ModbusRs232Comm(uint8_t slaveaddr) :
   ModbusComm(slaveaddr),
   _fd(-1)
{
}

bool ModbusRs232Comm::Open(const char *path)
{
   // Close if we're already open
   Close();

#ifdef HAVE_MINGW
   // On Win32 add \\.\ UNC prefix to COMx in order to correctly address
   // ports >= COM10.
   char device[MAXSTRING];
   if (!strnicmp(path, "COM", 3)) {
      snprintf(device, sizeof(device), "\\\\.\\%s", path);
      path = device;
   }
#endif

   if ((_fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY | O_BINARY | O_CLOEXEC)) < 0)
   {
      Dmsg(0, "%s: open(\"%s\") fails: %s\n", __func__, 
         path, strerror(errno));
      return false;
   }

   /* Cancel the no delay we just set */
   int cmd = fcntl(_fd, F_GETFL, 0);
   if (cmd != -1)
      (void)fcntl(_fd, F_SETFL, cmd & ~O_NDELAY);

   struct termios newtio;
   newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   newtio.c_oflag = 0;         /* Raw output */
   newtio.c_lflag = 0;         /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
    defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_NETBSD_OS) || \
    defined(HAVE_QNX_OS)
   newtio.c_ispeed = B9600;    /* Set input speed */
   newtio.c_ospeed = B9600;    /* Set output speed */
#endif   /* __openbsd__ || __freebsd__ || __netbsd__  */

   /* read() blocks until at least 1 char is received or 100ms btw chars */
   newtio.c_cc[VMIN] = 0;
   newtio.c_cc[VTIME] = 0;

#if defined(HAVE_OSF1_OS) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
   (void)cfsetospeed(&newtio, B9600);
   (void)cfsetispeed(&newtio, B9600);
#endif  /* do it the POSIX way */

   tcflush(_fd, TCIFLUSH);
   tcsetattr(_fd, TCSANOW, &newtio);
   tcflush(_fd, TCIFLUSH);

   _open = true;
   return true;
}

bool ModbusRs232Comm::Close()
{
   _open = false;
   close(_fd);
   _fd = -1;
   return true;
}

bool ModbusRs232Comm::ModbusTx(const ModbusFrame *frm, unsigned int sz)
{
   // Wait for line to become idle
   if (!ModbusWaitIdle())
      return false;

   Dmsg(100, "%s: Sending frame\n", __func__);
   hex_dump(100, frm, sz);

   // Write frame
   int rc = write(_fd, frm, sz);
   if (rc == -1)
   {
      Dmsg(0, "%s: write() fails: %s\n", __func__, strerror(errno));
      return false;
   }
   else if ((unsigned int)rc != sz)
   {
      Dmsg(0, "%s: write() short (%d of %u)\n", __func__, rc, sz);
      return false;
   }

   return true;
}

bool ModbusRs232Comm::ModbusRx(ModbusFrame *frm, unsigned int *sz)
{
#ifdef HAVE_MINGW
   // Set read timeout since we have no select() support
   COMMTIMEOUTS ct;
   HANDLE h = (HANDLE)_get_osfhandle(_fd);
   ct.ReadIntervalTimeout = MODBUS_INTERCHAR_TIMEOUT_MS;
   ct.ReadTotalTimeoutMultiplier = 0;
   ct.ReadTotalTimeoutConstant = MODBUS_RESPONSE_TIMEOUT_MS;
   ct.WriteTotalTimeoutMultiplier = 0;
   ct.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(h, &ct);

   int rc = read(_fd, frm, MODBUS_MAX_FRAME_SZ);
   if (rc == -1)
   {
      // Fatal read error
      Dmsg(0, "%s: read() fails: %s\n", __func__, strerror(errno));
      return false;
   }
   else if (rc == 0)
   {
      // Timeout
      Dmsg(0, "%s: timeout\n", __func__);
      return false;
   }

   Dmsg(100, "%s: Received frame\n", __func__);
   hex_dump(100, frm, rc);

   // Received a message ok
   *sz = rc;
   return true;
#else
   unsigned int nread = 0;
   unsigned int timeout = MODBUS_RESPONSE_TIMEOUT_MS;
   struct timeval tv;
   fd_set fds;
   int rc;

   while(1)
   {
      // Wait for character(s) to be available for reading
      do
      {
         FD_ZERO(&fds);
         FD_SET(_fd, &fds);

         tv.tv_sec = timeout / 1000;
         tv.tv_usec = (timeout % 1000) * 1000;

         rc = select(_fd+1, &fds, NULL, NULL, &tv);
      }
      while (rc == -1 && (errno == EAGAIN || errno == EINTR));

      if (rc == -1)
      {
         // fatal select() error
         Dmsg(0, "%s: select() failed: %s\n", __func__, strerror(errno));
         return false;
      }
      else if (rc == 0)
      {
         // If we've received some characters, this is simply the inter-char
         // timeout signalling the end of the frame...i.e. the success case
         if (nread)
         {
            Dmsg(100, "%s: Received frame\n", __func__);
            hex_dump(100, frm, nread);
            *sz = nread;
            return true;
         }

         // No chars read yet so this is a fatal timeout
         Dmsg(0, "%s: -------------------TIMEOUT\n", __func__);
         return false;
      }

      // Received at least 1 character so switch to inter-char timeout
      timeout = MODBUS_INTERCHAR_TIMEOUT_MS;

      // Read characters
      int rc = read(_fd, *frm+nread, MODBUS_MAX_FRAME_SZ-nread);
      if (rc == -1)
      {
         // Fatal read error
         Dmsg(0, "%s: read() fails: %s\n", __func__, strerror(errno));
         return false;
      }
      nread += rc;

      if (rc == 0)
      {
         Dmsg(0, "%s: 0-length read\n", __func__);
      }

      // Check for max message size
      if (nread == MODBUS_MAX_FRAME_SZ)
      {
         *sz = nread;
         return true;
      }
   }
#endif
}

bool ModbusRs232Comm::ModbusWaitIdle()
{
   // Determine when we need to exit by
   struct timeval exittime, now;
   gettimeofday(&exittime, NULL);
   exittime.tv_sec += MODBUS_IDLE_WAIT_TIMEOUT_MS / 1000;
   exittime.tv_usec += (MODBUS_IDLE_WAIT_TIMEOUT_MS % 1000) * 1000;
   if (exittime.tv_usec >= 1000000)
   {
      exittime.tv_sec++;
      exittime.tv_usec -= 1000000;
   }

#ifdef HAVE_MINGW
   // Set read timeout since we have no select() support
   COMMTIMEOUTS ct;
   HANDLE h = (HANDLE)_get_osfhandle(_fd);
   ct.ReadIntervalTimeout = MAXDWORD;
   ct.ReadTotalTimeoutMultiplier = MAXDWORD;
   ct.ReadTotalTimeoutConstant = MODBUS_INTERFRAME_TIMEOUT_MS;
   ct.WriteTotalTimeoutMultiplier = 0;
   ct.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(h, &ct);
#endif

   while (1)
   {
      unsigned char tmp;

#ifdef HAVE_MINGW
      int rc = read(_fd, &tmp, 1);
      if (rc == 0)
      {
         // timeout: line is now idle
         return true;
      }
      else if (rc == -1)
      {
         // fatal read error
         Dmsg(0, "%s: read() failed: %s\n", __func__, strerror(errno));
         return false;
      }

      // char received: unexpected...
      Dmsg(100, "%s: Discarding unexpected character 0x%x\n",
         __func__, tmp);
#else
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(_fd, &fds);

      struct timeval timeout;
      timeout.tv_sec = MODBUS_INTERFRAME_TIMEOUT_MS / 1000;
      timeout.tv_usec = (MODBUS_INTERFRAME_TIMEOUT_MS % 1000) * 1000;
      int rc = select(_fd+1, &fds, NULL, NULL, &timeout);
      if (rc == 0)
      {
         // timeout: line is now idle
         return true;
      }
      else if (rc == -1 && errno != EINTR && errno != EAGAIN)
      {
         // fatal select() error
         Dmsg(0, "%s: select() failed: %s\n", __func__, strerror(errno));
         return false;
      }
      else if (rc == 1)
      {
         // char received: unexpected...
         read(_fd, &tmp, 1);
         Dmsg(100, "%s: Discarding unexpected character 0x%x\n",
            __func__, tmp);
      }
#endif

      gettimeofday(&now, NULL);
      if (now.tv_sec > exittime.tv_sec ||
          (now.tv_sec == exittime.tv_sec &&
           now.tv_usec >= exittime.tv_usec))
      {
         // Line did not become idle soon enough
         Dmsg(0, "%s: Timeout\n", __func__);
         return false;
      }
   }
}
