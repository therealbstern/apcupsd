/*
 *  apcnet.c  -- network parts for apcupsd package
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

 /*
  * Major rewrite following same logic 11 Jan 2000, Kern Sibbald
  */

#include "apc.h"

#ifdef HAVE_OLDNET

extern UPSCOMMANDS cmd[];

/* Local variables */
static int socketfd;	 
static int newsocketfd = -1;
static struct sockaddr_in my_adr;
static int masterlen;
static struct sockaddr_in master_adr;
static struct in_addr master_h_addr;
static struct netdata get_data;
static int slave_disconnected;	      /* set if any slave disconnected */

static int send_to_slave(UPSINFO *ups, int who);

/* 
 * Possible state for a slave (also partially for a master)
 * RMT_NOTCONNECTED    not yet connected, except for RECONNECT, this is the
 *		       only time when the slave sends back a message in response
 *		       to the packet we send.  He sends back his usermagic
 *		       or security id string.
 * RMT_CONNECTED       All is OK
 * RMT_RECONNECT       Must redo initial packet swap, but this time we
 *		       verify his usermagic rather than just store it.
 *		       This occurs after a connect() error.
 * RMT_ERROR	       Set when some packet error occurs, presumably the
 *		       next call should work.
 * RMT_DOWN	       This occurs when we detect a security violation (e.g.
 *		       unauthorized slave. We mark him down and no longer
 *		       talk to him.
 */

/********************************************************************** 
 *
 * Called by the master to connect to the slaves. 
 */
int prepare_master(UPSINFO *ups)
{
    int i;

    for (i=0; i<slave_count; i++) {
	slaves[i].remote_state = RMT_NOTCONNECTED;
	send_to_slave(ups, i);
    }
    return 0;			  /* OK */
}

/*********************************************************************/
static void send_to_all_slaves(UPSINFO *ups)
{
    int i;
    int stat;

    slave_disconnected = FALSE;
    for (i=0; i<slave_count; i++) {
       stat = send_to_slave(ups, i);
       /* Print error message once */
       if (slaves[i].error == 0) {
	  switch(stat) {
	  case 6:
              log_event(ups, LOG_WARNING, "Cannot resolve slave name %s. ERR=%s\n",
		  slaves[i].name, strerror(slaves[i].ms_errno));
	      break;
	  case 5:
              log_event(ups, LOG_WARNING,"Got slave shutdown from %s.", slaves[i].name);
	      break;	      
	  case 4:
              log_event(ups, LOG_WARNING,"Cannot write to slave %s. ERR=%s", 
		  slaves[i].name, strerror(slaves[i].ms_errno));
	      break;
	  case 3:
              log_event(ups, LOG_WARNING,"Cannot read magic from slave %s.", slaves[i].name);
	      break;
	  case 2:
              log_event(ups, LOG_WARNING,"Connect to slave %s failed. ERR=%s", 
		   slaves[i].name, strerror(slaves[i].ms_errno));
	      break;
	  case 1:
              log_event(ups, LOG_WARNING,"Cannot open stream socket. ERR=%s",
		  strerror(slaves[i].ms_errno));
	      break;
	  case 0:
	      break;
	  default:
              log_event(ups, LOG_WARNING,"Unknown Error (send_to_all_slaves)");
	      break;
	  }
       }
       if (slaves[i].remote_state != RMT_CONNECTED) {
	  slave_disconnected = TRUE;
       }
       slaves[i].error = stat;
       if (stat != 0)
	   slaves[i].errorcnt++;
    }
}

/********************************************************************* 
 * Called from master to send data to a specific slave (who).
 * Returns: 0 if OK
 *	    non-zero, see send_to_all_slaves();
 *
 * Note, be careful not to change RMT_NOTCONNECTED or 
 * RMT_RECONNECT until we have completed the connection, even if an
 * error occurs.
 */
