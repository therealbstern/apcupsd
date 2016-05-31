/*
 * cloexec.c
 *
 * Utilities for handling CLOEXEC
 */

/*
 * Copyright (C) 2015 Adam Kropelin
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

/* Create socket with FD_CLOEXEC set */
sock_t socket_cloexec(int domain, int type, int protocol)
{
   sock_t s;
#ifdef SOCK_CLOEXEC
   s = socket(domain, type | SOCK_CLOEXEC, protocol);
   if (s == INVALID_SOCKET && errno == EINVAL)
   {
#endif
      // No SOCK_CLOEXEC, emulatre via fcntl
      s = socket(domain, type, protocol);
#ifdef FD_CLOEXEC
      if (s != INVALID_SOCKET)
      {
         int flags;
         if ((flags = fcntl(s, F_GETFD)) == -1 ||
             fcntl(s, F_SETFD, flags | FD_CLOEXEC) == -1)
         {
            Dmsg(0, "%s: Failed to set CLOEXEC\n", __func__);
         }
      }
#endif
#ifdef SOCK_CLOEXEC
   }
#endif
   return s;
}

/* Accept connection with FD_CLOEXEC set */
sock_t accept_cloexec(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
   sock_t s;
#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
   s = accept4(sockfd, addr, addrlen, SOCK_CLOEXEC);
   if (s == INVALID_SOCKET && errno == ENOSYS)
   {
#endif
      // No accept4, emulate via fcntl
      s = accept(sockfd, addr, addrlen);
#ifdef FD_CLOEXEC
      if (s != INVALID_SOCKET)
      {
         int flags;
         if ((flags = fcntl(s, F_GETFD)) == -1 ||
             fcntl(s, F_SETFD, flags | FD_CLOEXEC) == -1)
         {
            Dmsg(0, "%s: Failed to set CLOEXEC\n", __func__);
         }
      }
#endif
#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
   }
#endif
   return s;
}
