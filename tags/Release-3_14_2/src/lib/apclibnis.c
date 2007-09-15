/*
 * apclibnis.c
 *
 * Network utility routines.
 *
 * Part of this code is derived from the Prentice Hall book
 * "Unix Network Programming" by W. Richard Stevens
 *
 * Developers, please note: do not include apcupsd headers
 * or other apcupsd internal information in this file
 * as it is used by independent client programs such as the cgi
 * programs.
 */

/*
 * Copyright (C) 1999-2006 Kern Sibbald
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

#include "apc.h"

#ifdef HAVE_NISLIB

#ifndef   INADDR_NONE
#define   INADDR_NONE    -1
#endif

int net_errno = 0;                 /* error number -- not yet implemented */
char *net_errmsg = NULL;           /* pointer to error message */
char net_errbuf[256];              /* error message buffer for messages */


#ifdef HAVE_MINGW
#define close(fd)           closesocket(fd)
#endif


/*
 * Read nbytes from the network.
 * It is possible that the total bytes require in several
 * read requests
 */

static int read_nbytes(int fd, char *ptr, int nbytes)
{
   int nleft, nread = 0;
   struct timeval timeout;
   int rc;
   fd_set fds;

   nleft = nbytes;

   while (nleft > 0) {
      do {
         /* Expect data from the server within 15 seconds */
         timeout.tv_sec = 15;
         timeout.tv_usec = 0;

         FD_ZERO(&fds);
         FD_SET(fd, &fds);

         rc = select(fd + 1, &fds, NULL, NULL, &timeout);

         switch (rc) {
         case -1:
            if (errno == EINTR || errno == EAGAIN)
               continue;
            net_errno = errno;
            return -1;           /* error */
         case 0:
            return 0;            /* timeout */
         }

         nread = recv(fd, ptr, nleft, 0);
      } while (nread == -1 && (errno == EINTR || errno == EAGAIN));

      if (nread <= 0) {
         net_errno = errno;
         return (nread);           /* error, or EOF */
      }

      nleft -= nread;
      ptr += nread;
   }

   return (nbytes - nleft);        /* return >= 0 */
}

/*
 * Write nbytes to the network.
 * It may require several writes.
 */
static int write_nbytes(int fd, char *ptr, int nbytes)
{
   int nleft, nwritten;

   nleft = nbytes;
   while (nleft > 0) {
#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
      /*       
       * Work around a bug in OpenBSD & FreeBSD userspace pthreads
       * implementations.
       *
       * The pthreads implementation under the hood sets O_NONBLOCK
       * implicitly on all fds. This setting is not visible to the user
       * application but is relied upon by the pthreads library to prevent
       * blocking syscalls in one thread from halting all threads in the
       * process. When a process exit()s or exec()s, the implicit
       * O_NONBLOCK flags are removed from all fds, EVEN THOSE IT INHERITED.
       * If another process is still using the inherited fds, there will
       * soon be trouble.
       *
       * apcupsd is bitten by this issue after fork()ing a child process to
       * run apccontrol.
       *
       * This seemingly-pointless fcntl() call causes the pthreads
       * library to reapply the O_NONBLOCK flag appropriately.
       */
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL));
#endif
      nwritten = send(fd, ptr, nleft, 0);

      if (nwritten <= 0) {
         net_errno = errno;
         return (nwritten);        /* error */
      }

      nleft -= nwritten;
      ptr += nwritten;
   }

   return (nbytes - nleft);
}

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on hard end of file (i.e. network connection close)
 * Returns -2 on error
 */
