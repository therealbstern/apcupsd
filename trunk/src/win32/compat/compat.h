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
 * $Id: compat.h,v 1.3 2006-03-12 07:54:47 kerns Exp $
 */


#ifndef __COMPAT_H_
#define __COMPAT_H_

#ifndef HAVE_WIN32
#define HAVE_WIN32 1
#endif

#if (defined _MSC_VER) && (_MSC_VER >= 1400) // VC8+
#pragma warning(disable : 4996) // Either disable all deprecation warnings,
// #define _CRT_SECURE_NO_DEPRECATE // Or just turn off warnings about the newly deprecated CRT functions.
#define HAVE_VC8
#endif // VC8+

#if (!defined HAVE_MINGW) && (!defined HAVE_VC8) && (!defined HAVE_WXCONSOLE)
#define __STDC__ 1
#endif

#include <stdio.h>
#include <basetsd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <process.h>
#include <direct.h>
#include <winsock2.h>
#include <windows.h>
#include <wincon.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <utime.h>
#include <sys/time.h>

#if defined HAVE_MINGW
#include <stdint.h>
#include <sys/stat.h>
#endif

#include "getopt.h"

#define HAVE_WIN32 1

#ifndef HAVE_MINGW
#ifdef HAVE_CYGWIN
#undef HAVE_CYGWIN
#else
#endif //HAVE_CYGWIN
#endif //HAVE_MINGW

typedef UINT64 u_int64_t;
typedef UINT64 uint64_t;
typedef INT64 int64_t;
typedef UINT32 uint32_t;
typedef long int32_t;
typedef INT64 intmax_t;
typedef unsigned char uint8_t;
typedef float float32_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef signed char int8_t;

#ifndef HAVE_VC8
typedef long time_t;
#endif

#if __STDC__
#ifndef HAVE_MINGW
typedef _dev_t dev_t;
#ifndef HAVE_WXCONSOLE
typedef __int64 ino_t;
typedef __int64 off_t;          /* STDC=1 means we can define this */
#endif
#endif
#else
typedef long _off_t;            /* must be same as sys/types.h */
#endif

#ifndef HAVE_MINGW
#ifndef HAVE_WXCONSOLE
typedef int BOOL;
#define bool BOOL
#endif
#endif

typedef double float64_t;
typedef UINT32 u_int32_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;

#ifndef HAVE_MINGW
#undef uint32_t
#endif

void sleep(int);

typedef UINT32 key_t;

#ifdef HAVE_MINGW
#ifndef uid_t
typedef UINT32 uid_t;
typedef UINT32 gid_t;
#endif
#else
typedef UINT32 uid_t;
typedef UINT32 gid_t;
typedef UINT32 mode_t;
/* #ifndef _WX_DEFS_H_  ssize_t is defined in wx/defs.h */
typedef INT64  ssize_t;
/* #endif */
#endif //HAVE_MINGW

struct dirent {
    uint64_t    d_ino;
    uint32_t    d_off;
    uint16_t    d_reclen;
    char        d_name[256];
};

typedef void DIR;

#ifndef __cplusplus
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

struct timezone {
    int foo;
};

int strcasecmp(const char*, const char *);
int strncasecmp(const char*, const char *, int);
int gettimeofday(struct timeval *, struct timezone *);

#define ETIMEDOUT 55

#ifndef HAVE_MINGW

#ifndef _STAT_DEFINED
struct stat
{
    _dev_t      st_dev;
    uint64_t    st_ino;
    uint16_t    st_mode;
    int16_t     st_nlink;
    uint32_t    st_uid;
    uint32_t    st_gid;
    _dev_t      st_rdev;
    uint64_t    st_size;
    time_t      st_atime;
    time_t      st_mtime;
    time_t      st_ctime;
    uint32_t    st_blksize;
    uint64_t    st_blocks;
};
#endif

#undef  S_IFMT
#define S_IFMT         0170000         /* file type mask */
#undef  S_IFDIR
#define S_IFDIR        0040000         /* directory */
#define S_IFCHR        0020000         /* character special */
#define S_IFBLK        0060000         /* block special */
#define S_IFIFO        0010000         /* pipe */
#undef  S_IFREG
#define S_IFREG        0100000         /* regular */
#define S_IREAD        0000400         /* read permission, owner */
#define S_IWRITE       0000200         /* write permission, owner */
#define S_IEXEC        0000100         /* execute/search permission, owner */

#define S_IRUSR         S_IREAD
#define S_IWUSR         S_IWRITE
#define S_IXUSR         S_IEXEC
#define S_ISREG(x)  (((x) & S_IFMT) == S_IFREG)
#define S_ISDIR(x)  (((x) & S_IFMT) == S_IFDIR)
#define S_ISCHR(x) 0
#define S_ISBLK(x)  (((x) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(x) 0
#endif //HAVE_MINGW

