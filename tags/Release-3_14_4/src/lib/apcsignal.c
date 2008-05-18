/*
 * apcsignal.c
 *
 * signal() managing functions
 */

/*
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

/*********************************************************************/
void init_timer(int timer, void (*fnhandler) (int))
{
   signal(SIGALRM, fnhandler);
   alarm(timer);
}

/*********************************************************************/
#ifndef HAVE_MINGW
void init_signals(void (*handler) (int))
{
   /* Set up signals. */
   signal(SIGHUP, handler);
   signal(SIGINT, handler);
   signal(SIGTERM, handler);

   /* Picked up via wait */
   signal(SIGPIPE, SIG_IGN);
}
#endif

/*********************************************************************/
void restore_signals(void)
{
#ifndef HAVE_MINGW
   signal(SIGALRM, SIG_DFL);
   signal(SIGHUP, SIG_DFL);
   signal(SIGINT, SIG_DFL);
   signal(SIGTERM, SIG_DFL);
   signal(SIGCHLD, SIG_DFL);
   signal(SIGKILL, SIG_DFL);
#endif
}

/*********************************************************************/
void sleep_forever(void)
{
   /* Hugly !!! */
   for (;;)
      sleep(1000);
}
