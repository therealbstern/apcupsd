/*
 * nis.h
 *
 * Include file for nis.c definitions
 */

/*
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

#include "defines.h"

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 *
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on error
 */
int net_recv(sock_t sockfd, char *buff, int maxlen);

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 *
 * Returns number of bytes sent
 * Returns -1 on error
 */
int net_send(sock_t sockfd, const char *buff, int len);

/*     
 * Open a TCP connection to the UPS network server
 *
 * Returns -1 on error
 * Returns socket file descriptor otherwise
 */
sock_t net_open(const char *host, char *service, int port);

/* Close the network connection */
void net_close(sock_t sockfd);

/* Wait for and accept a new TCP connection */
sock_t net_accept(sock_t fd, struct sockaddr_in *cli_addr);
