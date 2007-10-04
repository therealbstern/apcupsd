#ifndef __AMUTEX_H
#define __AMUTEX_H

#include <pthread.h>
#include "astring.h"

class amutex
{
public:

   amutex(const char *name = DEFAULT_NAME, bool recursive = false);
   ~amutex();

   // Basic lock/unlock are inlined for efficiency
   inline void lock() { pthread_mutex_lock(&_mutex); }
   inline void unlock() { pthread_mutex_unlock(&_mutex); }

   // Timed lock is out-of-line because of size
   bool lock(int msec);

private:

   astring _name;
   pthread_mutex_t _mutex;

   static const char *DEFAULT_NAME;
   static const int TIMEOUT_FOREVER;
};

#endif
