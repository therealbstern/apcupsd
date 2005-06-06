/*
 * apclog.c
 *
 * Logging functions.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 2000-2005 Kern Sibbald <kern@sibbald.com>
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

void log_event(const UPSINFO *ups, int level, const char *fmt, ...)
{
   va_list arg_ptr;
   char msg[2 *MAXSTRING];
   char datetime[MAXSTRING];
   int event_fd;

   event_fd = ups ? ups->event_fd : -1;

   va_start(arg_ptr, fmt);
   avsnprintf(msg, sizeof(msg), fmt, arg_ptr);
   va_end(arg_ptr);

   syslog(level, "%s", msg);       /* log the event */
   Dmsg1(100, "%s\n", msg);

   /* Write out to our temp file. LOG_INFO is DATA logging, so
    * do not write it to our temp events file. */
   if (event_fd >= 0 && level != LOG_INFO) {
      int lm;
      time_t nowtime;
      struct tm tm;

      time(&nowtime);
      localtime_r(&nowtime, &tm);
      strftime(datetime, 100, "%a %b %d %X %Z %Y  ", &tm);
      write(event_fd, datetime, strlen(datetime));

      lm = strlen(msg);
      if (msg[lm - 1] != '\n')
         msg[lm++] = '\n';

      write(event_fd, msg, lm);
   }
}

/*
 * Subroutine prints a debug message if the level number
 * is less than or equal the debug_level. File and line numbers
 * are included for more detail if desired, but not currently
 * printed.
 *  
 * If the level is negative, the details of file and line number
 * are not printed.
 */

int debug_level = 0;
static FILE *trace_fd = NULL;
bool trace = false;

#define FULL_LOCATION 1
void d_msg(const char *file, int line, int level, const char *fmt, ...)
{
#ifdef DEBUG
   char buf[4096];
   int i;
   va_list arg_ptr;
   bool details = true;
   char *my_name = "apcupsd";

   if (level < 0) {
      details = false;
      level = -level;
   }

   if (level <= debug_level) {
#ifdef FULL_LOCATION
      if (details) {
         asnprintf(buf, sizeof(buf), "%s: %s:%d ", my_name, file, line);
         i = strlen(buf);
      } else {
         i = 0;
      }
#else
      i = 0;
#endif
      va_start(arg_ptr, fmt);
      avsnprintf(buf + i, sizeof(buf) - i, (char *)fmt, arg_ptr);
      va_end(arg_ptr);

       if (trace) {
          if (!trace_fd) {
             char fn[200];
             asnprintf(fn, sizeof(fn), "./apcupsd.trace");
             trace_fd = fopen(fn, "a+");
          }
          if (trace_fd) {
             fputs(buf, trace_fd);
             fflush(trace_fd);
          } else {
             /* Some problem, turn off tracing */
             trace = false;
          }
       } else {   /* not tracing */
          fputs(buf, stdout);
          fflush(stdout);
       }
   }
#endif
}
