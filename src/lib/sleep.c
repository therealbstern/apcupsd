#include "apc.h"

#ifndef HAVE_NANOSLEEP
/*
 * This is a close approximation of nanosleep() for platforms that
 * do not have it.
 */
int nanosleep(const struct timespec *req, struct timespec *rem)
{
   static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
   static pthread_cond_t timer = PTHREAD_COND_INITIALIZER;
   struct timespec timeout;
   int stat;

   struct timeval tv;
   struct timezone tz;

   /* Copy relative exit time */
   timeout = *req;

   /* Compute absolute exit time */
   gettimeofday(&tv, &tz);
   timeout.tv_nsec += tv.tv_usec * 1000;
   timeout.tv_sec += tv.tv_sec;
   while (timeout.tv_nsec >= 1000000000) {
      timeout.tv_nsec -= 1000000000;
      timeout.tv_sec++;
   }

   Dmsg1(200, "pthread_cond_timedwait sec=%d\n", timeout.tv_sec);

   /* Mutex is unlocked during the timedwait */
   P(timer_mutex);

   stat = pthread_cond_timedwait(&timer, &timer_mutex, &timeout);
   Dmsg1(200, "pthread_cond_timedwait stat=%d\n", stat);

   V(timer_mutex);

   /* Assume no time leftover */
   if (rem) {
      rem->tv_nsec = 0;
      rem->tv_sec = 0;
   }

   return 0;
}
#endif /* HAVE_NANOSLEEP */

#ifndef HAVE_USLEEP
/*
 * This is a close approximation of usleep() for platforms that
 * do not have it.
 */
void usleep(unsigned long usec)
{
   struct timespec ts;
   
   ts.tv_sec = usec/1000000;
   ts.tv_nsec = usec%1000;
   nanosleep(&ts, NULL);
}
#endif /* HAVE_USLEEP */
