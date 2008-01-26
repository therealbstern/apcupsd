#include "aqueue.h"
#include <stdio.h>
#include <unistd.h>

void *th(void *arg)
{
   while(1) sleep(1);
   
}
int main()
{
   aqueue<int> queue;
   
   pthread_t pid;
   pthread_create(&pid, NULL, th, NULL);
   
   queue.enqueue(1);
   queue.enqueue(2);
   queue.enqueue(3);

   int t;
   queue.dequeue(t);
   printf("%d\n", t);
   queue.dequeue(t);
   printf("%d\n", t);
   queue.dequeue(t);
   printf("%d\n", t);

   printf("last=%d\n", queue.dequeue(t, 5000));

   return 0;
}
