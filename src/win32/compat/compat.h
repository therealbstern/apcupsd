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
//   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//   MA 02111-1307, USA.
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

#include <stdint.h>
#include <stdio.h>
#include <basetsd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <process.h>
#include <direct.h>
#include <wchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincon.h>
#include <winbase.h>
#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#include <process.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <malloc.h>
#include <setjmp.h>
#include <direct.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <lmcons.h>
#include <dirent.h>
#include <winapi.h>
#include <sys/stat.h>

typedef long time_t;

#if !__STDC__
typedef long _off_t;            /* must be same as sys/types.h */
#endif

#ifndef __cplusplus
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT 55
#endif

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif

#if __STDC__
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define O_NOCTTY 0

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym
#endif

#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
// Sockets
//******************************************************************************
#define WNOHANG 0
#define WIFEXITED(x) 0
#define WEXITSTATUS(x) x
#define WIFSIGNALED(x) 0
#define HAVE_OLD_SOCKOPT
int WSA_Init(void);
int inet_aton(const char *cp, struct in_addr *inp);

//******************************************************************************
// Time
//******************************************************************************
struct timespec;
void sleep(int);
struct timezone;
#define alarm(a) 0

//******************************************************************************
// User/Password
//******************************************************************************
int geteuid();

#ifndef uid_t
typedef UINT32 uid_t;
typedef UINT32 gid_t;
#endif

struct passwd {
    char *pw_name;
};

struct group {
    char *foo;
};

#define getpwuid(x) NULL
#define getgrgid(x) NULL
#define getuid() 0
#define getgid() 0

//******************************************************************************
// File/Path
//******************************************************************************
int readlink(const char *, char *, int);
int lchown(const char *, uid_t uid, gid_t gid);
int chown(const char *, uid_t uid, gid_t gid);

#ifndef F_GETFL
# define F_GETFL 1
#endif
#ifndef F_SETFL
# define F_SETFL 2
#endif
int fcntl(int fd, int cmd, ...);

#define _PC_PATH_MAX 1
#define _PC_NAME_MAX 2
long pathconf(const char *, int);

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

//******************************************************************************
// Signals
//******************************************************************************
struct sigaction {
    int sa_flags;
    void (*sa_handler)(int);
};
#define sigfillset(x)
#define sigaction(a, b, c)
int kill(int pid, int signo);

#define SIGKILL 9
#define SIGUSR2 9999
#define SIGCHLD 0
#define SIGALRM 0
#define SIGHUP 0
#define SIGCHLD 0
#define SIGPIPE 0

//******************************************************************************
// Process
//******************************************************************************
#define getpid _getpid
#define getppid() 0
#define gethostid() 0
int fork();
int waitpid(int, int *, int);

//******************************************************************************
// Misc
//******************************************************************************
long int random(void);
void srandom(unsigned int seed);

/* Should use strtok_s but mingw doesn't have it. strtok is thread-safe on
 * Windows via TLS, so this substitution should be ok... */
#define strtok_r(a,b,c) strtok(a,b)

/* Return the smaller of a or b */
#ifndef MIN
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif

// Parse windows-style command line into individual arguments
char *GetArg(char **cmdline);

#ifdef __cplusplus
};
#endif

#endif /* __COMPAT_H_ */
