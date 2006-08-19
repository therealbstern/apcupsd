/*                               -*- Mode: C -*- 
 * compat.h -- 
// Copyright (C)2004 Raider Solutions, Inc.
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
 * 
 * 
 * Author          : Christopher S. Hull
 * Created On      : Fri Jan 30 13:00:51 2004
 * Last Modified By: Christopher S. Hull
 * Last Modified On: Tue Feb 24 11:13:32 2004
 * Update Count    : 218
 * $Id: compat.h,v 1.1 2004-04-16 13:18:45 kerns Exp $
 */


#ifndef __COMPAT_H_
#define __COMPAT_H_

#define __STDC__ 1

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


#include "getopt.h"

#ifdef HAVE_CYGWIN
#error should not be used under cygwin...
#else
#define HAVE_WIN32 1
#endif
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
typedef long time_t;
#if __STDC__
typedef _dev_t dev_t;
typedef __int64 ino_t;
typedef __int64 off_t;		/* STDC=1 means we can define this */
#else
typedef long _off_t;		/* must be same as sys/types.h */
#endif
typedef signed char int8_t;
typedef int BOOL;
#define bool BOOL
typedef double float64_t;
typedef UINT32 u_int32_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;

#undef uint32_t

void sleep(int);

typedef UINT32 key_t;
typedef UINT32 uid_t;
typedef UINT32 gid_t;
typedef UINT32 mode_t;
typedef INT64  ssize_t;


typedef void DIR;


struct dirent {
    uint64_t    d_ino;
    uint32_t    d_off;
    uint16_t    d_reclen;
    char        d_name[256];
};

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

#define S_IFMT         0170000         /* file type mask */
#define S_IFDIR        0040000         /* directory */
#define S_IFCHR        0020000         /* character special */
#define S_IFIFO        0010000         /* pipe */
#define S_IFREG        0100000         /* regular */
#define S_IREAD        0000400         /* read permission, owner */
#define S_IWRITE       0000200         /* write permission, owner */
#define S_IEXEC        0000100         /* execute/search permission, owner */

#define S_IRUSR         S_IREAD
#define S_IWUSR         S_IWRITE
#define S_IXUSR         S_IEXEC

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


#define S_ISREG(x)  (((x) & S_IFREG) == S_IFREG)
#define S_ISDIR(x)  (((x) & S_IFDIR) == S_IFDIR)
#define S_ISCHR(x) 0
#define S_ISBLK(x) 0
#define S_ISFIFO(x) 0
#define S_ISSOCK(x) 0
#define S_ISLNK(x)      0


#if __STDC__
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym

#endif
#define SIGUSR2 9999

int umask(int);
int lchown(const char *, uid_t uid, gid_t gid);
int chown(const char *, uid_t uid, gid_t gid);
int chmod(const char *, mode_t mode);
int utime(const char *filename, struct utimbuf *buf);
int open(const char *, int, int);
off_t lseek(int, off_t, int);
ssize_t read(int fd, void *, size_t nbytes);
ssize_t write(int fd, const void *, size_t nbytes);
int close(int fd);
int inet_aton(const char *cp, struct in_addr *inp);
int kill(int pid, int signo);
int pipe(int []);
int fork();
int dup2(int, int);
int waitpid(int, int *, int);
#define WNOHANG 0
#define WIFEXITED(x) 0
#define WEXITSTATUS(x) x
#define WIFSIGNALED(x) 0

#define SIGKILL 9

#define HAVE_OLD_SOCKOPT


#define vsnprintf __vsnprintf
int __vsnprintf(char *s, size_t count, const char *format, va_list args);

#define vsprintf __vsprintf
int __vsprintf(char *s, const char *format, va_list args);

#define snprintf __snprintf
int __snprintf(char *str, size_t count, const char *fmt, ...);

#define sprintf __sprintf
int __sprintf(char *str, const char *fmt, ...);


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

#define R_OK 04
#define W_OK 02


struct sigaction {
    int sa_flags;
    void (*sa_handler)(int);
};
#define sigfillset(x)
#define sigaction(a, b, c)

#define mkdir(p, m) _mkdir(p)
#define chdir win32_chdir

int stat(const char *, struct stat *);
int syslog(int, const char *, const char *);
#define LOG_DAEMON 0
#define LOG_ERR 0

#ifdef __cplusplus
#define access _access
extern "C" _CRTIMP int __cdecl _access(const char *, int);
int execvp(const char *, char *[]);
extern "C" void *  __cdecl _alloca(size_t);
#endif

#define getpid _getpid

#define getppid() 0
#define gethostid() 0
#define getuid() 0
#define getgid() 0

#define getcwd win32_getcwd
#define chdir win32_chdir
char *win32_getcwd(char *buf, int maxlen);
int win32_chdir(const char *buf);

int WSA_Init(void);

#endif /* __COMPAT_H_ */