static int send_to_slave(UPSINFO *ups, int who)
{
    struct hostent *slavent;
    struct netdata read_data;
    struct netdata send_data;
    int turnon = 1;
    int stat = 0;
    long l;

    switch (slaves[who].remote_state) {
    case RMT_NOTCONNECTED:
	if ((slavent = gethostbyname(slaves[who].name)) == NULL) {
	    slaves[who].ms_errno = errno;
	    slaves[who].remote_state = RMT_DOWN;
	    slaves[who].down_time = time(NULL);
	    return 6;
	}
	/* memset is more portable than bzero */
	memset((void *) &slaves[who].addr, 0, sizeof(struct sockaddr_in));
	slaves[who].addr.sin_family = AF_INET;
	memcpy((void *)&slaves[who].addr.sin_addr,
		 (void *)slavent->h_addr,  sizeof(struct in_addr));
	slaves[who].addr.sin_port = htons(ups->NetUpsPort);
	slaves[who].usermagic[0] = 0;
	slaves[who].socket = -1;
	break;
    case RMT_DOWN:
	/* 
	 * After 2 minutes of down time (due to serious error),
	 * attempt to bring the slave up on next poll.
	 */
	if (time(NULL) - slaves[who].down_time > 2 * 60) {
	   slaves[who].remote_state = RMT_NOTCONNECTED;
	}
	return 0;
    default:
	break;
    }

    if (slaves[who].socket == -1) {
	if ((slaves[who].socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    slaves[who].ms_errno = errno;
	    slaves[who].remote_state = RMT_DOWN;
	    slaves[who].down_time = time(NULL);
	    slaves[who].socket = -1;
	    return 1;
	}

	/*
	 * Receive notification when connection dies.
	 */
	if (setsockopt(slaves[who].socket, SOL_SOCKET, SO_KEEPALIVE, &turnon, sizeof(turnon)) < 0) {
            log_event(ups, LOG_WARNING, "Cannot set SO_KEEPALIVE on socket: %s\n" , strerror(errno));
	}

	if ((connect(slaves[who].socket, (struct sockaddr *) &slaves[who].addr,
		     sizeof(slaves[who].addr))) == -1) {
	    slaves[who].ms_errno = errno;
	    if (slaves[who].remote_state == RMT_CONNECTED)
		slaves[who].remote_state = RMT_RECONNECT;

	    shutdown(slaves[who].socket, SHUT_RDWR);
	    close(slaves[who].socket);
	    slaves[who].socket = -1;
	    return 2;
	}
    }
    /* Extra complexity is because of compiler
     * problems with htonl(ups->OnBatt);
     */
    l = (UPS_ISSET(UPS_ONBATT) ? 1 : 0);
    send_data.OnBatt	    = htonl(l);
    l = (UPS_ISSET(UPS_BATTLOW) ? 1 : 0);
    send_data.BattLow	    = htonl(l);
    l = (long)ups->BattChg;
    send_data.BattChg	    = htonl(l);
    l = ups->ShutDown;
    send_data.ShutDown	    = htonl(l);
    l = ups->nettime;
    send_data.nettime	    = htonl(l);
    l = (long)ups->TimeLeft;
    send_data.TimeLeft	    = htonl(l);
    l = (UPS_ISSET(UPS_REPLACEBATT) ? 1 : 0);
    send_data.ChangeBatt    = htonl(l);
    l = (UPS_ISSET(UPS_SHUT_LOAD) ? 1 : 0);
    send_data.load	    = htonl(l);
    l = (UPS_ISSET(UPS_SHUT_BTIME) ? 1 : 0);
    send_data.timedout	    = htonl(l);
    l = (UPS_ISSET(UPS_SHUT_LTIME) ? 1 : 0);
    send_data.timelout	    = htonl(l);
    l = (UPS_ISSET(UPS_SHUT_EMERG) ? 1 : 0);
    send_data.emergencydown = htonl(l);
    l = ups->UPS_Cap[CI_BATTLEV];
    send_data.cap_battlev   = htonl(l);
    l = ups->UPS_Cap[CI_RUNTIM];
    send_data.cap_runtim    = htonl(l);

    send_data.remote_state  = htonl(slaves[who].remote_state);
    strcpy(send_data.apcmagic, APC_MAGIC);
    strcpy(send_data.usermagic, slaves[who].usermagic);

    /* Send our data to Slave */
    if ((write(slaves[who].socket, &send_data,
			  sizeof(send_data))) != sizeof(send_data)) {
	if (slaves[who].remote_state == RMT_CONNECTED)
	    slaves[who].remote_state = RMT_RECONNECT;
	slaves[who].ms_errno = errno;
	stat = 4;	      /* write error */

    /*
     * If slave not yet connected, he will respond with his
     *  two "magic" values.
     */
    } else if (slaves[who].remote_state == RMT_NOTCONNECTED ||
		slaves[who].remote_state == RMT_RECONNECT) {
	fd_set rfds;
	struct timeval tv;

	read_data.apcmagic[0] = 0;
	read_data.usermagic[0] = 0;

	FD_ZERO(&rfds);
	FD_SET(slaves[who].socket, &rfds);
	tv.tv_sec = 10; 	      /* wait 10 seconds for response */
	tv.tv_usec = 0;
	   
	switch (select(slaves[who].socket+1, &rfds, NULL, NULL, &tv)) {
	case 0: 	     /* No chars available in 10 seconds. */
	    slaves[who].ms_errno = ETIME;
	    shutdown(slaves[who].socket, SHUT_RDWR);
	    close(slaves[who].socket);
	    slaves[who].socket = -1;
	    return 2;
	case -1:	     /* error */
	    slaves[who].ms_errno = errno;
	    shutdown(slaves[who].socket, SHUT_RDWR);
	    close(slaves[who].socket);
	    slaves[who].socket = -1;
	    return 2;
	default:
	    break;
	}
	read(slaves[who].socket, &read_data, sizeof(read_data));
	read_data.apcmagic[APC_MAGIC_SIZE-1] = 0;
	read_data.usermagic[APC_MAGIC_SIZE-1] = 0;
	if (strcmp(APC_MAGIC, read_data.apcmagic) == 0) { 
	    /*
	     * new non-disconnecting slaves send 1000 back in the
	     * UPS_CHANGEBATT field 
	     */
	    slaves[who].disconnecting_slave = ntohl(read_data.ChangeBatt) != 1000;	
            Dmsg3(400, "Slave %s disconnecting_slave=%d ChangeBatt value=%d\n", 
	       slaves[who].name, slaves[who].disconnecting_slave,
	       ntohl(read_data.ChangeBatt));
	    if (slaves[who].remote_state == RMT_NOTCONNECTED) {
		strcpy(slaves[who].usermagic, read_data.usermagic);
	    } else {
		if (strcmp(slaves[who].usermagic, read_data.usermagic) != 0)
		    stat = 3;	    /* bad magic */
	    }
	} else {
	    stat = 3;	      /* bad magic */
	}
	if (stat == 0) {
            log_event(ups, LOG_WARNING,"Connect to slave %s succeeded", slaves[who].name);
	    slaves[who].remote_state = RMT_CONNECTED;
	} 
    }
    if (slaves[who].remote_state != RMT_CONNECTED || 
	  slaves[who].disconnecting_slave) {
	shutdown(slaves[who].socket, SHUT_RDWR);
	close(slaves[who].socket);
	slaves[who].socket = -1;
    }
    return stat;
}


/*********************************************************************/

/* slaves
 *  Note, here we store certain information about the master in
 *  the slave[0] packet.
 */


/********************************************************************* 
 *
 * Called by a slave to open a socket and listen for the master 
 * Returns: 1 on error
 *	    0 OK
 */
int prepare_slave(UPSINFO *ups)
{
    int i, bound;
    struct hostent *mastent;
    int turnon = 1;
    struct in_addr local_ip;

    local_ip.s_addr = INADDR_ANY;
    if (ups->nisip[0]) {
	if (inet_pton(AF_INET, ups->nisip, &local_ip) != 1) {
            log_event(ups, LOG_WARNING, "Invalid NISIP specified: '%s'", ups->nisip);
	    local_ip.s_addr = INADDR_ANY;
	}  
    }  

    strcpy(ups->mode.long_name, "Network Slave"); /* don't know model */
    slaves[0].remote_state = RMT_DOWN;
    slaves[0].down_time = time(NULL);
    if ((mastent = gethostbyname(ups->master_name)) == NULL) {
        log_event(ups, LOG_ERR,"Can't resolve master name %s: ERR=%s", 
	    ups->master_name, strerror(errno));
	return 1;
    }
    /* save host address.  
     * Note, this is necessary because on Windows, mastent->h_addr
     * is not valid later when the connection is made.
     */
    memcpy((void *)&master_h_addr, (void *)mastent->h_addr, sizeof(struct in_addr));
    /* Open socket for network communication */
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_event(ups, LOG_ERR, "Cannot open slave stream socket. ERR=%s", 
	    strerror(errno));
	return 1;
    }

   /*
    * Reuse old sockets 
    */
   if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &turnon, sizeof(turnon)) < 0) {
      log_event(ups, LOG_WARNING, "Cannot set SO_REUSEADDR on socket: %s\n" , strerror(errno));
   }

    /* memset is more portable than bzero */
    memset((char *) &my_adr, 0, sizeof(struct sockaddr_in));
    my_adr.sin_family = AF_INET;
    my_adr.sin_addr = local_ip;
    my_adr.sin_port = htons(ups->NetUpsPort);

    bound = FALSE;
    for (i=0; i<30; i++) {	  /* retry every 30 seconds for 15 minutes */
	if (bind(socketfd, (struct sockaddr *) &my_adr, sizeof(my_adr)) < 0) {
            log_event(ups, LOG_WARNING, "Cannot bind port %d, retrying ...", ups->NetUpsPort);
	    sleep(30);
	} else {
	    bound = TRUE;
	    break;
	}
    }
    if (!bound) {
        log_event(ups, LOG_ERR, "Cannot bind port %d, probably already in use", ups->NetUpsPort);
	close(socketfd);
	return 1;
    }

    if (listen(socketfd, 1) == -1) {
        log_event(ups, LOG_ERR, "Listen Failure");
	close(socketfd);
	return 1;
    }

    slaves[0].remote_state = RMT_NOTCONNECTED;
    strcpy(slaves[0].name, ups->master_name);
    return 0;
}

