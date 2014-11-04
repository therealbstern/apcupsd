//                              -*- Mode: C++ -*-
// compat.cpp -- compatibilty layer to make bacula-fd run
//               natively under windows
//
// Copyright transferred from Raider Solutions, Inc to
//   Kern Sibbald and John Walker by express permission.
//
//  Copyright (C) 2004-2006 Kern Sibbald
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  version 2 as amended with additional clauses defined in the
//  file LICENSE in the main source directory.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  the file LICENSE for additional details.
//
// Author          : Christopher S. Hull
// Created On      : Sat Jan 31 15:55:00 2004
// $Id: compat.cpp,v 1.22.2.5 2010-09-10 14:50:12 adk0212 Exp $

#include "apc.h"
#include "compat.h"
#include "winapi.h"

#define b_errno_win32 (1<<29)

/* apcupsd doesn't need special allocators */
#define get_pool_memory(x) (char *)malloc(x)
#define free_pool_memory(x) free((char *)x)
#define check_pool_memory_size(x, y) x
#define PM_FNAME 2000
#define PM_MESSAGE 2000

/* No assertion checking */
#define ASSERT(x) 

/* to allow the usage of the original version in this file here */
#undef fputs

#define USE_WIN32_COMPAT_IO 1
#define USE_WIN32_32KPATHCONVERSION 1

// from MicroSoft SDK (KES) is the diff between Jan 1 1601 and Jan 1 1970
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000ULL 
#define WIN32_FILETIME_SCALE  10000000             // 100ns/second

void conv_unix_to_win32_path(const char *name, char *win32_name, DWORD dwSize)
{
    const char *fname = name;
    char *tname = win32_name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (name[0] == '/' && name[1] == '/') {  /* double slash? */
           name++;                               /* yes, skip first one */
        }
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    /* but leave "c:\" with backslash (root directory case */
    if (*fname != 0 && win32_name[-1] == '\\' && strlen (fname) != 3) {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }
}

int umask(int)
{
   return 0;
}

int chmod(const char *, mode_t)
{
   return 0;
}

int chown(const char *k, uid_t, gid_t)
{
   return 0;
}

int lchown(const char *k, uid_t, gid_t)
{
   return 0;
}

long int
random(void)
{
    return rand();
}

void
srandom(unsigned int seed)
{
   srand(seed);
}

// /////////////////////////////////////////////////////////////////
// convert from Windows concept of time to Unix concept of time
// /////////////////////////////////////////////////////////////////
void
cvt_utime_to_ftime(const time_t  &time, FILETIME &wintime)
{
    uint64_t mstime = time;
    mstime *= WIN32_FILETIME_SCALE;
    mstime += WIN32_FILETIME_ADJUST;

    #ifdef HAVE_MINGW
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffUL);
    #else
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffI64);
    #endif
    wintime.dwHighDateTime = (DWORD) ((mstime>>32)& 0xffffffffUL);
}

time_t
cvt_ftime_to_utime(const FILETIME &time)
{
    uint64_t mstime = time.dwHighDateTime;
    mstime <<= 32;
    mstime |= time.dwLowDateTime;

    mstime -= WIN32_FILETIME_ADJUST;
    mstime /= WIN32_FILETIME_SCALE; // convert to seconds.

    return (time_t) (mstime & 0xffffffff);
}

void
sleep(int sec)
{
   Sleep(sec * 1000);
}

int
geteuid(void)
{
   return 0;
}

int
execvp(const char *, char *[]) {
   errno = ENOSYS;
   return -1;
}


int
fork(void)
{
   errno = ENOSYS;
   return -1;
}

int
waitpid(int, int*, int)
{
   errno = ENOSYS;
   return -1;
}

int
readlink(const char *, char *, int)
{
   errno = ENOSYS;
   return -1;
}

int
strncasecmp(const char *s1, const char *s2, int len)
{
    register int ch1, ch2;

    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    while (len--) {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
        if (ch1 == 0 || tolower(ch1) != tolower(ch2)) break;
    }

    return (ch1 - ch2);
}

