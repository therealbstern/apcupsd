/*
 * amutex.h
 *
 * Simple mutex wrapper class.
 */

/*
 * Copyright (C) 2007 Adam Kropelin
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

#ifndef __AMUTEX_H
#define __AMUTEX_H

#include <pthread.h>
#include "astring.h"
#include "apc.h"

class amutex
{
public:

   amutex(const char *name = DEFAULT_NAME, bool recursive = false);
   ~amutex();

   // Basic lock/unlock are inlined for efficiency
   inline void lock()   const { pthread_mutex_lock(&_mutex); }
   inline void unlock() const { pthread_mutex_unlock(&_mutex); }

   // Timed lock is out-of-line because of size
   bool lock(int msec);

private:

   astring _name;
   mutable pthread_mutex_t _mutex;

   static const char *DEFAULT_NAME;
   static const int TIMEOUT_FOREVER;
};

#endif
