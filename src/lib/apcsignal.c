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

/* Pickup then ignore SIGCHLD */
void child_handler(int sig)
{
#if !defined(HAVE_AIX_OS) && !defined(HAVE_SGI_OS)
   /*
    * This will cause an infinite loop on AIX and supposedly on 
    * Irix as well.
    */
   signal(SIGCHLD, child_handler);
#endif
}

/*********************************************************************/
void init_timer(int timer, void (*fnhandler) (int))
{
   signal(SIGALRM, fnhandler);
   alarm(timer);
}

/*********************************************************************/
void init_thread_signals(void)
{
#ifndef HAVE_PTHREADS              /* only done once for real threads */
   /* Set up signals. */
   signal(SIGHUP, apc_thread_terminate);
   signal(SIGINT, apc_thread_terminate);
   signal(SIGTERM, apc_thread_terminate);
   signal(SIGPIPE, SIG_IGN);

   /* Children reaped by waitpid() */
   signal(SIGCHLD, child_handler);

   /* I think this is not effective -RF */
   signal(SIGKILL, apc_thread_terminate);
#endif   /* !HAVE_PTHREADS */
}

/*********************************************************************/
void init_signals(void (*handler) (int))
{
   /* Set up signals. */
   signal(SIGHUP, handler);
   signal(SIGINT, handler);
   signal(SIGTERM, handler);

   /* Picked up via wait */
#ifndef HAVE_CYGWIN
   signal(SIGCHLD, child_handler);
#endif

   signal(SIGPIPE, SIG_IGN);

   /* I think this is not effective -RF */
   signal(SIGKILL, handler);
}

/*********************************************************************/
void restore_signals(void)
{
   signal(SIGALRM, SIG_DFL);
   signal(SIGHUP, SIG_DFL);
   signal(SIGINT, SIG_DFL);
   signal(SIGTERM, SIG_DFL);
   signal(SIGCHLD, SIG_DFL);
   signal(SIGKILL, SIG_DFL);
}

/*********************************************************************/
void sleep_forever(void)
{
   /* Hugly !!! */
   for (;;)
      sleep(1000);
}