int net_recv(int sockfd, char *buff, int maxlen)
{
   int nbytes;
   short pktsiz;

   /* get data size -- in short */
   if ((nbytes = read_nbytes(sockfd, (char *)&pktsiz, sizeof(short))) <= 0) {
      /* probably pipe broken because client died */
      return -1;                   /* assume hard EOF received */
   }
   if (nbytes != sizeof(short))
      return -2;

   pktsiz = ntohs(pktsiz);         /* decode no. of bytes that follow */
   if (pktsiz > maxlen) {
      net_errmsg = "net_recv: record length too large\n";
      return -2;
   }
   if (pktsiz == 0)
      return 0;                    /* soft EOF */

   /* now read the actual data */
   if ((nbytes = read_nbytes(sockfd, buff, pktsiz)) <= 0) {
      net_errmsg = "net_recv: read_nbytes error\n";
      return -2;
   }
   if (nbytes != pktsiz) {
      net_errmsg = "net_recv: error in read_nbytes\n";
      return -2;
   }

   return (nbytes);                /* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
int net_send(int sockfd, char *buff, int len)
{
   int rc;
   short pktsiz;

   /* send short containing size of data packet */
   pktsiz = htons((short)len);
   rc = write_nbytes(sockfd, (char *)&pktsiz, sizeof(short));
   if (rc != sizeof(short)) {
      net_errmsg = "net_send: write_nbytes error of length prefix\n";
      return -1;
   }

   /* send data packet */
   rc = write_nbytes(sockfd, buff, len);
   if (rc != len) {
      net_errmsg = "net_send: write_nbytes error\n";
      return -1;
   }

   return rc;
}

/*     
 * Open a TCP connection to the UPS network server
 * Returns -1 on error
 * Returns socket file descriptor otherwise
 */
int net_open(char *host, char *service, int port)
{
   int sockfd;
   struct hostent *hp;
   struct sockaddr_in tcp_serv_addr;  /* socket information */
   unsigned int inaddr;               /* Careful here to use unsigned int for */
                                      /* compatibility with Alpha */

   /* 
    * Fill in the structure serv_addr with the address of
    * the server that we want to connect with.
    */
   memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
   tcp_serv_addr.sin_family = AF_INET;
   tcp_serv_addr.sin_port = htons(port);

   if ((inaddr = inet_addr(host)) != INADDR_NONE) {
      tcp_serv_addr.sin_addr.s_addr = inaddr;
   } else {
      if ((hp = gethostbyname(host)) == NULL) {
         net_errmsg = "tcp_open: hostname error\n";
         return -1;
      }

      if (hp->h_length != sizeof(inaddr) || hp->h_addrtype != AF_INET) {
         net_errmsg = "tcp_open: funny gethostbyname value\n";
         return -1;
      }

      tcp_serv_addr.sin_addr.s_addr = *(unsigned int *)hp->h_addr;
   }


   /* Open a TCP socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      net_errmsg = "tcp_open: cannot open stream socket\n";
      return -1;
   }

   /* connect to server */
#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
   /* 
    * Work around a bug in OpenBSD & FreeBSD userspace pthreads
    * implementations. Rationale is the same as described above.
    */
   fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL));
#endif

   if (connect(sockfd, (struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr)) == -1) {
      asnprintf(net_errbuf, sizeof(net_errbuf),
         _("tcp_open: cannot connect to server %s on port %d.\n"
        "ERR=%s\n"), host, port, strerror(errno));
      net_errmsg = net_errbuf;
      close(sockfd);
      return -1;
   }

   return sockfd;
}

/* Close the network connection */
void net_close(int sockfd)
{
   close(sockfd);
}

/*     
 * Accept a TCP connection.
 * Returns -1 on error.
 * Returns file descriptor of new connection otherwise.
 */
int net_accept(int fd, struct sockaddr_in *cli_addr)
{
#ifdef HAVE_MINGW                                       
   /* kludge because some idiot defines socklen_t as unsigned */
   int clilen = sizeof(*cli_addr);
#else
   socklen_t clilen = sizeof(*cli_addr);
#endif
   int newfd;

#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
   int rc;
   fd_set fds;
#endif

   do {

#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
      /*
       * Work around a bug in OpenBSD & FreeBSD userspace pthreads
       * implementations. Rationale is the same as described above.
       */
      do {
         FD_ZERO(&fds);
         FD_SET(fd, &fds);
         rc = select(fd + 1, &fds, NULL, NULL, NULL);
      } while (rc == -1 && (errno == EINTR || errno == EAGAIN));

      if (rc < 0) {
         net_errno = errno;
         return (-1);              /* error */
      }
#endif
      newfd = accept(fd, (struct sockaddr *)cli_addr, &clilen);
   } while (newfd == -1 && (errno == EINTR || errno == EAGAIN));

   if (newfd < 0) {
      net_errno = errno;
      return (-1);                 /* error */
   }

   return newfd;
}
#endif                             /* HAVE_NISLIB */
