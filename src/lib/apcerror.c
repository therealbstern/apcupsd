/*
 * apcerror.c
 *
 * Error functions.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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

/*
 * Wraps call thru error_out function pointer so we can model it as no-return
 * in Coverity to clean up false positives.
 */
void error_out_wrapper(const char *file, int line, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   error_out(file, line, fmt, args);
   // Never get here...
   va_end(args);
   while(1)
      ;
}

/*
 * Subroutine normally called by macro error_abort() to print
 * FATAL ERROR message and supplied error message
 */
void generic_error_out(const char *file, int line, const char *fmt, va_list arg_ptr)
{
   char buf[256];
   int i;

   asnprintf(buf, sizeof(buf), "FATAL ERROR in %s at line %d\n", file, line);
   i = strlen(buf);
   avsnprintf((char *)&buf[i], sizeof(buf) - i, (char *)fmt, arg_ptr);
   fprintf(stdout, "%s", buf);

   if (error_cleanup)
      error_cleanup();

   exit(1);
}

void (*error_out) (const char *file, int line, const char *fmt, va_list arg_ptr) = generic_error_out;
void (*error_cleanup) (void) = NULL;
