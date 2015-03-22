/*
 * apclibnis.c
 *
 * Network utility routines.
 */

/*
 * Copyright (C) 1999-2006 Kern Sibbald
 * Copyright (C) 2007-2015 Adam Kropelin
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

#include "apc.h"

#ifdef HAVE_NISLIB

/* Some Win32 specific screwery */
#ifdef HAVE_MINGW

#define close(fd)             closesocket(fd)
#define getsockopt(s,l,o,d,z) getsockopt((s),(l),(o),(char*)(d),(z))
#define EINPROGRESS           WSAEWOULDBLOCK

int WSA_Init(void);
int dummy = WSA_Init();

#undef errno
#define errno   WSAGetLastError()

#undef h_errno
#define h_errno WSAGetLastError()

#endif // HAVE_MINGW


/*
 * Read nbytes from the network.
 * It is possible that the total bytes require in several
 * read requests
 */

static int read_nbytes(sock_t fd, char *ptr, int nbytes)
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
            return -errno;       /* error */
         case 0:
            return -ETIMEDOUT;   /* timeout */
         }

         nread = recv(fd, ptr, nleft, 0);
      } while (nread == -1 && (errno == EINTR || errno == EAGAIN));

      if (nread == 0)
         return 0;               /* EOF */
      if (nread < 0)
         return -errno;          /* error */

      nleft -= nread;
      ptr += nread;
   }

   return nbytes - nleft;        /* return >= 0 */
}

/*
 * Write nbytes to the network.
 * It may require several writes.
 */
