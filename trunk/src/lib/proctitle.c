/*
 *  proctitle.c -- set process title.
 *
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
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
 *	Set the process title. We do not check for overflow of
 *	the stack space since we know there is plenty for
 *      our needs and we'll never use more than 7 bytes anyway.
 *
 *	Stolen from sysvinit-2.75.
 *
 *	Modified for apcupsd needs.
 *	-RF
 */

#include "apc.h"

#undef init_proctitle
#undef setproctitle

#ifndef HAVE_CYGWIN

static char *argv0;
static int maxproclen;

void init_proctitle(char *a0) {
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

#endif /* !HAVE_CYGWIN */
