#include "amutex.h"
#include "autil.h"

const char *amutex::DEFAULT_NAME = "unnamed_mutex";
const int amutex::TIMEOUT_FOREVER = -1;

amutex::amutex(const char *name, bool recursive)
   : _name(name)
{
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr,
      recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
   pthread_mutex_init(&_mutex, &attr);
   pthread_mutexattr_destroy(&attr);
}

amutex::~amutex()
{
   pthread_mutex_destroy(&_mutex);
}

bool amutex::lock(int msec)
{
   if (msec == TIMEOUT_FOREVER)
   {
      lock();
      return true;
   }

   struct timespec abstime;
   calc_abstimeout(msec, &abstime);
   return pthread_mutex_timedlock(&_mutex, &abstime) == 0;
}