static int write_nbytes(sock_t fd, const char *ptr, int nbytes)
{
   int nleft, nwritten;

   nleft = nbytes;
   while (nleft > 0) {
      nwritten = send(fd, ptr, nleft, 0);

      switch (nwritten) {
      case -1:
         if (errno == EINTR || errno == EAGAIN)
            continue;
         return -errno;           /* error */
      case 0:
         return nbytes - nleft;   /* EOF */
      }

      nleft -= nwritten;
      ptr += nwritten;
   }

   return nbytes - nleft;
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
int net_recv(sock_t sockfd, char *buff, int maxlen)
{
   int nbytes;
   unsigned short pktsiz;

   /* get data size -- in short */
   if ((nbytes = read_nbytes(sockfd, (char *)&pktsiz, sizeof(pktsiz))) <= 0) {
      /* probably pipe broken because client died */
      return nbytes;               /* assume hard EOF received */
   }
   if (nbytes != sizeof(pktsiz))
      return -EINVAL;

   pktsiz = ntohs(pktsiz);         /* decode no. of bytes that follow */
   if (pktsiz > maxlen)
      return -EINVAL;
   if (pktsiz == 0)
      return 0;                    /* soft EOF */

   /* now read the actual data */
   if ((nbytes = read_nbytes(sockfd, buff, pktsiz)) <= 0)
      return nbytes;
   if (nbytes != pktsiz)
      return -EINVAL;

   return nbytes;                /* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent, 0 for EOF
 * Returns -errno on error
 */
int net_send(sock_t sockfd, const char *buff, int len)
{
   int rc;
   short pktsiz;

   /* send short containing size of data packet */
   pktsiz = htons((short)len);
   rc = write_nbytes(sockfd, (char *)&pktsiz, sizeof(short));
   if (rc <= 0)
      return rc;
   if (rc != sizeof(short))
      return -EINVAL;

   /* send data packet */
   rc = write_nbytes(sockfd, buff, len);
   if (rc <= 0)
      return rc;
   if (rc != len)
      return -EINVAL;

   return rc;
}

/*     
 * Open a TCP connection to the UPS network server
 * Returns -errno on error
 * Returns socket file descriptor otherwise
 */
sock_t net_open(const char *host, char *service, int port)
{
   int nonblock = 1;
   int block = 0;
   sock_t sockfd;
   int rc;
   struct sockaddr_in tcp_serv_addr;  /* socket information */

#ifndef HAVE_MINGW
   // Every platform has their own magic way to avoid getting a SIGPIPE
   // when writing to a stream socket where the remote end has closed. 
   // This method works pretty much everywhere which avoids the mess
   // of figuring out which incantation this platform supports. (Excepting
   // for win32 which doesn't support signals at all.)
   struct sigaction sa;
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sa, NULL);
#endif

   /* 
    * Fill in the structure serv_addr with the address of
    * the server that we want to connect with.
    */
   memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
   tcp_serv_addr.sin_family = AF_INET;
   tcp_serv_addr.sin_port = htons(port);
   tcp_serv_addr.sin_addr.s_addr = inet_addr(host);
   if (tcp_serv_addr.sin_addr.s_addr == INADDR_NONE) {
      struct hostent he;
      char *tmphstbuf = NULL;
      size_t hstbuflen = 0;
      struct hostent *hp = gethostname_re(host, &he, &tmphstbuf, &hstbuflen);
      if (!hp)
      {
         free(tmphstbuf);
         Dmsg(100, "%s: gethostname fails: %d\n", __func__, h_errno);
         return -ENXIO;
      }

      if (hp->h_length != sizeof(tcp_serv_addr.sin_addr.s_addr) || 
          hp->h_addrtype != AF_INET)
      {
         free(tmphstbuf);
         Dmsg(100, "%s: Bad address returned from gethostbyname\n", __func__);
         return -EAFNOSUPPORT;
      }

      memcpy(&tcp_serv_addr.sin_addr.s_addr, hp->h_addr, 
             sizeof(tcp_serv_addr.sin_addr.s_addr));
      free(tmphstbuf);
   }

   /* Open a TCP socket */
   if ((sockfd = socket_cloexec(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      rc = -errno;
      Dmsg(100, "%s: socket fails: %s\n", __func__, strerror(-rc));
      return rc;
   }

   /* connect to server */

   /* Set socket to non-blocking mode */
   if (ioctl(sockfd, FIONBIO, &nonblock) != 0) {
      rc = -errno;
      close(sockfd);
      Dmsg(100, "%s: ioctl(FIONBIO,nonblock) fails: %s\n", __func__, strerror(-rc));
      return rc;
   }

   /* Initiate connection attempt */
   rc = connect(sockfd, (struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr));
   if (rc == -1 && errno != EINPROGRESS) {
      rc = -errno;
      close(sockfd);
      Dmsg(100, "%s: connect fails: %s\n", __func__, strerror(-rc));
      return rc;
   }

   /* If connection is in progress, wait for it to complete */
   if (rc == -1) {
      struct timeval timeout;
      fd_set fds;
      int err;
      socklen_t errlen = sizeof(err);

      do {
         /* Expect connection within 5 seconds */
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
         FD_ZERO(&fds);
         FD_SET(sockfd, &fds);

         /* Wait for connection to complete */
         rc = select(sockfd + 1, NULL, &fds, NULL, &timeout);
         switch (rc) {
         case -1: /* select error */
            if (errno == EINTR || errno == EAGAIN)
               continue;
            err = -errno;
            close(sockfd);
            Dmsg(100, "%s: select fails: %s\n", __func__, strerror(-err));
            return err;
         case 0: /* timeout */
            close(sockfd);
            Dmsg(100, "%s: select timeout\n", __func__);
            return -ETIMEDOUT;
         }
      }
      while (rc == -1 && (errno == EINTR || errno == EAGAIN));

      /* Connection completed? Check error status. */
      if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
         rc = -errno;
         close(sockfd);
         Dmsg(100, "%s: getsockopt fails: %s\n", __func__, strerror(-rc));
         return rc;
      }
      if (errlen != sizeof(err)) {
         close(sockfd);
         Dmsg(100, "%s: getsockopt bad length\n", __func__);
         return -EINVAL;
      }
      if (err) {
         close(sockfd);
         Dmsg(100, "%s: connection completion fails: %s\n", __func__, strerror(err));
         return -err;
      }
   }

   /* Connection completed successfully. Set socket back to blocking mode. */
   if (ioctl(sockfd, FIONBIO, &block) != 0) {
      rc = -errno;
      close(sockfd);
      Dmsg(100, "%s: ioctl(FIONBIO,block) fails: %s\n", __func__, strerror(-rc));
      return rc;
   }

   return sockfd;
}

/* Close the network connection */
void net_close(sock_t sockfd)
{
   close(sockfd);
}

/*     
 * Accept a TCP connection.
 * Returns -1 on error.
 * Returns file descriptor of new connection otherwise.
 */
sock_t net_accept(sock_t fd, struct sockaddr_in *cli_addr)
{
#ifdef HAVE_MINGW                                       
   /* kludge because some idiot defines socklen_t as unsigned */
   int clilen = sizeof(*cli_addr);
#else
   socklen_t clilen = sizeof(*cli_addr);
#endif
   sock_t newfd;

   do {
      newfd = accept_cloexec(fd, (struct sockaddr *)cli_addr, &clilen);
   } while (newfd == INVALID_SOCKET && (errno == EINTR || errno == EAGAIN));

   if (newfd < 0)
      return -errno;                 /* error */

   return newfd;
}
#endif                             /* HAVE_NISLIB */
