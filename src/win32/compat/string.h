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

#ifndef __COMPAT_STRING_H_
#define __COMPAT_STRING_H_

#include_next <string.h>

/* Should use strtok_s but mingw doesn't have it. strtok is thread-safe on
 * Windows via TLS, so this substitution should be ok... */
#define strtok_r(a,b,c) strtok(a,b)

#endif /* __COMPAT_STRING_H_ */
