/*
 * Dumb little program to read named pipe written
 * by apcupsd and to print what it got.
 *
 * The default file is /var/log/apcupsd.status
 *
 * To make: make readfifo
 *    
 * called
 *
 *    ./readfifo <named pipe>
 *
 *  it exits after printing 200 records
 *
 */
#include <stdio.h>

main(int argc, char *argv[])
{
   int i;
   int rfd;
   char buf[200];
   char *file = "/var/log/apcupsd.status";

   if (argc > 1) {
      file = argv[1];
   }

  if ((rfd = open(file, 0)) < 0) {     /* open named pipe read only */
      printf("Could not open named pipe %s\n", file);
      exit(1);
   } else {
/*    printf("Fifo opened as %d\n"); */
   }
   for (i=0; i<200; i++) {
      int n;
      n = read(rfd, buf, sizeof buf);
      buf[n] = 0;
/*    printf("%d bytes read from %s\n", n, argv[1]);    */
      printf(buf);
  }
}