/********************************************************************
 * 
 * Get data from master, we hang on accept() or read()
 * until the master decides that it is time to send us 
 * something. We must be patient.
 *
 * Returns: 0 OK 
 *	    non-zero is error, see update_from_master();
 */
static int get_data_from_master(UPSINFO *ups) 
 {
    int stat;
    int ShutDown;
    int turnon = 1;

    for ( ;; ) {
	fd_set rfds;
	struct timeval tv;

	if (newsocketfd == -1) {
	    UPS_SET(UPS_COMMLOST);
	    masterlen = sizeof(master_adr);
	    FD_ZERO(&rfds);
	    FD_SET(socketfd, &rfds);
	    tv.tv_sec = MASTER_TIMEOUT;
	    tv.tv_usec = 0;
	    switch (select(socketfd+1, &rfds, NULL, NULL, &tv)) {
	    case 0:		 /* No chars available in 2 minutes. */
	    case -1:		 /* error */
		slaves[0].remote_state = RMT_ERROR;
		return 6;
	    default:
		break;
	    }
	    if ((newsocketfd = accept(socketfd, (struct sockaddr *) &master_adr,
				  &masterlen)) < 0) {
		slaves[0].remote_state = RMT_DOWN;
		slaves[0].down_time = time(NULL);
		slaves[0].ms_errno = errno;
		newsocketfd = -1;
		return 1;
	    }

#ifdef HAVE_LIBWRAP
	    /*
             * This function checks the incoming client and if it's not
	     * allowed closes the connection.
	     */
	    if (check_wrappers(argvalue, newsocketfd) == FAILURE) {
		slaves[0].remote_state = RMT_DOWN;
		slaves[0].down_time = time(NULL);
		shutdown(newsocketfd, SHUT_RDWR);
		close(newsocketfd);
		newsocketfd = -1;
		return 2;
	    }
#endif

	    /*
             *  Let's add some basic security
	     */
	    if (memcmp((void *) &master_adr.sin_addr, (void *)&master_h_addr,
		       sizeof(struct in_addr)) != 0) {
		slaves[0].remote_state = RMT_DOWN;
		slaves[0].down_time = time(NULL);
		shutdown(newsocketfd, SHUT_RDWR);
		close(newsocketfd);
		newsocketfd = -1;
		return 2;
	    }
	}

	/*
	 * Receive notification when connection dies.
	 */
	if (setsockopt(newsocketfd, SOL_SOCKET, SO_KEEPALIVE, &turnon, sizeof(turnon)) < 0) {
            log_event(ups, LOG_WARNING, "Cannot set SO_KEEPALIVE on socket: %s\n" , strerror(errno));
	}

	FD_ZERO(&rfds);
	FD_SET(newsocketfd, &rfds);
	tv.tv_sec = MASTER_TIMEOUT;
	tv.tv_usec = 0;
	switch (select(newsocketfd+1, &rfds, NULL, NULL, &tv)) {
	case 0: 	     /* No chars available in 2 minutes. */
	case -1:	     /* error */
	    slaves[0].remote_state = RMT_ERROR;
	    shutdown(newsocketfd, SHUT_RDWR);
	    close(newsocketfd);
	    newsocketfd = -1;
	    UPS_SET(UPS_COMMLOST);
	    return 6;		      /* master not responding */
	default:
	    break;
	}
	if ((read(newsocketfd, &get_data, sizeof(get_data))) != sizeof(get_data)) {
	    get_data.apcmagic[APC_MAGIC_SIZE-1] = 0;
	    get_data.usermagic[APC_MAGIC_SIZE-1] = 0;
	    slaves[0].remote_state = RMT_ERROR;
	    slaves[0].ms_errno = errno;
	    shutdown(newsocketfd, SHUT_RDWR);
	    close(newsocketfd);
	    newsocketfd = -1;
	    UPS_SET(UPS_COMMLOST);
	    return 3;		      /* read error */
	}
	get_data.apcmagic[APC_MAGIC_SIZE-1] = 0;
	get_data.usermagic[APC_MAGIC_SIZE-1] = 0;
	break;
    }

    /* At this point, we read something */

    if (strcmp(APC_MAGIC, get_data.apcmagic) != 0) { 
	slaves[0].remote_state = RMT_ERROR;
	shutdown(newsocketfd, SHUT_RDWR);
	close(newsocketfd);
	newsocketfd = -1;
	UPS_SET(UPS_COMMLOST);
	return 4;
    }
	  
    stat = ntohl(get_data.remote_state);
    /* If not connected, send him our user magic */
    if (stat == RMT_NOTCONNECTED || stat == RMT_RECONNECT) {
	strcpy(get_data.apcmagic, APC_MAGIC);
	strcpy(get_data.usermagic, ups->usermagic);
	get_data.ChangeBatt = htonl(1000); /* flag to say we are non-disconnecting */
        Dmsg1(400, "Slave sending non-disconnecting flag = %d.\n",
	   ntohl(get_data.ChangeBatt));
	write(newsocketfd, &get_data, sizeof(get_data));
    }

    if (strcmp(ups->usermagic, get_data.usermagic) == 0) {
	if (ntohl(get_data.OnBatt)) {
	    UPS_CLEAR_ONLINE();
	} else {
	    UPS_SET_ONLINE();
	}
	if (ntohl(get_data.BattLow)) {
	    UPS_SET(UPS_BATTLOW);
	} else {
	    UPS_CLEAR(UPS_BATTLOW);
	}
	ups->BattChg	   = ntohl(get_data.BattChg);
	ShutDown	   = ntohl(get_data.ShutDown);
	ups->nettime	   = ntohl(get_data.nettime);
	ups->TimeLeft	   = ntohl(get_data.TimeLeft);
/*
 * Setting ChangeBatt triggers false alarms if the master goes
 * down and comes back up, so remove it for now.  KES 27Feb01
 *
 *	if (ntohl(get_data.ChangeBatt)) {
 *	    UPS_SET(UPS_REPLACEBATT);
 *	} else {
 *	    UPS_CLEAR(UPS_REPLACEBATT);
 *	}
 */
	if (ntohl(get_data.load)) {
	    UPS_SET(UPS_SHUT_LOAD);
	} else {
	    UPS_CLEAR(UPS_SHUT_LOAD);
	}
	if (ntohl(get_data.timedout)) {
	    UPS_SET(UPS_SHUT_BTIME);
	} else {
	    UPS_CLEAR(UPS_SHUT_BTIME);
	}
	if (ntohl(get_data.timelout)) {
	    UPS_SET(UPS_SHUT_LTIME);
	} else {
	    UPS_CLEAR(UPS_SHUT_LTIME);
	}
	if (ntohl(get_data.emergencydown)) {
	    UPS_SET(UPS_SHUT_EMERG);
	} else {
	    UPS_CLEAR(UPS_SHUT_EMERG);
	}
	ups->remote_state  = ntohl(get_data.remote_state);
	ups->UPS_Cap[CI_BATTLEV] = ntohl(get_data.cap_battlev);
	ups->UPS_Cap[CI_RUNTIM] = ntohl(get_data.cap_runtim);
    } else {
	slaves[0].remote_state = RMT_ERROR;
	shutdown(newsocketfd, SHUT_RDWR);
	close(newsocketfd);
	newsocketfd = -1;
	UPS_SET(UPS_COMMLOST);
	return 5;
    }

    if (ShutDown) {		    /* if master has shutdown */
	UPS_SET(UPS_SHUT_REMOTE); /* we go down too */
    }
	 
#ifdef old_disconnecting_code
    close(newsocketfd);
    newsocketfd = -1;
#endif

    /* 
     * Note if UPS_COMMLOST is set at this point, it is because it
     * was previously detected. If we get here, we have a good
     * connection, so generate the event and clear the flag.
     */
    if (UPS_ISSET(UPS_COMMLOST)) {
        log_event(ups, LOG_WARNING, "Connect from master %s succeeded",
		slaves[0].name);
	execute_command(ups, cmd[CMDMASTERCONN]);
	UPS_CLEAR(UPS_COMMLOST);
    } 
    slaves[0].remote_state = RMT_CONNECTED;
    time(&ups->last_master_connect_time);
    return 0;
}

