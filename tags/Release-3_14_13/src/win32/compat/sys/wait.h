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

#ifndef __WAIT_H_
#define __WAIT_H_

#define WNOHANG 0
#define WIFEXITED(x) 0
#define WEXITSTATUS(x) x
#define WIFSIGNALED(x) 0

#ifdef __cplusplus
extern "C" {
#endif

int waitpid(int, int *, int);

#ifdef __cplusplus
};
#endif

#endif /* __WAIT_H_ */
