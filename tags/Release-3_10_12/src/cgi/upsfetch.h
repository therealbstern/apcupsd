/* upsfetch.h - prototypes for important functions used when linking

   Copyright (C) 1999  Russell Kroll <rkroll@exploits.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              
*/

extern char statbuf[4096];
extern int statlen;

/* like strerror, but for errors that are related to the UPS code */
char *upsstrerror (int errnum);


/* Read data into memory buffer to be used by getupsvar() */
int fetch_events (char *host);

/* get <varname> from <host> and put the reply in <buf> */
int getupsvar (char *host, char *varname, char *buf, int buflen);

/* get a list of all variables from <host> and put it in <buf> */
int getupsvarlist (char *host, char *buf, int buflen);

/* open a tcp connection to <host> and return the fd if successful */
int upsconnect (char *host);

/* close tcp connection cleanly */
void closeupsfd (int fd);

/* get variable list via open connection <fd> and put it in <buf> */
int getupsvarlistfd (int fd, char *upsname, char *buf, int buflen);

/* get variable via open connection <fd> and put it in <buf> */
int getupsvarfd (int fd, char *upsname, char *varname, char *buf, int buflen);

/* login to the <upsname> via connection <fd> with <password> */
int upslogin (int fd, char *upsname, char *password);

extern int upserror;

/* privilege levels for upsgetprivs() */
#define UPSPRIV_MASTER  0x2001

/* request a privilege for this connection */
int upsgetprivs (int fd, int level);

/* send a raw buffer to the ups */
int upssendraw (int fd, char *cmd);

/* get a raw buffer from the ups */
int upsreadraw (int fd, char *buf, int buflen);