/*********************************************************************
 *
 * Called from slave to get data from the master 
 * Returns: 0 on error
 *	    1 OK
 */
static int update_from_master(UPSINFO *ups)
{
    int stat = get_data_from_master(ups);
    /* Print error message once */
    if (slaves[0].error == 0) {
       switch(stat) {
	  case 0:
	     break;
	  case 1:
             log_event(ups, LOG_ERR, "Socket accept error");
	     break;
	  case 2:
             log_event(ups, LOG_ERR, "Unauthorised attempt from master %s",
			inet_ntoa(master_adr.sin_addr));
	     break;
	  case 3:
             log_event(ups, LOG_ERR, "Read failure from socket. ERR=%s\n",
		 strerror(slaves[0].ms_errno));
	     break;
	  case 4:
             log_event(ups, LOG_ERR, "Bad APC magic from master: %s", get_data.apcmagic);
	     break;
	  case 5:
             log_event(ups, LOG_ERR, "Bad user magic from master: %s", get_data.usermagic);
	     break;
	  case 6:
	     generate_event(ups, CMDMASTERTIMEOUT);
	     break;
	  default:
             log_event(ups, LOG_ERR, "Unknown Error in get_from_master");
	     break;
       }
    }
    slaves[0].error = stat;
    if (stat != 0)
       slaves[0].errorcnt++;
    return 0;
}


