/*
 *  apc.h -- main header file for apcupsd package
 *
 *  Copyright (C) 1999 Brian Schau <Brian@Schau.dk>
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 */

/*
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *                           675 Mass Ave, Cambridge, MA 02139, USA
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
 *  Contributed by Brian Schau <Brian@Schau.dk>
 *
 *  Added in apcupsd-3.6.3
 */

#ifndef APC_H
#define APC_H 1

#include "config.h"

/* Note, on the Alpha, we must include stdarg to get
 * the GNU version before stdio or we get multiple
 * definitions.  This order could probably be used
 * on all systems, but is untested so I #ifdef it.
 * KES 9/2000
 */            
#ifdef HAVE_OSF1_OS
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
#else
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef HAVE_OPENSERVER_OS
# define _SVID3		/* OpenServer needs this to see TIOCM_ defn's */
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_GETOPTLONG
# include <getopt.h>
#else
# include "getopt.h"
#endif

#ifdef HAVE_PTHREADS

# define _THREAD_SAFE 1
# define _REENTRANT   1

# include <pthread.h>
# ifdef HAVE_SUN_OS
#  define set_thread_concurrency() thr_setconcurrency(4)
# else
#  define set_thread_concurrency()
# endif
/* setproctitle not used with pthreads */
# undef  HAVE_SETPROCTITLE
# define HAVE_SETPROCTITLE
# define init_proctitle(x)
# define setproctitle(x)
#endif

#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <limits.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <termios.h>
#include <netdb.h>
#include <sys/ioctl.h>

#ifdef HAVE_CYGWIN
# include <apc_winsem.h>
#else
# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
# ifdef HAVE_SYS_SEM_H
#  include <sys/sem.h>
# endif
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif
#endif

#include <sys/socket.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <sys/types.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_HPUX_OS
# include <sys/modem.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
           

/* Include apcupsd stuff */

#include "apc_config.h"

#include "apc_i18n.h"
#include "apc_version.h"
#include "apc_defines.h"
#include "apc_struct.h"
#include "apc_drivers.h"
#include "apc_extern.h"

#endif
