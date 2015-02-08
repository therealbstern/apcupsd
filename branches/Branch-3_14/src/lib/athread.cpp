/*
 * athread.cpp
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

#include "athread.h"

const int athread::PRIORITY_INHERIT = -1;

bool athread::run()
{
   if (_running)
      return false;

   pthread_attr_t attr;
   pthread_attr_init(&attr);

   int inherit = PTHREAD_INHERIT_SCHED;
   if (_prio != PRIORITY_INHERIT) {
      struct sched_param param;
      param.sched_priority = _prio;
      pthread_attr_setschedparam(&attr, &param);
      inherit = PTHREAD_EXPLICIT_SCHED;
   }

   pthread_attr_setinheritsched(&attr, inherit);

   int rc = pthread_create(&_tid, &attr, &athread::springboard, this);
   if (rc == 0)
      _running = true;

   pthread_attr_destroy(&attr);
   return _running;
}

void *athread::springboard(void *arg)
{
   athread *_this = (athread*)arg;
   _this->body();
   _this->_running = false;
   return NULL;
}

bool athread::join()
{
   return pthread_join(_tid, NULL) == 0;
}
