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

// from MicroSoft SDK (KES) is the diff between Jan 1 1601 and Jan 1 1970
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000ULL 
#define WIN32_FILETIME_SCALE  10000000             // 100ns/second

void conv_unix_to_win32_path(const char *name, char *win32_name, DWORD dwSize)
{
    const char *fname = name;
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

int
WSA_Init(void)
{
    WORD wVersionRequested = MAKEWORD( 1, 1);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);


    if (err != 0) {
        printf("Can not start Windows Sockets\n");
        errno = ENOSYS;
        return -1;
    }

    return 0;
}
