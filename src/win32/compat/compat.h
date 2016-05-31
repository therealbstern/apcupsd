/*                               -*- Mode: C -*-
 * compat.h --
 */
// Copyright transferred from Raider Solutions, Inc to
//   Kern Sibbald and John Walker by express permission.
//
// Copyright (C) 2004-2006 Kern Sibbald
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free
//   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//   MA 02110-1335, USA.
/*
 *
 * Author          : Christopher S. Hull
 * Created On      : Fri Jan 30 13:00:51 2004
 * Last Modified By: Thorsten Engel
 * Last Modified On: Fri Apr 22 19:30:00 2004
 * Update Count    : 218
 * $Id: compat.h,v 1.21.2.4 2009-08-01 12:01:59 adk0212 Exp $
 */


#ifndef __COMPAT_H_
#define __COMPAT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Parse windows-style command line into individual arguments
char *GetArg(char **cmdline);

#ifdef __cplusplus
};
#endif

#endif /* __COMPAT_H_ */
