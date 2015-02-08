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

#include <syslog.h>
#include <stdio.h>
#include "winapi.h"

/* Implement syslog() using Win32 Event Service */
void openlog(const char *app, int, int) {}
void closelog(void) {}
void syslog(int type, const char *fmt, ...)
{
   va_list arg_ptr;
   char message[1024];
   HANDLE heventsrc;
   char* strings[32];
   WORD wtype;

   va_start(arg_ptr, fmt);
   message[0] = '\n';
   message[1] = '\n';
   vsnprintf(message+2, sizeof(message)-2, fmt, arg_ptr);
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
