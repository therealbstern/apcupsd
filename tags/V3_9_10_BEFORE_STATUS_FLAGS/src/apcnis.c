/*
 *  apcserver.c  -- Network server for apcupsd
 *
 *   Kern Sibbald 14 November 1999
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
#include "apc_nis.h"

#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif

#ifdef HAVE_PTHREADS
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#else
#define P(x)
#define V(x)
#endif


static char largebuf[4096];
static int stat_recs;

struct s_arg {
   UPSINFO *ups;
   int newsockfd;
};

#ifdef HAVE_LIBWRAP
int allow_severity = LOG_INFO;
int deny_severity = LOG_WARNING;

int check_wrappers(char *av, int newsock)
{
    struct request_info req;
    char *av0;

    if (strchr(av, '/'))
        av0 = strrchr(av, '/');
    else
	av0 = av;

    request_init(&req, RQ_DAEMON, av0, RQ_FILE, newsock, NULL);
    fromhost(&req);
    if (!hosts_access(&req)) {
	log_event(core_ups, LOG_WARNING,
            _("Connection from %.500s refused by tcp_wrappers."),
	    eval_client(&req));
	return FAILURE;
    }
#ifdef I_WANT_LOTS_OF_LOGGING
    log_event(core_ups, LOG_NOTICE, "connect from %.500s", eval_client(&req));
#endif
    return SUCCESS;
}

#endif /* HAVE_LIBWRAP */


/* forward referenced subroutines */
void *handle_client_request(void *arg);

/*
 * This routine is called by the main process to
 * track its children. On CYGWIN, the child processes
 * don't always exit when the other end of the socket
 * hangs up. Thus they remain hung on a read(). After
 * 30 seconds, we send them a SIGTERM signal, which 
 * causes them to wake up to the reality of the situation.
 */
#ifndef HAVE_PTHREADS
static void reap_children(int childpid)
{
    static int pids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static int times[10];
    int i;
    time_t now;


    time(&now);
    for (i=0; i<10; i++) {
	if (pids[i] && (waitpid(pids[i], NULL, WNOHANG) == pids[i]))
	    pids[i] = 0;
	else if (pids[i] && ((now - times[i]) > 30))
	    kill(pids[i], SIGTERM);
    }
    for (i=0; i<10; i++) {
	if (pids[i] && (waitpid(pids[i], NULL, WNOHANG) == pids[i]))
	    pids[i] = 0;
	if (childpid && (pids[i] == 0)) {
	    pids[i] = childpid;
	    times[i] = now;
	    childpid = 0;
	}
    }
}
#endif /* HAVE_PTHREADS */

static void status_open(UPSINFO *ups)
{
    P(mutex);
    largebuf[0] = 0;
    stat_recs = 0;
}

#define STAT_REV 1

/*
 * Send the status lines across the network one line
 * at a time (to prevent sending too large a buffer).
 *
 * Returns -1 on error or EOF
 *	    0 OK
 */
static int status_close(UPSINFO *ups, int nsockfd) 
{
    int i;
    char buf[MAXSTRING];
    char *sptr, *eptr;

    i = strlen(largebuf);
    sprintf(buf, "APC      : %03d,%03d,%04d\n", STAT_REV, stat_recs, i);
    
    if (net_send(nsockfd, buf, strlen(buf)) <= 0) {
	V(mutex);
	return -1;
    }
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
    if (net_send(nsockfd, NULL, 0) < 0) {
	V(mutex);
	return -1;
    }
    V(mutex);
    return 0;
}



/********************************************************************* 
 * 
 * Buffer up the status messages so that they can be sent
 * by the status_close() routine over the network.
 */
static void status_write(UPSINFO *ups, char *fmt, ...)
{
    va_list ap;
    int i;
    char buf[MAXSTRING];

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    if ((i=(strlen(largebuf) + strlen(buf))) < (sizeof(largebuf)-1)) {
	strcat(largebuf, buf);
	stat_recs++;
    } else
	log_event(ups, LOG_ERR,
          "apcserver.c: Status buffer overflow %d bytes\n", i-sizeof(largebuf));
}


