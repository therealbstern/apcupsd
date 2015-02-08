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

#include <signal.h>
#include <errno.h>
#include "winapi.h"

int
kill(int pid, int signal)
{
   int rval = 0;
   DWORD exitcode = 0;

   switch (signal) {
   case SIGTERM:
      /* Terminate the process */
      if (!TerminateProcess((HANDLE)pid, (UINT) signal))
      {
         rval = -1;
         errno = EPERM;
      }
      CloseHandle((HANDLE)pid);
      break;
   case 0:
      /* Just check if process is still alive */
      if (GetExitCodeProcess((HANDLE)pid, &exitcode) &&
          exitcode != STILL_ACTIVE)
      {
         rval = -1;
         errno = ESRCH;
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
