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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef __COMPAT_FCNTL_H_
#define __COMPAT_FCNTL_H_

#include_next <fcntl.h>

#define O_NOCTTY 0

#ifndef F_GETFL
# define F_GETFL 1
#endif
#ifndef F_SETFL
# define F_SETFL 2
#endif

#ifdef __cplusplus
extern "C" {
#endif

int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
};
#endif

#endif /* __COMPAT_FCNTL_H_ */
