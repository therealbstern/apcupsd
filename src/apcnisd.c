/*
 *  apcnisd.c	 -- Network Information Server daemon for apcupsd
 *
 *  Copyright (C) 1999 Kern Sibbald
 *     10 November 1999
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS "APCUPSD-Team" BE LIABLE TO ANY PARTY FOR 
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE "APCUPSD-Team" HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE "APCUPSD-Team" SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN "AS IS" BASIS, AND THE "APCUPSD-Team" HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE "APCUPSD-Team" HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, "APCC".  THE "APCUPSD-Team" DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH "APCC".  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */


#include "apc.h"

#define NETD_VERSION "1.0"

char *pname;

static char largebuf[4096];
static int  stat_recs;
static int  logstats = 0;
char argvalue[MAXSTRING];
UPSINFO *ups = NULL;

/* forward referenced subroutines */
void handle_client_request();
int do_daemon(int argc, char *argv[]); 
int do_inetd(int argc, char *argv[]);

void apcnisd_error_cleanup(void)
{
    if (ups)
	destroy_ups(ups);
    closelog();
    exit(1);
}

/*
 * This routine is called by the main process to
 * track its children. On CYGWIN, the child processes
 * don't always exit when the other end of the socket
 * hangs up. Thus they remain hung on a read(). After
 * 30 seconds, we send them a SIGTERM signal, which 
 * causes them to wake up to the reality of the situation.
 */
static void reap_children(int childpid)
{
   static int pids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   static int times[10];
   int i;
   time_t now;
    int wpid;


    time(&now);
    for (i=0; i<10; i++) {
	if (pids[i]) {
	    wpid = waitpid(pids[i], NULL, WNOHANG);	       
	    if (wpid == -1 || wpid == pids[i]) {
		pids[i] = 0;	      /* Child gone, remove from table */
	    } else if (wpid == 0 && ((now - times[i]) > 30)) {
		kill(pids[i], SIGTERM);  /* still running, kill it */
	    }
	}
    }
    /* Make another pass reaping killed programs and inserting new child */
    for (i=0; i<10; i++) {
	if (pids[i]) {
	    wpid = waitpid(pids[i], NULL, WNOHANG);	       
	    if (wpid == -1 || wpid == pids[i]) {
		pids[i] = 0;	      /* Child gone, remove from table */
	     }
	}
	if (childpid && (pids[i] == 0)) {
	    pids[i] = childpid;
	    times[i] = now;
	    childpid = 0;
	}
    }
}


static void status_open(UPSINFO *ups)
{
   largebuf[0] = 0;
   stat_recs = 0;
   logstats = ups->logstats;
}

#define STAT_REV 1

static int status_close(UPSINFO *ups, int nsockfd)
{
    int i;   
    char buf[MAXSTRING];
    char *sptr, *eptr;

    i = strlen(largebuf);
    if (i > sizeof(largebuf)-1) 
        Error_abort1("Status buffer overflow %d bytes\n", i-sizeof(largebuf));
    sprintf(buf, "APC      : %03d,%03d,%04d\n", STAT_REV, stat_recs, i);
    
    if (net_send(nsockfd, buf, strlen(buf)) <= 0)
	return -1;
    sptr = eptr = largebuf;
    for ( ; i > 0; i--) {
        if (*eptr == '\n') {
	   eptr++;
	   if (net_send(nsockfd, sptr, eptr - sptr) <= 0)
	       break;
	   sptr = eptr;
	} else 
	   eptr++;
    }
    if (net_send(nsockfd, NULL, 0) < 0)
	return -1;
    return 0;
}



/********************************************************************* 
 * log one line of the status file
 * also send it to system log
 *
 */
static void status_write(UPSINFO *ups, char *fmt, ...)
{
    va_list ap;
    char buf[MAXSTRING];

    va_start(ap, fmt);

    avsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    strcat(largebuf, buf);
    stat_recs++;
}


/* 
 * We begin execution here. If the first and only
 * argument given on the command line is a -i, then
 * we assume that we were called by inetd.  
 *
 */
int main(int argc, char *argv[])
{

    /*
     * Set specific cleanup handler.
     */
    error_cleanup = apcnisd_error_cleanup;

    strncpy(argvalue, argv[0], sizeof(argvalue)-1);
    argvalue[sizeof(argvalue)-1] = 0;

	if (argc == 1 || argc == 3) {
	do_daemon(argc, argv);
	} 
        else if ((argc == 2) && (strcmp(argv[1], "-i") == 0)) {
	do_inetd(argc, argv);
	} 
	else {
        error_exit("Usage: %s \n", argv[0]);
	}
    return 0;
}

#ifdef HAVE_LIBWRAP
int allow_severity = LOG_INFO;
int deny_severity = LOG_WARNING;

int check_wrappers(char *av, int newsock)
{
    struct request_info req;
    char *av0;

    av0 = strrchr(av, '/');
    if (av0) {
	av0++;			      /* strip all but final name */
    } else {
	av0 = av;
    }

    request_init(&req, RQ_DAEMON, av0, RQ_FILE, newsock, NULL);
    fromhost(&req);
    if (!hosts_access(&req)) {
	syslog(LOG_WARNING,
            _("Connection from %.500s refused by tcp_wrappers."),
	    eval_client(&req));
	return FAILURE;
    }
#ifdef I_WANT_LOTS_OF_LOGGING
    syslog(LOG_NOTICE, "connect from %.500s", eval_client(&req));
#endif
    return SUCCESS;
}

