/*
 * proctitle.c
 *
 * Set process title.
 *
 * Stolen from sysvinit-2.75.
 */

/*
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/*
 *	Set the process title. We do not check for overflow of
 *	the stack space since we know there is plenty for
 * our needs and we'll never use more than 7 bytes anyway.
 */

#include "apc.h"

#undef init_proctitle
#undef setproctitle

#ifndef HAVE_CYGWIN

static char *argv0;
static int maxproclen;

void init_proctitle(char *a0)
{
   argv0 = a0;
   maxproclen = strlen(a0);
}

int setproctitle(char *fmt, ...)
{
   va_list ap;
   int len;
   char buf[256];

   buf[0] = 0;

   va_start(ap, fmt);
   len = avsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);

   memset(argv0, 0, maxproclen + 1);
   strncpy(argv0, buf, maxproclen);

   return len;
}

#endif   /* !HAVE_CYGWIN */