void do_server(UPSINFO *ups)
{
   int newsockfd, sockfd, clilen, childpid;
   struct sockaddr_in cli_addr;       /* client's address */
   struct sockaddr_in serv_addr;      /* our address */
   int tlog;
   int turnon = 1;
   struct s_arg *arg;
   struct in_addr local_ip;

   init_thread_signals();

   for (tlog=0; (ups=attach_ups(ups, SHM_RDONLY)) == NULL; tlog -= 5*60 ) {
      if (tlog <= 0) {
	 tlog = 60*60; 
         log_event(ups, LOG_ERR, "apcserver: Cannot attach SYSV IPC.\n");
      }
      sleep(5*60);
   }

   local_ip.s_addr = INADDR_ANY;
   if (ups->nisip[0]) {
      if (inet_pton(AF_INET, ups->nisip, &local_ip) != 1) {
         log_event(ups, LOG_WARNING, "Invalid NISIP specified: '%s'", ups->nisip);
	 local_ip.s_addr = INADDR_ANY;
      }
   }

   /*
    * Open a TCP socket  
    */
   for (tlog=0; (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0; tlog -= 5*60 ) {
      if (tlog <= 0) {
	 tlog = 60*60; 
         log_event(ups, LOG_ERR,  "apcserver: cannot open stream socket");
      }
      sleep(5*60);
   }

   /*
    * Reuse old sockets 
    */
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &turnon, sizeof(turnon)) < 0) {
      log_event(ups, LOG_WARNING, "Cannot set SO_REUSEADDR on socket: %s\n" , strerror(errno));
   }
   /* 
    * Bind our local address so that the client can send to us.
    */
   memset((char *)&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr = local_ip;
   serv_addr.sin_port = htons(ups->statusport);

   for (tlog=0; bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0; tlog -= 5*60 ) {
      if (tlog <= 0) {
	 tlog = 60*60; 
         log_event(ups, LOG_ERR, "apcserver: cannot bind port %d. ERR=%s",
	    ups->statusport, strerror(errno));
      }
      sleep(5*60);
   }
   listen(sockfd, 5);		      /* tell system we are ready */

   log_event(ups, LOG_INFO, "NIS server startup succeeded");

   for (;;) {
      /* 
       * Wait for a connection from a client process.
       */
       clilen = sizeof(cli_addr);
       for (tlog=0; (newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0; tlog -= 5*60 ) {
	  if (tlog <= 0) {
	     tlog = 60*60; 
             log_event(ups, LOG_ERR,  "apcserver: accept error. ERR=%s",
		strerror(errno));
	  }
	  sleep(5*60);
       }

#ifdef HAVE_LIBWRAP
       /*
        * This function checks the incoming client and if it's not
	* allowed closes the connection.
	*/
       if (check_wrappers(argvalue, newsockfd) == FAILURE) {
	   shutdown(newsockfd, 2);
	   close(newsockfd);
	   continue;
       }
#endif

       arg = malloc(sizeof(struct s_arg));
       arg->newsockfd = newsockfd;
       arg->ups = ups;
       childpid = 0;

#ifdef HAVE_CYGWIN
       handle_client_request(arg);
       close(newsockfd);	      /* parent process */
#else

#ifdef HAVE_PTHREADS
       {
	  pthread_t tid;
	  pthread_create(&tid, NULL, handle_client_request, arg);
       }

#else 

       /* fork to provide the response */
       for (tlog=0; (childpid = fork()) < 0; tlog -= 5*60 ) {
	  if (tlog <= 0) {
	     tlog = 60*60; 
             log_event(ups, LOG_ERR, "apcserver: fork error. ERR=%s",
		strerror(errno));
	  }
	  sleep(5*60);
       } 
       if (childpid == 0) {	      /* child process */
	  close(sockfd);	      /* close original socket */
	  handle_client_request(arg); /* process the request */
	  shutdown(newsockfd, 2);
	  close(newsockfd);
	  exit(0);
       }
       close(newsockfd);	      /* parent process */
       reap_children(childpid);
#endif /* HAVE_PTHREADS */

#endif /* HAVE_CYGWIN */

    }
}   

/* 
 * Accept requests from client.  Send output one line
 * at a time followed by a zero length transmission.
 *
 * Return when the connection is terminated or there
 * is an error.
 */

void *handle_client_request(void *arg)
{
    FILE *events_file;
    char line[MAXSTRING];
    char errmsg[]   = "Invalid command\n";
    char notavail[] = "Not available\n";
    char notrun[]   = "Apcupsd not running\n";
    int nsockfd = ((struct s_arg *)arg)->newsockfd;
    UPSINFO *ups = ((struct s_arg *)arg)->ups;

#ifdef HAVE_PTHREADS
    pthread_detach(pthread_self());
#endif
    if ((ups=attach_ups(ups, SHM_RDONLY)) == NULL) {
	net_send(nsockfd, notrun, sizeof(notrun));
	net_send(nsockfd, NULL, 0);
	free(arg);
        Error_abort0("Cannot attach SYSV IPC.\n");
    }

    for (;;) {
	/* Read command */
	if (net_recv(nsockfd, line, MAXSTRING) <= 0) {
	    break;			 /* connection terminated */
	}

        if (strncmp("status", line, 6) == 0) {
	    if (output_status(ups, nsockfd, status_open, status_write, status_close) < 0) {
		break; 
	    }

        } else if (strncmp("events", line, 6) == 0) {
	    if ((ups->eventfile[0] == 0) ||
                 ((events_file = fopen(ups->eventfile, "r")) == NULL)) {
	       net_send(nsockfd, notavail, sizeof(notavail));	  
	       if (net_send(nsockfd, NULL, 0) < 0) {
		   break;
	       }
	   } else {
	       int stat = output_events(nsockfd, events_file);
	       fclose(events_file);
	       if (stat < 0) {
		   net_send(nsockfd, notavail, sizeof(notavail));
		   net_send(nsockfd, NULL, 0);
		   break;
	       }
	   }
	 
        } else if (strncmp("rawupsinfo", line, 10) == 0) {
	    net_send(nsockfd, (char *)ups, sizeof(UPSINFO));
	    if (net_send(nsockfd, NULL, 0) < 0) {
		break;
	    }
	
        } else if (strncmp("eprominfo", line, 9) == 0) {
	    int len;
	    len = strlen(ups->eprom) + 1;
	    net_send(nsockfd, ups->eprom, len);
	    len = strlen(ups->firmrev) + 1;
	    net_send(nsockfd, ups->firmrev, len);
	    len = strlen(ups->upsmodel) + 1;
	    net_send(nsockfd, ups->upsmodel, len);

	    if (net_send(nsockfd, NULL, 0) < 0) {
		break;
	    }

	} else {
	    net_send(nsockfd, errmsg, sizeof(errmsg));
	    if (net_send(nsockfd, NULL, 0) < 0) {
		break;
	    }
	}
    }
#ifdef HAVE_PTHREADS
    shutdown(nsockfd, 2);
    close(nsockfd);
#endif
    free(arg);
    detach_ups(ups);
    return NULL;
}