/*
 * XXX
 * 
 * Why ?
 *
 * This function seem to do nothing except for a special case ... and
 * it do only logging.
 *
 */
void kill_net(UPSINFO *ups) {
    log_event(ups, LOG_WARNING,"%s: Waiting For ShareUPS Master to shutdown.",
	    argvalue);
    sleep(60);
    log_event(ups, LOG_WARNING,"%s: Great Mains Returned, Trying a REBOOT",
	    argvalue);
    log_event(ups, LOG_WARNING, "%s: Drat!! Failed to have power killed by Master",
	    argvalue);
    log_event(ups, LOG_WARNING,"%s: Perform CPU-reset or power-off",
	    argvalue);
}

/********************************************************************* 
 *
 * Each slave executes this code, reading the status from the master
 * and doing the actions necessary (do_action).
 * Note, that the slave hangs on an accept() in get_from_master()
 * waiting for the master to send it data. 
 *
 * When time permits, we should implement an alert() to interrupt
 * the accept() periodically and check when the last time the master
 * gave us data.  He should be talking to us roughly every nettime
 * seconds. If not (for example, he is absent more than 2*nettime),
 * then we should issue a message saying that he is down.
 */
void do_net(UPSINFO *ups)
{
    init_thread_signals();

    UPS_SET(UPS_SLAVE);       /* We are networking not connected */

    while (1) {
	/* Note, we do not lock shm so that apcaccess can
	 * read it.  We are the only one allowed to update 
	 * it.
	 */
	update_from_master(ups);
	do_action(ups);
	do_reports(ups);
    }
}


