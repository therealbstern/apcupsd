#ifndef __ATIMER_H
#define __ATIMER_H

#include <pthread.h>
#include "athread.h"

class atimer: public athread
{
public:

   typedef void (*CallbackFunc)(void *arg);

   atimer(CallbackFunc callback, void *arg);
   ~atimer();

   void start(unsigned long msec);
   void stop();

private:

   virtual void body();

   CallbackFunc _callback;
   void *_arg;
   pthread_mutex_t _mutex;
   pthread_cond_t _condvar;
   bool _started;
   struct timespec _abstimeout;
};

#endif // __ATIMER_H
