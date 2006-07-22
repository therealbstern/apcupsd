/*
 * apc_config.h
 *
 * Platform compatibility cruft.
 *
 * Contributed by Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 * Modify it accordingly to configure.in file. Do _not_ use it for
 * apcupsd-specific things, just for configure.
 */

/*
 *  Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  Copyright (C) 1996-1999 Andre M. Hedrick <andre@suse.com>
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

#ifndef _APC_CONFIG_H
#define _APC_CONFIG_H

#ifndef HAVE_STRFTIME
# define strftime(msg, max, format, tm) \
   strncpy(msg, "time not available", max)
#endif   /* !HAVE_STRFTIME */

/* 
 * wait macros.
 */
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifndef HAVE_SOCKLEN_T
# define socklen_t unsigned int
#endif

/* Alpha Tru64 */
#ifdef HAVE_OSF1_OS
# define int32_t int
# define _SEM_SEMUN_UNDEFINED 1
#endif

#ifdef HAVE_HPUX_OS
# define _SEM_SEMUN_UNDEFINED 1

/*
 * These defines, related to serial devices, need to be clarified for HP-UX
 * operating system.
 */
# define TIOCM_LE 0
# define TIOCM_ST 0
# define TIOCM_SR 0
#endif

/* NetBSD */
#ifdef HAVE_NETBSD_OS
# define _SEM_SEMUN_UNDEFINED 1
#endif

#ifdef HAVE_SUN_OS
# define _SEM_SEMUN_UNDEFINED 1
#endif

#ifdef HAVE_AIX_OS
# define _SEM_SEMUN_UNDEFINED 1
#endif

#ifdef HAVE_OPENSERVER_OS
# define int32_t         int
# define _SEM_SEMUN_UNDEFINED 1
#endif

#ifndef SHUT_RDWR
# define SHUT_RDWR 2                /* for socket shutdown() calls */
#endif

#ifdef SETPGRP_VOID
# define SETPGRP_ARGS(x, y)        /* No arguments */
#else
# define SETPGRP_ARGS(x, y) x,y
#endif

/*
 * Special "kludges" for the win32 version of
 * apcupsd.
 */
#ifdef HAVE_CYGWIN

/* Apparently later version of Cygwin have this */
#define ioctl(fd, func, addr) winioctl(fd, func, addr)

/* modem ioctls */
#undef  TIOCMGET
#define TIOCMGET ('d'<<8 | 1)
#undef  TIOCMBIS
#define TIOCMBIS ('d'<<8 | 2)
#undef  TIOCMBIC
#define TIOCMBIC ('d'<<8 | 3)

/* modem lines */
#undef  TIOCM_LE
#undef  TIOCM_DTR
#undef  TIOCM_RTS
#undef  TIOCM_ST
#undef  TIOCM_SR
#undef  TIOCM_CTS
#undef  TIOCM_CAR
#undef  TIOCM_RNG
#undef  TIOCM_DSR
#undef  TIOCM_CD
#undef  TIOCM_RI

#define TIOCM_LE        0x001
#define TIOCM_DTR       0x002
#define TIOCM_RTS       0x004
#define TIOCM_ST        0x008
#define TIOCM_SR        0x010
#define TIOCM_CTS       0x020
#define TIOCM_CAR       0x040
#define TIOCM_RNG       0x080
#define TIOCM_DSR       0x100
#define TIOCM_CD        TIOCM_CAR
#define TIOCM_RI        TIOCM_RNG

#undef HAVE_NAMESER_H

#undef SIGTSTP
#undef SIGTTOU
#undef SIGTTIN
#undef SIGSTP


#define SHM_RDONLY 1

/* no setproctitle on win32 */
#define HAVE_SETPROCTITLE
#define setproctitle(x)
#define init_proctitle(x)

/*
 * Pretend that we are always root --
 * probably should fix this on NT for administrator
 */
#define getuid() 0
#define geteuid() 0


/*
 * ApcupsdMain is called from win32/winmain.cpp
 * we need to eliminate "main" as an entry point,
 * otherwise, it interferes with the Windows
 * startup.
 */
#define main ApcupsdMain

#endif   /* HAVE_CYGWIN */

#ifdef HAVE_MINGW
/*
 * ApcupsdMain is called from win32/winmain.cpp
 * we need to eliminate "main" as an entry point,
 * otherwise, it interferes with the Windows
 * startup.
 */
#define main ApcupsdMain

#endif

/* If we have it, the init is not needed */
#ifdef HAVE_SETPROCTITLE
# undef init_proctitle
# define init_proctitle(x)
#endif

#ifndef ENABLE_NLS
# define textdomain(x)
#endif

#ifndef O_NDELAY
# define O_NDELAY 0
#endif

/*
 * For HP-UX the definition of FILENAME_MAX seems not conformant with
 * POSIX standard. To avoid any problem we are forced to define a
 * private macro. This accounts also for other possible problematic OSes.
 * If none of the standard macros is defined, fall back to 256.
 */
#if defined(FILENAME_MAX) && FILENAME_MAX > 255
# define APC_FILENAME_MAX FILENAME_MAX
#elif defined(PATH_MAX) && PATH_MAX > 255
# define APC_FILENAME_MAX PATH_MAX
#elif defined(MAXPATHLEN) && MAXPATHLEN > 255
# define APC_FILENAME_MAX MAXPATHLEN
#else
# define APC_FILENAME_MAX 256
#endif

/* ETIME not on BSD, incl. Darwin */
#ifndef ETIME
# define ETIME ETIMEDOUT
#endif

/*
 * apcupsd requires SHM_RDONLY even if compiling for pthreads: the
 * attach_ipc interface is a wrapper for both.
 */
#if !defined(HAVE_SYS_SHM_H) && !defined SHM_RDONLY
# define SHM_RDONLY O_RDONLY
#endif

/* If no system localtime_r(), forward declaration of our internal substitute. */
#if !defined(HAVE_LOCALTIME_R)
extern struct tm *localtime_r(const time_t *timep, struct tm *tm);
#endif

/* If no system inet_pton(), forward declaration of our internal substitute. */
#if !defined(HAVE_INETPTON)

/* Define constants based on RFC 883, RFC 1034, RFC 1035 */
#define NS_PACKETSZ     512        /* maximum packet size */
#define NS_MAXDNAME     1025       /* maximum domain name */
#define NS_MAXCDNAME    255        /* maximum compressed domain name */
#define NS_MAXLABEL     63         /* maximum length of domain label */
#define NS_HFIXEDSZ     12         /* #/bytes of fixed data in header */
#define NS_QFIXEDSZ     4          /* #/bytes of fixed data in query */
#define NS_RRFIXEDSZ    10         /* #/bytes of fixed data in r record */
#define NS_INT32SZ      4          /* #/bytes of data in a u_int32_t */
#define NS_INT16SZ      2          /* #/bytes of data in a u_int16_t */
#define NS_INT8SZ       1          /* #/bytes of data in a u_int8_t */
#define NS_INADDRSZ     4          /* IPv4 T_A */
#define NS_IN6ADDRSZ    16         /* IPv6 T_AAAA */
#define NS_CMPRSFLGS    0xc0       /* Flag bits indicating name compression. */
#define NS_DEFAULTPORT  53         /* For both TCP and UDP. */
extern int inet_pton(int af, const char *src, void *dst);
#endif

/* Determine if the C(++) compiler requires complete function prototype  */
#ifndef __P
# if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#  define __P(x) x
# else
#  define __P(x) ()
# endif
#endif

#ifndef __cplusplus
#define bool int
#define true 1
#define false 0
#endif

#endif   /* _APC_CONFIG */
