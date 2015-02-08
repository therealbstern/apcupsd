/*
 * athread.h
 *
 * Simple POSIX based thread class
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

#ifndef __ATHREAD_H
#define __ATHREAD_H

#include <pthread.h>
 
class athread
{
public:

   athread(int prio = PRIORITY_INHERIT)
      : _prio(prio),
        _running(false) {}

   virtual ~athread() {}

   virtual bool run();
   virtual bool join();

protected:

   virtual void body() = 0;

   static void *springboard(void *arg);

   static const int PRIORITY_INHERIT;

   pthread_t _tid;
   int _prio;
   bool _running;
};

#endif
