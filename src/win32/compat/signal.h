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

#ifndef __COMPAT_SIGNAL_H_
#define __COMPAT_SIGNAL_H_

/* Pull in mingw signal.h */
#include_next <signal.h>

struct sigaction {
    int sa_flags;
    void (*sa_handler)(int);
};

#define SIGKILL 9
#define SIGUSR2 9999
#define SIGALRM 0
#define SIGHUP 0
#define SIGCHLD 0
#define SIGPIPE 0

#ifdef __cplusplus
extern "C" {
#endif

#define sigfillset(x)
#define sigaction(a, b, c)
int kill(int pid, int signo);

#ifdef __cplusplus
};
#endif

#endif /* __COMPAT_SIGNAL_H_ */