/********************************************************************* 
 *
 * The master executes this code, periodically sending the status
 * to the slaves.
 *
 */
void do_slaves(UPSINFO *ups)
{
    time_t now;
    time_t last_time_net = 0;
    int slave_stat; 
      
    init_thread_signals();

    while (1) {
	time(&now);

        /* This read is necessary to update "ups" */

	/* Send info to slaves if nettime expired (set by apcupsd.conf)
	 * or in power fail situation (FASTPOLL) or some slave is 
	 * not connected (slave_disconnected)
	 */
	if (((now - last_time_net) > ups->nettime) || UPS_ISSET(UPS_FASTPOLL) ||
	     slave_disconnected) {;
	    slave_stat = slave_disconnected;
	    send_to_all_slaves(ups);
	    time(&last_time_net);
	    /*
	     * If change in slave status, update Status
	     */
	    if (slave_stat != slave_disconnected) {
		write_lock(ups);
		if (slave_disconnected) {
		   UPS_SET(UPS_SLAVEDOWN);
		} else {
		   UPS_CLEAR(UPS_SLAVEDOWN);
		}
		write_unlock(ups);
	    }
	}
	if (UPS_ISSET(UPS_FASTPOLL)) {
	    sleep(1);		      /* urgent send data faster */
	} else {
	    sleep(TIMER_SLAVES);      /* wait 10 seconds */
	}
    }
}

#else /* HAVE_OLDNET */

int prepare_master(UPSINFO *ups) {
    log_event(ups, LOG_ERR, _("Master/slave network code disabled. Use --enable-master-slave to enable"));
    Dmsg0(000, _("Master/slave network code disabled. Use --enable-master-slave to enable.\n"));
    return 1; /* Not OK */
}

int prepare_slave(UPSINFO *ups) {
    return prepare_master(ups);
}

void kill_net(UPSINFO *ups) {
    prepare_master(ups);
}

void do_slaves(UPSINFO *ups) {
    prepare_master(ups);
    exit(1);
}

void do_net(UPSINFO *ups) {
    prepare_master(ups);
    exit(1);
}

#endif /* HAVE_OLDNET */