int
gettimeofday(struct timeval *tv, struct timezone *)
{
    SYSTEMTIME now;
    FILETIME tmp;
    GetSystemTime(&now);

    if (tv == NULL) {
       errno = EINVAL;
       return -1;
    }
    if (!SystemTimeToFileTime(&now, &tmp)) {
       errno = b_errno_win32;
       return -1;
    }

    int64_t _100nsec = tmp.dwHighDateTime;
    _100nsec <<= 32;
    _100nsec |= tmp.dwLowDateTime;
    _100nsec -= WIN32_FILETIME_ADJUST;

    tv->tv_sec =(long) (_100nsec / 10000000);
    tv->tv_usec = (long) ((_100nsec % 10000000)/10);
    return 0;

}

void
init_stack_dump(void)
{

}


long
pathconf(const char *path, int name)
{
    switch(name) {
    case _PC_PATH_MAX :
        if (strncmp(path, "\\\\?\\", 4) == 0)
            return 32767;
    case _PC_NAME_MAX :
        return 255;
    }
    errno = ENOSYS;
    return -1;
}

int
kill(int pid, int signal)
{
   int rval = 0;
   DWORD exitcode = 0;

   switch (signal) {
   case SIGTERM:
      /* Terminate the process */
      if (!TerminateProcess((HANDLE)pid, (UINT) signal)) {
         rval = -1;
         errno = b_errno_win32;
      }
      CloseHandle((HANDLE)pid);
      break;
   case 0:
      /* Just check if process is still alive */
      if (GetExitCodeProcess((HANDLE)pid, &exitcode) &&
          exitcode != STILL_ACTIVE) {
         rval = -1;
      }
      break;
   default:
      /* Don't know what to do, so just fail */
      rval = -1;
      errno = EINVAL;
      break;   
   }

   return rval;
}

/* Implement syslog() using Win32 Event Service */
void syslog(int type, const char *fmt, ...)
{
   va_list arg_ptr;
   char message[MAXSTRING];
   HANDLE heventsrc;
   char* strings[32];
   WORD wtype;

   va_start(arg_ptr, fmt);
   message[0] = '\n';
   message[1] = '\n';
   avsnprintf(message+2, sizeof(message)-2, fmt, arg_ptr);
   va_end(arg_ptr);

   strings[0] = message;

   // Convert syslog type to Win32 type. This mapping is somewhat arbitrary
   // since there are many more LOG_* types than EVENTLOG_* types.
   switch (type) {
   case LOG_ERR:
      wtype = EVENTLOG_ERROR_TYPE;
      break;
   case LOG_CRIT:
   case LOG_ALERT:
   case LOG_WARNING:
      wtype = EVENTLOG_WARNING_TYPE;
      break;
   default:
      wtype = EVENTLOG_INFORMATION_TYPE;
      break;
   }

   // Use event logging to log the error
   heventsrc = RegisterEventSource(NULL, "Apcupsd");

   if (heventsrc != NULL) {
      ReportEvent(
              heventsrc,              // handle of event source
              wtype,                  // event type
              0,                      // event category
              0,                      // event ID
              NULL,                   // current user's SID
              1,                      // strings in 'strings'
              0,                      // no bytes of raw data
              (const char **)strings, // array of error strings
              NULL);                  // no raw data

      DeregisterEventSource(heventsrc);
   }
}

/* Convert Win32 baud constants to POSIX constants */
int posixbaud(DWORD baud)
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

/* Convert POSIX baud constants to Win32 constants */
DWORD winbaud(int baud)
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

/* Convert Win32 bytesize constants to POSIX constants */
int posixsize(BYTE size)
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

/* Convert POSIX bytesize constants to Win32 constants */
BYTE winsize(int size)
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

int tiocmbic(int fd, int bits)
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

int tiocmbis(int fd, int bits)
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

int tiocmget(int fd, int *bits)
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

// Parse windows-style command line into individual arguments
char *GetArg(char **cmdline)
{
   // Skip leading whitespace
   while (isspace(**cmdline))
      (*cmdline)++;

   // Bail if there's nothing left
   if (**cmdline == '\0')
      return NULL;

   // Find end of this argument
   char *ret;
   if (**cmdline == '"') {
      // Find end of quoted argument
      ret = ++(*cmdline);
      while (**cmdline && **cmdline != '"')
         (*cmdline)++;
   } else {
      // Find end of non-quoted argument
      ret = *cmdline;
      while (**cmdline && !isspace(**cmdline))
         (*cmdline)++;
   }

   // NUL-terminate this argument
   if (**cmdline)
      *(*cmdline)++ = '\0';

   return ret;
}
