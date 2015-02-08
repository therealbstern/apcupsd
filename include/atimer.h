/*
 * atimer.h
 *
 * Simple POSIX based timer class
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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

#ifndef __ATIMER_H
#define __ATIMER_H

#include <pthread.h>
#include "athread.h"

class atimer: public athread
{
public:

   class client
   {
   public:
      virtual void HandleTimeout(int id) = 0;
   protected:
      client() {}
      virtual ~client() {}
   };

   atimer(client &cli, int id = 0);
   ~atimer();

   void start(unsigned long msec);
   void stop();

private:

   virtual void body();

   client &_client;
   int _id;
   pthread_mutex_t _mutex;
   pthread_cond_t _condvar;
   bool _started;
   struct timespec _abstimeout;

   // Prevent use
   atimer(const atimer &rhs);
   atimer &operator=(const atimer &rhs);
};

#endif // __ATIMER_H
