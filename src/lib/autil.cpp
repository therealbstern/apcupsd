#include "autil.h"
#include "apc.h"
#include <sys/time.h>

void calc_abstimeout(int msec, struct timespec *abstime)
{
      // It would be best to use clock_gettime here, but it is not
      // widely available, so use gettimeofday() which we can count on
      // being available, if not quite as accurate.
      struct timeval now;
      gettimeofday(&now, NULL);
      abstime->tv_sec = now.tv_sec + msec/1000;
      abstime->tv_nsec = now.tv_usec*1000 + (msec % 1000) * 1000000;
      if (abstime->tv_nsec >= 1000000000) {
         abstime->tv_sec++;
         abstime->tv_nsec -= 1000000000;
      }
}