#endif /* HAVE_LIBWRAP */

/* Called here if started by any means other than by
 * inetd
 */
int do_daemon(int argc, char *argv[]) 
{
    int newsockfd, sockfd, childpid;
    struct sockaddr_in cli_addr;       /* client's address */
    struct sockaddr_in serv_addr;      /* our address */
    int turnon = 1;
    struct in_addr local_ip;

   local_ip.s_addr = INADDR_ANY;
   if ((argc == 3) && (strcmp(argv[1], "-a") == 0)) {
      if (inet_pton(AF_INET, argv[2], &local_ip) != 1) {
         error_exit("Invalid NISIP specified: '%s'", argv[2]);
      }
   }


    pname = argv[0];

    openlog("apcnetd", LOG_CONS|LOG_PID, LOG_DAEMON);

#ifndef HAVE_CYGWIN
    if ((childpid = fork()) < 0)
        Error_abort1("Cannot fork to become daemon. ERR=%s", strerror(errno));
    else if (childpid > 0)
	exit(0);			/* original parent */
#endif

    /* we are now a daemon */
    setsid();
    signal(SIGCHLD, SIG_IGN);	       /* prevent zombies */

    syslog(LOG_INFO, "apcnetd %s startup succeeded", NETD_VERSION);

    /*
     * Open a TCP socket  
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
       Error_abort1("server: cannot open stream socket. ERR=%s", strerror(errno));

    /*
     * Reuse old sockets, ignore errors
     */
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &turnon, sizeof(turnon));  

    /* 
     * Bind our local address so that the client can send to us.
     */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr = local_ip;
    serv_addr.sin_port = htons(NISPORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
       Error_abort1("server: cannot bind local address. ERR=%s", strerror(errno));

    listen(sockfd, 5);		       /* tell system we are ready */
    for (;;) {
	socklen_t clilen;
	/* 
	 * Wait for a connection from a client process.
	 */
	 clilen = sizeof(cli_addr);
	 newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	 if (newsockfd < 0) {
             Error_abort1("server: accept error. ERR=%s\n", strerror(errno));
	 }

#ifdef HAVE_LIBWRAP
       /*
        * This function checks the incoming client and if it's not
	* allowed closes the connection.
	*/
       if (check_wrappers("apcupsd", newsockfd) == FAILURE) {
	   shutdown(newsockfd, 2);
	   close(newsockfd);
	   continue;
       }
#endif

#ifdef HAVE_CYGWIN
	childpid = 0;
	handle_client_request(newsockfd);	 /* process the request */
#else 
	/* fork to provide the response */
	if ((childpid = fork()) < 0)
            Error_abort1("server: fork error. ERR=%s\n", strerror(errno));
	else if (childpid == 0) {      /* child process */
	    close(sockfd);		/* close original socket */
	    handle_client_request(newsockfd);	     /* process the request */
	    close(newsockfd);
	    exit(0);
	}
#endif

	close(newsockfd);	       /* parent process */
        /* Reap any "previous" children that were not properly
	 * reaped by the SIG_IGN */
	reap_children(childpid);
     }
}   

/* Called here if started by inetd */
int do_inetd(int argc, char *argv[]) 
{
    /* read from "stdin" -- socket is already open */
    handle_client_request(0);	      /* process the request */
    exit(0);
}   


/* 
 * Accept requests from client.
 *
 * Return when the connection is terminated.
 */

void handle_client_request(int nsockfd)
{
    FILE *events_file;
    char line[MAXSTRING];
    char errmsg[]   = "Invalid command\n";
    char notavail[] = "Not available\n";
    char notrun[]   = "Apcupsd not running\n";


    ups = attach_ups(ups, SHM_RDONLY);
    if (!ups) {
	net_send(nsockfd, notrun, sizeof(notrun));
	net_send(nsockfd, NULL, 0);
        Error_abort0("Cannot attach SYSV IPC.\n");
    }

    for (;;) {
	/* Read command */
       if ((net_recv(nsockfd, line, MAXSTRING)) <= 0) {
	   break;			/* connection terminated */
       }

       if (strncmp("status", line, 6) == 0) {
	   if (output_status(ups, nsockfd, status_open, status_write, status_close) < 0) {
	       break;
	   }

       } else if (strncmp("events", line, 6) == 0) {
	   if ((ups->eventfile[0] == 0)  ||
               (events_file = fopen(ups->eventfile, "r")) == NULL) {
	       net_send(nsockfd, notavail, sizeof(notavail));
	       if (net_send(nsockfd, NULL, 0) < 0)
		   break;
	   } else {
	       int stat = output_events(nsockfd, events_file);
	       fclose(events_file);
	       if (stat < 0) {
		   net_send(nsockfd, notavail, sizeof(notavail));
		   net_send(nsockfd, NULL, 0);
		   break;
	      }
	   }

       } else {
	   net_send(nsockfd, errmsg, sizeof(errmsg));
	   if (net_send(nsockfd, NULL, 0) < 0)
	       break;
       }
    }
    detach_ups(ups);
    /*
     * Don't forget to set ups structure = NULL (cleanup handler).
     */
    ups = NULL;
    return;
}
