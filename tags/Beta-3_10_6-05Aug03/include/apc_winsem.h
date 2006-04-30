/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CYGWIN

#ifndef _SYS_SEM_H
#define _SYS_SEM_H      1

typedef long __key_t;
typedef long __time_t;

#include <features.h>

#include <sys/types.h>

/* Get common definition of System V style IPC.  */
#include <apc_winipc.h>

/* Get system dependent definition of `struct semid_ds' and more.  */
/* #include <bits/sem.h> */
/* Flags for `semop'.  */
#define SEM_UNDO        0x1000          /* undo the operation on exit */

/* Commands for `semctl'.  */
#define GETPID          11              /* get sempid */
#define GETVAL          12              /* get semval */
#define GETALL          13              /* get all semval's */
#define GETNCNT         14              /* get semncnt */
#define GETZCNT         15              /* get semzcnt */
#define SETVAL          16              /* set semval */
#define SETALL          17              /* set all semval's */

/* Data structure describing a set of semaphores.  */
struct semid_ds
{
  struct ipc_perm sem_perm;             /* operation permission struct */
  __time_t sem_otime;                   /* last semop() time */
  __time_t sem_ctime;                   /* last time changed by semctl() */
  struct sem *__sembase;                /* ptr to first semaphore in array */
  struct sem_queue *__sem_pending;      /* pending operations */
  struct sem_queue *__sem_pending_last; /* last pending operation */
  struct sem_undo *__undo;              /* ondo requests on this array */
  unsigned short int sem_nsems;         /* number of semaphores in set */
};


/* The user should define a union like the following to use it for arguments
   for `semctl'. */

   union semun
   {
     int val;                        /* <= value for SETVAL */
     struct semid_ds *buf;           /* <= buffer for IPC_STAT & IPC_SET */
     unsigned short int *array;      /* <= array for GETALL & SETALL */
     struct seminfo *__buf;          /* <= buffer for IPC_INFO */
   };


/* The following System V style IPC functions implement a semaphore
   handling.  The definition is found in XPG2.  */

/* Structure used for argument to `semop' to describe operations.  */
struct sembuf
{
  short int sem_num;            /* semaphore number */
  short int sem_op;             /* semaphore operation */
  short int sem_flg;            /* operation flag */
};


__BEGIN_DECLS

/* Semaphore control operation.  */
extern int semctl __P ((int __semid, int __semnum, int __cmd, ...));

/* Get semaphore.  */
extern int semget __P ((key_t __key, int __nsems, int __semflg));

/* Operate on semaphore.  */
extern int semop __P ((int __semid, struct sembuf *__sops,
                       unsigned int __nsops));

__END_DECLS

#endif /* sys/sem.h */

#endif /* HAVE_CYGWIN */