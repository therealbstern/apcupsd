/*
 *  apcsignal.c -- signal() file managing functions
 *
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS "APCUPSD-Team" BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE "APCUPSD-Team" HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE "APCUPSD-Team" SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN "AS IS" BASIS, AND THE "APCUPSD-Team" HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE "APCUPSD-Team" HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, "APCC".  THE "APCUPSD-Team" DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH "APCC".  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */

/*
 *  Contributed by Facchetti Riccardo <riccardo@master.oasi.gpa.it>
 */

#include "apc.h"

/*
 * Pickup then ignore SIGCHLD
 */
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
void init_timer(int timer, void (*fnhandler)(int))
{
    signal(SIGALRM, fnhandler);
    alarm(timer);
}

/*********************************************************************/
void init_thread_signals(void)
{
#ifndef HAVE_PTHREADS  /* only done once for real threads */
    /*
     * Set up signals.
     */
    signal(SIGHUP, apc_thread_terminate);
    signal(SIGINT, apc_thread_terminate);
    signal(SIGTERM, apc_thread_terminate);
    signal(SIGPIPE, SIG_IGN);

    /* Children reaped by waitpid() */
    signal(SIGCHLD, child_handler);

    /*
     * I think this is not effective
     * -RF
     */
    signal(SIGKILL, apc_thread_terminate);
#endif /* !HAVE_PTHREADS */
}

/*********************************************************************/
void init_signals(void (*handler)(int))
{
    /*
     * Set up signals.
     */
    signal(SIGHUP, handler);
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    /* Picked up via wait */
#ifndef HAVE_CYGWIN
    signal(SIGCHLD, child_handler);
#endif

    signal(SIGPIPE, SIG_IGN);

    /*
     * I think this is not effective
     * -RF
     */
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
    /*
     * Hugly !!!
     */
    for (;;)
	sleep(1000);
}