#define S_IRGRP         000040
#define S_IWGRP         000020
#define S_IXGRP         000010

#define S_IROTH         00004
#define S_IWOTH         00002
#define S_IXOTH         00001

#define S_IRWXO         000007
#define S_IRWXG         000070
#define S_ISUID         004000
#define S_ISGID         002000
#define S_ISVTX         001000
#define S_ISSOCK(x) 0
#define S_ISLNK(x)      0

#if __STDC__
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define O_EXCL   _O_EXCL

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym
#endif

#ifndef HAVE_VC8
int umask(int);
off_t lseek(int, off_t, int);
int dup2(int, int);
int close(int fd);
#ifndef HAVE_WXCONSOLE
ssize_t read(int fd, void *, ssize_t nbytes);
ssize_t write(int fd, const void *, ssize_t nbytes);
#endif
#endif
int lchown(const char *, uid_t uid, gid_t gid);
int chown(const char *, uid_t uid, gid_t gid);
int chmod(const char *, mode_t mode);
int inet_aton(const char *cp, struct in_addr *inp);
int kill(int pid, int signo);
int pipe(int []);
int fork();
int waitpid(int, int *, int);

#ifndef HAVE_MINGW
int utime(const char *filename, struct utimbuf *buf);
int open(const char *, int, int);
int open(const char *, int);
#define vsnprintf __vsnprintf
int __vsnprintf(char *s, size_t count, const char *format, va_list args);

#define vsprintf __vsprintf
int __vsprintf(char *s, const char *format, va_list args);

#define snprintf __snprintf
int __snprintf(char *str, size_t count, const char *fmt, ...);

#define sprintf __sprintf
int __sprintf(char *str, const char *fmt, ...);

#endif //HAVE_MINGW


#define WNOHANG 0
#define WIFEXITED(x) 0
#define WEXITSTATUS(x) x
#define WIFSIGNALED(x) 0
#define SIGKILL 9
#define SIGUSR2 9999

#define HAVE_OLD_SOCKOPT

int readdir(unsigned int fd, struct dirent *dirp, unsigned int count);
int nanosleep(const struct timespec*, struct timespec *);
struct tm *localtime_r(const time_t *, struct tm *);
struct tm *gmtime_r(const time_t *, struct tm *);
long int random(void);
void srandom(unsigned int seed);
int lstat(const char *, struct stat *);
long pathconf(const char *, int);
int readlink(const char *, char *, int);
#define _PC_PATH_MAX 1
#define _PC_NAME_MAX 2



int geteuid();

DIR *opendir(const char *name);
int closedir(DIR *dir);

struct passwd {
    char *foo;
};

struct group {
    char *foo;
};

struct passwd *getpwuid(uid_t);
struct group *getgrgid(uid_t);

#ifndef HAVE_MINGW
#define R_OK 04
#define W_OK 02
#endif //HAVE_MINGW

struct sigaction {
    int sa_flags;
    void (*sa_handler)(int);
};
#define sigfillset(x)
#define sigaction(a, b, c)

#define mkdir(p, m) win32_mkdir(p)
#define unlink win32_unlink
#define chdir win32_chdir

#ifndef HAVE_MINGW
int stat(const char *, struct stat *);
int ftruncate(int, off_t);
int _fstat(int, struct stat *);
#ifdef __cplusplus
#define access _access
extern "C" _CRTIMP int __cdecl _access(const char *, int);
int execvp(const char *, char *[]);
extern "C" void *  __cdecl _alloca(size_t);
#endif
#endif //HAVE_MINGW

#define getpid _getpid
#define fstat  _fstat

#define getppid() 0
#define gethostid() 0
#define getuid() 0
#define getgid() 0

#define getcwd win32_getcwd
#define chdir win32_chdir
#define fputs win32_fputs
char *win32_getcwd(char *buf, int maxlen);
int win32_chdir(const char *buf);
int win32_mkdir(const char *buf);
int win32_fputs(const char *string, FILE *stream);
int win32_unlink(const char *filename);

char* win32_cgets (char* buffer, int len);


int WSA_Init(void);

#ifdef HAVE_MINGW
void closelog();
#endif //HAVE_MINGW

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/* Kludges to make compat.cpp work in apcupsd */
#ifdef _APCUPSD
#define POOLMEM char
#define get_pool_memory(x) (char *)malloc(1000)
#define free_pool_memory(x) free((char *)x)
#define check_pool_memory_size(x, s) x
#define ASSERT(x)
#define bstrncat astrncat
#define bstrncpy astrncpy
#define bmicrosleep amicrosleep
#define b_errno_exit (1<<28)
#define INT32_MAX       (2147483647)
/* ***FIXME*** */
#define start_child_timer(x, y) (btimer_t *)1
#define stop_child_timer(x)
#endif

#ifndef O_APPEND
#define O_APPEND _O_APPEND
#endif

#endif /* __COMPAT_H_ */
