/*
 *  apcnet.c  -- network parts for apcupsd package
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *                           675 Mass Ave, Cambridge, MA 02139, USA
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

/*
 * Master side complete reimplementation to support asynchronous connections
 * New state logic, supporting the same slave protocol.
 *
 * Howard Wilkinson <howard@cohtech.com>
 * Coherent Technology Limited, London N16 8DH, England
 * 1st November 2003
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
static int slave_disconnected;        /* set if any slave disconnected */

static int connect_to_slave(UPSINFO *ups, int who);
static void send_to_all_slaves(UPSINFO *ups);
static int talk_to_slave(UPSINFO *ups, int who, int rok, int wok, int eok);
static void down_slave(UPSINFO *ups, int who);
static void close_slave(UPSINFO *ups, int who);
static int connect_slave(UPSINFO *ups, int who);
static int activeslaveset(UPSINFO *ups, fd_set *active_fds);
static int wait_for_slaves(UPSINFO *ups,
                           int nactive, fd_set *readfds, fd_set *writefds,
                           fd_set *exceptfds,
                           struct timeval *waittime);
static void process_slave_error(UPSINFO *ups, int who, int stat);
static int receive_from_slave(UPSINFO *ups, int who, int rok);
static int check_slave(UPSINFO *ups, int who);
static void setup_packet(UPSINFO *ups, int who, struct netdata *send_data);
static int send_packet(UPSINFO *ups, int who, struct netdata* send_data);
static int receive_packet(UPSINFO *ups, int who, struct netdata *read_data);
static int process_packet(UPSINFO *ups, int who, struct netdata *read_data);

/* 
 * Possible state for a slave (also partially for a master)
 * RMT_NOTCONNECTED    not yet connected, except for RECONNECT, this is the
 *                     only time when the slave sends back a message in response
 *                     to the packet we send.  He sends back his usermagic
 *                     or security id string.
 * RMT_CONNECTED       All is OK
 * RMT_RECONNECT       Must redo initial packet swap, but this time we
 *                     verify his usermagic rather than just store it.
 *                     This occurs after a connect() error.
 * RMT_ERROR           Set when some packet error occurs, presumably the
 *                     next call should work.
 * RMT_DOWN            This occurs when we detect a security violation (e.g.
 *                     unauthorized slave. We mark him down and no longer
 *                     talk to him.
 */

/********************************************************************** 
 *
 * Called by the master to connect to the slaves. 
 */
int prepare_master(UPSINFO *ups)
{
    int i;

    /*
     * Set up each slave first
     * Note connect just starts the connection process, we use
     * non-blocking sockets to get round the down slaves problems
     * with connect taking 3 minutes to time out
     */
    Dmsg0(100, "Prepare master called\n");
    for (i=0; i<slave_count; i++) {
        slaves[i].remote_state = RMT_NOTCONNECTED;
    }
    /*
     * Now just start the dialogue with each slave
     * The NOTCONNECTED state will mean we go through the initial
     * handshake
     */
    send_to_all_slaves(ups);
    Dmsg0(100, "Prepare master returning\n");
    return 0;                     /* OK */
}

/*********************************************************************
 * Called by the master to start a slave connection
 */
static int connect_to_slave(UPSINFO *ups, int who)
{
    struct hostent *slavent;
    int stat = 0;

    Dmsg1(100, "Enter connect_to_slave %d\n", who);
    switch (slaves[who].remote_state) {
    case RMT_NOTCONNECTED:
        if ((slavent = gethostbyname(slaves[who].name)) == NULL) {
            slaves[who].ms_errno = errno;
            slaves[who].remote_state = RMT_DOWN;
            slaves[who].down_time = time(NULL);
            stat = 6;
            break;
        } else {
            /* memset is more portable than bzero */
            memset((void *) &slaves[who].addr, 0, sizeof(struct sockaddr_in));
            slaves[who].addr.sin_family = AF_INET;
            memcpy((void *)&slaves[who].addr.sin_addr,
                   (void *)slavent->h_addr,  sizeof(struct in_addr));
            slaves[who].addr.sin_port = htons(ups->NetUpsPort);
            slaves[who].usermagic[0] = 0;
            slaves[who].socket = -1;
        }
        /*
         * Fall through for common state code
         */
    case RMT_RECONNECT:
        stat = connect_slave(ups, who);
        break;
    default:
        log_event(ups, LOG_WARNING,
                  "Unknown slave state setting %d! Down slave %d",
                  slaves[who].remote_state, who);
        down_slave(ups, who);
        break;
    }

    Dmsg2(100, "Exit connect_to_slave %d, stat = %d\n", who, stat);
    return stat;
}

/*********************************************************************
 * Mark slave as down
 */
static void down_slave(UPSINFO *ups, int who)
{
    if (slaves[who].socket != -1) close_slave(ups, who);
    slaves[who].remote_state = RMT_DOWN;
    slaves[who].down_time = time(NULL);
    return;
}

/*********************************************************************
 * Close down the connection to a slave
 */
static void close_slave(UPSINFO *ups, int who)
{
    shutdown(slaves[who].socket, SHUT_RDWR);
    close(slaves[who].socket);
    Dmsg2(100, "close slave %d - socket %d\n", who, slaves[who].socket);
    slaves[who].socket = -1;
    return;
}

/*********************************************************************
 * Initiate the connection to a slave
 */
static int connect_slave(UPSINFO *ups, int who)
{
    int stat = 0;
    int turnon = 1;

    if (slaves[who].socket == -1) {
        if ((slaves[who].socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          slaves[who].ms_errno = errno;
          slaves[who].remote_state = RMT_DOWN;
          slaves[who].down_time = time(NULL);
          slaves[who].socket = -1;
          stat = 1;
        }
    }

    if (slaves[who].socket != -1) {
        Dmsg2(100, "Slave %d has socket %d\n", who, slaves[who].socket);
        /*
         * Receive notification when connection dies.
         */
        if (setsockopt(slaves[who].socket, SOL_SOCKET, SO_KEEPALIVE,
                       &turnon, sizeof(turnon)) < 0) {
            log_event(ups, LOG_WARNING,
                      "Cannot set SO_KEEPALIVE on socket: %s\n",
                      strerror(errno));
        }

        /*
         * Connect timeouts take up to 3 minutes - which means other slaves
         * lose connections
         * We allow a slave 10 seconds to get a connection open
         */

        /*
         * Set the socket into non-block mode, issue the connect and return
         */

        if (fcntl(slaves[who].socket, F_SETFL, FNONBLOCK) < 0) {
            log_event(ups, LOG_WARNING,
                      "Cannot set NONBLOCK on socket: %s", strerror(errno));
        }

        if ((connect(slaves[who].socket, (struct sockaddr *) &slaves[who].addr,
                     sizeof(slaves[who].addr))) == -1) {
            if(errno == EINPROGRESS) {
                /*
                 * This is OK it just means we get it on the select
                 */
                Dmsg1(100, "Connect to slave %d in progress\n", who);
                if (slaves[who].remote_state == RMT_RECONNECT) 
                    slaves[who].remote_state = RMT_RECONNECTING1;
                else
                    slaves[who].remote_state = RMT_CONNECTING1;
            } else {
                slaves[who].ms_errno = errno;
                Dmsg1(100, "Cannot connect to slave: ERR=%s\n",
                      strerror(errno));
                if (slaves[who].remote_state == RMT_CONNECTED)
                   slaves[who].remote_state = RMT_RECONNECT;
                else
                  slaves[who].remote_state = RMT_NOTCONNECTED;
                close_slave(ups, who);
                stat = 2;
            }
        } else {
            Dmsg1(100, "Connect to slave %d completed OK\n", who);
            if (slaves[who].remote_state == RMT_RECONNECT)
                slaves[who].remote_state = RMT_RECONNECTING2;
            else
                slaves[who].remote_state = RMT_CONNECTING2;
        }
    }

    return stat;
}

/*********************************************************************/
static void send_to_all_slaves(UPSINFO *ups)
{
    /*
     * We want to visit every slave once each NETTIME.
     * A slave may be in a number of states and we process
     * it depending on the state
     *
     * RMT_NOTCONNECTED:
     *   We have no successful connection to this slave
     *   Initiate one now. 
     *     Success:->RMT_CONNECTING1
     *     Failure:->RMT_DOWN
     * RMT_CONNECTING1
     *   We have started a connection to the slave and are
     *   waiting for the connect to finish. Allow up to
     *   NETTIME for connection to return.
     *     Success:->RMT_CONNECTING2
     *     Failure:->RMT_DOWN
     * RMT_CONNECTING2
     *   We have a successful network connection to the slave
     *   send the slave initial data.
     *     Success:->RMT_CONNECTING3
     *     Failure:->RMT_DOWN
     * RMT_CONNECTING3
     *   We have a successful network connection to the slave
     *   and have sent initial data, waiting for response from
     *   slave. Allow up to NETTIME for the response to arrive
     *   Record magic data from response.
     *     Success:->RMT_CONNECTED
     *     Failure:->RMT_DOWN
     * RMT_CONNECTED
     *    We have a successful handshake with the slave. IF we have not
     *    contacted the slave for NETTIME then send the slave data.
     *    If slave is disconnecting then Success:->RMT_RECONNECT
     *    If slave is non-disconnecting then Success:->RMT_CONNECTED
     *      Failure:->RMT_DOWN
     * RMT_RECONNECT
     *   Initiate a new connection for slave.
     *     Success:->RMT_RECONNECTING1
     *     Failure:->RMT_DOWN
     * RMT_RECONNECTING1
     *   We have started a connection to the slave and are
     *   waiting for the connect to finish. Allow up to
     *   NETTIME for connection to return.
     *     Success:->RMT_RECONNECTING2
     *     Failure:->RMT_DOWN
     * RMT_RECONNECTING2
     *   We have a successful network connection to the slave
     *   send the slave initial data.
     *     Success:->RMT_RECONNECTING3
     *     Failure:->RMT_DOWN
     * RMT_RECONNECTING3
     *   We have a successful network connection to the slave
     *   and have sent initial data, waiting for response from
     *   slave. Allow up to NETTIME for the response to arrive.
     *   Check that magic data matches previous connection.
     *     Success:->RMT_CONNECTED
     *     Failure:->RMT_DOWN
     * RMT_ERROR
     *   Unreachable state in the manager.
     * RMT_DOWN
     *   The slave has failed to connect successfully, wait for
     *   a grace period and try again. We calculate the grace period
     *   as
     * (2 * number_of_slaves * NETTIME) - rand((number_of_slaves * NETTIME)/2)
     *     Success:->RMT_NOTCONNECTED
     *     Failure:->RMT_DOWN
     *
     *  We can spend no more than NETTIME inside this routine.
     */

    int i;
    int stat = 0;
    int base_time = (slave_count * ups->nettime);
    time_t start_time = time(NULL);
    fd_set active_fds;
    fd_set ready_rfds, ready_wfds, ready_efds;
    int slavesready = 0;
    int slavesactive = 0;

    slave_disconnected = FALSE;
    /*
     * Look at all slaves and if any are marked down see if we should
     * start the connection machine on them.
     */
    for (i=0; i<slave_count; i++) {
        if (slaves[i].remote_state == RMT_DOWN) {
            int down_elapsed = start_time - slaves[i].down_time;
            int rand_time = (int)((base_time/2.0)*rand()/(RAND_MAX+1.0));
            Dmsg4(100,
                  "send_to_all_slaves slave %d down, elapsed %d, base_time %d, rand_time %d\n",
                  i, down_elapsed, 2 * base_time, rand_time);
            if (down_elapsed > (2 * base_time - rand_time)) {
              slaves[i].remote_state = RMT_NOTCONNECTED;
            }
        }
    }

    /*
     * Look at all slaves and if any are marked in notconnected start
     * connections
     */
    for (i=0; i<slave_count; i++) {
        if ((slaves[i].remote_state == RMT_NOTCONNECTED)
            || (slaves[i].remote_state == RMT_RECONNECT)) {
          connect_to_slave(ups, i);
        }
    }
    /*
     * Get all active slave sockets for select
     */
    slavesactive = activeslaveset(ups, &active_fds);

    Dmsg1(100, "send_to_slaves has %d active slave sockets\n", slavesactive);

    memcpy((void*)&ready_rfds, (void*)&active_fds, sizeof(active_fds));
    memcpy((void*)&ready_wfds, (void*)&active_fds, sizeof(active_fds));
    memcpy((void*)&ready_efds, (void*)&active_fds, sizeof(active_fds));

    /*
     * Process each slave at most once in each NETTIME interval
     */
    while(slavesactive) {
        /*
         * We can wait up to the end of interval for something to
         * be active
         */
        struct timeval waittime;

        waittime.tv_sec = ups->nettime - (time(NULL) - start_time);
        waittime.tv_usec = 0;

        if (waittime.tv_sec < 0) {
            /*
             * Do a poll!
             */
            waittime.tv_sec = 0;
        }

        Dmsg1(100, "send_to_slaves will wait %d seconds\n", waittime.tv_sec);

        slavesready = wait_for_slaves(ups, slavesactive,
                                      &ready_rfds,
                                      &ready_wfds,
                                      &ready_efds,
                                      &waittime);

        Dmsg2(100, "send_to_slaves has %d slaves ready with %d seconds left\n",
              slavesready, waittime.tv_sec);

        /*
         * If slavesready is zero then no sockets are
         * open and we must have timed out
         * So just finish the sends here
         */
        if (slavesready <= 0) break;

        FD_ZERO(&active_fds);

        for (i=0; i<slave_count; i++) {
            int slavesocket = slaves[i].socket;
            if (slavesocket == -1) continue;
            if (FD_ISSET(slavesocket, &ready_rfds)
                || FD_ISSET(slavesocket, &ready_wfds)
                || FD_ISSET(slavesocket, &ready_efds)) {
                int oldstate = slaves[i].remote_state;
                Dmsg1(100, "Talking to slave %d\n", i);
                stat = talk_to_slave(ups,
                                     i,
                                     FD_ISSET(slavesocket, &ready_rfds),
                                     FD_ISSET(slavesocket, &ready_wfds),
                                     FD_ISSET(slavesocket, &ready_efds));
                Dmsg2(100, "talk_to_slave %d  returned %d\n", i, stat);
                process_slave_error(ups, i, stat);
                /*
                 * If we still have a connection to the slave
                 * and the slave is not CONNECTED
                 * (i.e. we have finished the handshake)
                 * and the slave is not in RECONNECT state and started
                 * in RECONNECT state, then we can have another go
                 * this time round
                 */
                if ((slaves[i].socket != -1)
                    && ((slaves[i].remote_state != RMT_CONNECTED)
                        && ((slaves[i].remote_state != RMT_RECONNECT)
                            && (oldstate != RMT_RECONNECT))))
                  FD_SET(slavesocket, &active_fds);
            }
        }

        /*
         * Have we run out of time?
         * then just finish and come back later
         */

        if (waittime.tv_sec <= 0) {
          if (waittime.tv_usec <= 0) break;
        }
        /*
         * Check to see if we can talk to more slaves
         */
        slavesactive = 0;
        for (i=0; i<slave_count; i++) {
          int slavesocket = slaves[i].socket;
          if ((slavesocket != -1)
              && FD_ISSET(slavesocket, &active_fds)) {
            slavesactive = (slavesactive < slavesocket)
              ?slavesocket
              :slavesactive;
          }
        }
        /*
         * Go round again and deal with any reads on second loop
         */
        memcpy((void*)&ready_rfds, (void*)&active_fds, sizeof(active_fds));
        memcpy((void*)&ready_wfds, (void*)&active_fds, sizeof(active_fds));
        memcpy((void*)&ready_efds, (void*)&active_fds, sizeof(active_fds));
    }

    return;
}

/*********************************************************************
 * Return the select masks for the active slaves
 */
static int activeslaveset(UPSINFO *ups,
                          fd_set *active_fds)
{
    int i;
    int nsockets = 0;

    FD_ZERO(active_fds);
    for (i=0; i<slave_count; i++) {
        if (slaves[i].socket != -1) {
            FD_SET(slaves[i].socket, active_fds);
            if (nsockets < slaves[i].socket)
                nsockets = slaves[i].socket;
        }
    }
    return nsockets;
}
/*********************************************************************
 * Wait for all of the slaves to be ready to receive data
 */
static int wait_for_slaves(UPSINFO *ups,
                           int nactive,
                           fd_set *ready_rfds,
                           fd_set *ready_wfds,
                           fd_set *ready_efds,
                           struct timeval *waittime)
{
    struct timeval timeout;
    int i;
    fd_set rfds, wfds, efds;

    timeout.tv_sec = waittime->tv_sec;
    timeout.tv_usec = waittime->tv_usec;

    FD_ZERO(&rfds);
    if (ready_rfds)
      memcpy(&rfds, ready_rfds, sizeof(fd_set));
    FD_ZERO(&wfds);
    if (ready_wfds)
      memcpy(&wfds, ready_wfds, sizeof(fd_set));
    FD_ZERO(&efds);
    if (ready_efds)
      memcpy(&efds, ready_efds, sizeof(fd_set));

    Dmsg2(100, "wait_for_slaves %d for %d secs\n", nactive, waittime->tv_sec);

    while (1) {
        if ((i = select(nactive + 1,
                        ready_rfds,
                        ready_wfds,
                        ready_efds,
                        waittime)) < 0) {
            log_event(ups, LOG_WARNING,
                      "Select for slaves failed %s", strerror(errno));
            if (errno == EINTR) {
#ifdef notdef
              /* Don't include this as we would like time to pass */
                waittime->tv_sec = timeout.tv_sec;
                waittime->tv_usec = timeout.tv_usec;
#endif
                if (ready_rfds)
                  memcpy(ready_rfds, &rfds, sizeof(fd_set));
                if (ready_wfds)
                  memcpy(ready_wfds, &wfds, sizeof(fd_set));
                if (ready_efds)
                  memcpy(ready_efds, &efds, sizeof(fd_set));
                continue;
            }
            i = 0;
        }
        break;
    }

    return i;
}

/*********************************************************************
 * Process any error returned from the slave send
 */
static void process_slave_error(UPSINFO *ups, int who, int stat)
{
    Dmsg2(100, "process_slave_error called for slave %d - stat %d\n",
          who, stat);
    if (slaves[who].error == 0) {
        Dmsg1(100, "process_slave_error first time for %d\n", who);
        switch(stat) {
        case 7:
          /*
           * Non fatal error for information only
           */
#ifdef notdef
          log_event(ups, LOG_WARNING,
                    "talking to slave %s stalled restart conversation\n",
                    slaves[who].name);
#endif
          Dmsg2(100, "talking to slave %d(%s) stalled restart conversation\n",
                who, slaves[who].name);
          break;
        case 6:
            log_event(ups, LOG_WARNING,
                      "Cannot resolve slave name %s. ERR=%s\n",
                      slaves[who].name, strerror(slaves[who].ms_errno));
            break;
        case 5:
            log_event(ups, LOG_WARNING,
                      "Got slave shutdown from %s.", slaves[who].name);
            break;
        case 4:
            log_event(ups, LOG_WARNING,
                      "Cannot write to slave %s. ERR=%s", 
                      slaves[who].name, strerror(slaves[who].ms_errno));
            break;
        case 3:
            log_event(ups, LOG_WARNING,
                      "Cannot read magic from slave %s.", slaves[who].name);
            break;
        case 2:
            log_event(ups, LOG_WARNING,
                      "Connect to slave %s failed. ERR=%s", 
                      slaves[who].name, strerror(slaves[who].ms_errno));
            break;
        case 1:
            log_event(ups, LOG_WARNING,"Cannot open stream socket. ERR=%s",
                      strerror(slaves[who].ms_errno));
            break;
        case 0:
            break;
        default:
            log_event(ups, LOG_WARNING,
                      "Unknown Error Slavenum=%d, ERR=%s, stat=%d",
                      who, strerror(slaves[who].ms_errno), stat);
            break;
        }
    }
    slaves[who].error = stat;
    if (stat != 0)
        slaves[who].errorcnt++;
    Dmsg2(100, "process_slave_error returns for slave %d - stat %d\n",
          who, stat);
    return;
}

/********************************************************************* 
 * Called from master to send data to a specific slave (who).
 * Returns: 0 if OK
 *          non-zero, see process_slave_error();
 *
 */
static int talk_to_slave(UPSINFO *ups, int who, int rok, int wok, int eok)
{
    struct netdata send_data;
    int stat = 0;
    int sockerror = 0;
    int sockerrsz = sizeof(sockerror);

    Dmsg5(100,
          "Enter talk_to_slave %d - state %d, rok(%d), wok(%d), eok(%d)\n",
          who, slaves[who].remote_state, rok, wok, eok);

    switch (slaves[who].remote_state) {
    case RMT_NOTCONNECTED:
    case RMT_RECONNECT:
        log_event(ups, LOG_WARNING,
                  "Talking to a not connected slave! Down slave %d",
                  who);
        down_slave(ups, who);
        break;
    case RMT_CONNECTING1:
        Dmsg1(100, "Slave %d async connection\n", who);
    case RMT_RECONNECTING1:
        Dmsg1(100, "Slave %d async (re)connection\n", who);
        if (wok) {
            if (getsockopt(slaves[who].socket, SOL_SOCKET,
                           SO_ERROR, &sockerror, &sockerrsz) < 0) {
                log_event(ups, LOG_WARNING,
                          "Cannot get connection result from socket %d, -- %s",
                          slaves[who].socket, strerror(errno));
                down_slave(ups, who);
                break;
            } else {
                if (sockerror) {
                    log_event(ups, LOG_WARNING,
                              "Slave connection failed %s! Down slave %d",
                              strerror(sockerror), who);
                    down_slave(ups, who);
                    break;
                }
                Dmsg1(100, "Slave %d connected OK!\n", who);
                if (slaves[who].remote_state == RMT_RECONNECTING1)
                    slaves[who].remote_state = RMT_RECONNECTING2;
                else
                    slaves[who].remote_state = RMT_CONNECTING2;
                /*
                 * Short circuit calls to talk and drop through to next state
                 */
            }
        } else {
            /*
             * Connection is still not ready
             */
          Dmsg1(100, "Slave %d still not connected OK!\n", who);
          break;
        }
    case RMT_CONNECTING2:
        Dmsg1(100, "Slave %d send connect packet\n", who);
    case RMT_RECONNECTING2:
        Dmsg1(100, "Slave %d send (re)connect packet\n", who);
        if (wok) {
            setup_packet(ups, who, &send_data);
            stat = send_packet(ups, who, &send_data);
            if (!stat) {
                Dmsg1(100, "Slave %d sent packet OK!\n", who);
                if (slaves[who].remote_state == RMT_RECONNECTING2)
                    slaves[who].remote_state = RMT_RECONNECTING3;
                else
                    slaves[who].remote_state = RMT_CONNECTING3;
              /*
               * Short circuit calls to talk and drop through to next state
               */
            } else {
                Dmsg2(100, "Slave %d failed to send packet!\n", who, stat);
                break;
            }
        } else {
            /*
             * Connection is not ready - try again later
             */
            Dmsg1(100, "Slave %d still not ready to handshake!\n", who);
            break;
        }
    case RMT_CONNECTING3:
        Dmsg1(100, "Slave %d receive connect handshake\n", who);
    case RMT_RECONNECTING3:
        Dmsg1(100, "Slave %d receive (re)connect handshake\n", who);
        stat = receive_from_slave(ups, who, rok);
        if (!stat) {
            Dmsg1(100, "Slave %d received packet OK!\n", who);
            if (slaves[who].disconnecting_slave) {
              slaves[who].remote_state = RMT_RECONNECT;
              close_slave(ups, who);
            } else {
              slaves[who].remote_state = RMT_CONNECTED;
            }
        } else {
          Dmsg2(100, "Slave %d receive returns stat %d\n", who, stat);
        }
        break;
    case RMT_CONNECTED:
        Dmsg1(100, "Slave %d send periodic data\n", who);
        if (wok) {
            setup_packet(ups, who, &send_data);
            stat = send_packet(ups, who, &send_data);
            if (slaves[who].remote_state != RMT_CONNECTED)
                slave_disconnected = TRUE;
            Dmsg2(100, "Slave %d sent packet stat %d\n", who, stat);
        }
        break;
    case RMT_ERROR:
        log_event(ups, LOG_WARNING,
                  "Slave is in error state! Down slave %d",
                  who);
        down_slave(ups, who);
        break;
    case RMT_DOWN:
        log_event(ups, LOG_WARNING,
                  "Slave is down but ready! Just ignore %d",
                  who);
        break;
    default:
        log_event(ups, LOG_WARNING,
                  "Slave in unknown state! Down slave %d",
                  who);
        down_slave(ups, who);
        break;
    }

#ifdef notdef
    /*
     * Should not be needed here as it is handled in (RE)CONNECTING3
     */
    if (slaves[who].remote_state == RMT_CONNECTED &&
          slaves[who].disconnecting_slave) {
        slaves[who].remote_state = RMT_RECONNECT;
        close_slave(ups, who);
    }
#endif
  
    Dmsg2(100, "Exit talk_to_slave %d - state %d\n",
          who, slaves[who].remote_state);
    return stat;
}

/*********************************************************************
 * Setup the send packet for the slave
 */
static void setup_packet(UPSINFO *ups, int who, struct netdata *send_data)
{
    long l;

    /* Extra complexity is because of compiler
     * problems with htonl(ups->OnBatt);
     */
    l = (is_ups_set(UPS_ONBATT) ? 1 : 0);
    send_data->OnBatt       = htonl(l);
    l = (is_ups_set(UPS_BATTLOW) ? 1 : 0);
    send_data->BattLow      = htonl(l);
    l = (long)ups->BattChg;
    send_data->BattChg      = htonl(l);
    l = ups->ShutDown;
    send_data->ShutDown     = htonl(l);
    l = ups->nettime;
    send_data->nettime      = htonl(l);
    l = (long)ups->TimeLeft;
    send_data->TimeLeft     = htonl(l);
    l = (is_ups_set(UPS_REPLACEBATT) ? 1 : 0);
    send_data->ChangeBatt    = htonl(l);
    l = (is_ups_set(UPS_SHUT_LOAD) ? 1 : 0);
    send_data->load         = htonl(l);
    l = (is_ups_set(UPS_SHUT_BTIME) ? 1 : 0);
    send_data->timedout     = htonl(l);
    l = (is_ups_set(UPS_SHUT_LTIME) ? 1 : 0);
    send_data->timelout     = htonl(l);
    l = (is_ups_set(UPS_SHUT_EMERG) ? 1 : 0);
    send_data->emergencydown = htonl(l);
    l = ups->UPS_Cap[CI_BATTLEV];
    send_data->cap_battlev   = htonl(l);
    l = ups->UPS_Cap[CI_RUNTIM];
    send_data->cap_runtim    = htonl(l);

    send_data->remote_state
      = htonl((slaves[who].remote_state == RMT_RECONNECTING2)
              ?RMT_RECONNECT
              :(slaves[who].remote_state == RMT_CONNECTING2)
              ?RMT_NOTCONNECTED
              :RMT_CONNECTED);
    strcpy(send_data->apcmagic, APC_MAGIC);
    strcpy(send_data->usermagic, slaves[who].usermagic);

    return;
}

/*********************************************************************
 * Send packet data to slave
 */
static int send_packet(UPSINFO *ups, int who, struct netdata *send_data)
{
    int stat = 0;
    int sent = 0;
    int tries = 10;   /* Should be a better method than this */
    int retry = 1;
    fd_set writefds;
    struct timeval timeout;
    void *bufp = (void*)send_data;

    Dmsg3(100, "Write %d bytes to slaves magic=%s usrmagic=%s\n",   
          sizeof(*send_data),
          send_data->apcmagic, send_data->usermagic);
    /* Send our data to Slave */
    /* Socket is non-blocking so we need to try a bit harder */
    do {
        do {
            stat = write(slaves[who].socket, bufp,
                         sizeof(*send_data) - sent);
        } while ((stat < 0) && (errno == EINTR));

        if ((stat < 0) && (errno == EAGAIN)) {
            /*
             * Hang around for 10 milli-seconds and retry - Once 
             */
          if (retry) {
            retry = 0;
            FD_SET(slaves[who].socket, &writefds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;
            select(slaves[who].socket + 1, NULL, &writefds, NULL,
                   &timeout);
            continue;
          }
        }

        if (stat > 0) {
            /*
             * Sent some data so we can retry again if we block
             */
            retry = 1;
            sent += stat;
            bufp = &((char*)bufp)[stat];
            stat = 0;

            if (sent < sizeof(*send_data)) {
                /*
                 * Sent some of it try again but not for more than 10 tries
                 */
                tries -= 1;
                if(tries) continue;
            }
        }

        break;
    } while (TRUE);

    if ((stat < 0)
        || ((sent != 0) && (sent != sizeof(*send_data)))) {
        /*
         * A real error
         */
        log_event(ups, LOG_WARNING,
                  "Slave write failed %s! Down slave %d",
                  strerror(errno), who);
        /*
         * If the failure happened with a connected slave
         * then try again immediately
         * Otherwise leave it alone for a while
         */
        if (slaves[who].remote_state == RMT_CONNECTED) {
          slaves[who].remote_state = RMT_RECONNECT;
          slave_disconnected = TRUE;
          close_slave(ups, who);
        } else
          down_slave(ups, who);
        stat = 4;
    } else if (sent == 0) {
      Dmsg1(100, "Slave %d write stalled\n", who);
      stat = 7;
    }

    return stat;
}

/*********************************************************************
 * Receive data from slave
 * This can be called inside send to slave if we have started
 * the conversation and the slave responds fast enough - msec's
 * Otherwise it is called from the slave poll loop, when we should
 * be resuming a conversation in the middle
 */
static int receive_from_slave(UPSINFO *ups, int who, int rok)
{
    int stat;
    struct netdata read_data;

    if (!rok) {
        fd_set rfds;
        struct timeval timeout;

        FD_ZERO(&rfds);
        FD_SET(slaves[who].socket, &rfds);
        timeout.tv_sec=0;
        timeout.tv_usec = 0;

        if(select(slaves[who].socket + 1, &rfds, NULL, NULL, &timeout) < 0) {
            log_event(ups, LOG_WARNING,
                      "Select on slave %d socket %d failed - %s",
                      who, slaves[who].socket, strerror(errno));
            close_slave(ups, who);
            if (slaves[who].remote_state == RMT_RECONNECTING3)
              slaves[who].remote_state = RMT_RECONNECT;
            else
              slaves[who].remote_state = RMT_NOTCONNECTED;
            return 5;
        }

        if (!FD_ISSET(slaves[who].socket, &rfds)) {
            /*
             * Cannot read now mark as stalled
             */
          Dmsg1(100, "Slave %d read stalled - no data available\n", who);
          return 7;
        }
    }
    Dmsg1(100, "Enter receive_from_slave %d\n", who);
    stat = receive_packet(ups, who, &read_data);
    Dmsg2(100, "Receive_packet from %d stat = %d\n", who, stat);
    if (!stat) {
        stat = process_packet(ups, who, &read_data);
        Dmsg2(100, "Process_packet from %d stat = %d\n", who, stat);
    }
    if (!stat) {
        log_event(ups, LOG_WARNING,
                  "Connect to slave %s succeeded", slaves[who].name);
        slaves[who].remote_state = RMT_CONNECTED;
    }
    return stat;
}

/*********************************************************************
 * Receive packet from a slave
 */
static int receive_packet(UPSINFO *ups, int who, struct netdata *read_data)
{
    int stat = 0;
    int received = 0;
    int tries = 10;    /* Should be a better method than this */
    int retry = 1;
    fd_set readfds;
    struct timeval timeout;
    void *bufp = (void*)read_data;

    Dmsg1(100, "Enter receive_packet %d\n", who);

    read_data->apcmagic[0] = 0;
    read_data->usermagic[0] = 0;
    /*
     * Socket is non-blocking so no-need to select first just try to get
     * some data - EAGAIN says wait a while, If we have received any
     * data we will wait for 10 milliseconds
     * Otherwise just return a good status (later state machine will
     * deal with no response from client.
     */
    do {
        do {
          Dmsg2(100, "Reading %d bytes from slave %d\n",
                sizeof(*read_data) - received, who);
          stat = read(slaves[who].socket, bufp,
                      sizeof(*read_data) - received);
          Dmsg2(100, "Slave %d read result %d\n", who, stat);
        } while ((stat < 0) && (errno = EINTR));

        if ((stat < 0) && (errno == EAGAIN)) {
            /*
             * Hang around for 10 milli-seconds and retry - Once
             */
            Dmsg1(100, "Slave %d read stalled\n", who);
            if (retry) {
                Dmsg1(100, "Wait for 10000 usecs and retry on slave %d\n",
                      who);
                retry = 0;
                FD_SET(slaves[who].socket, &readfds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;
                select(slaves[who].socket + 1, &readfds, NULL, NULL,
                       &timeout);
                Dmsg1(100, "Retry slave %d\n", who);
                continue;
            }
        }

        if (stat > 0) {
            /*
             * Received some data so we can retry again if we block
             */
            Dmsg2(100, "Slave %d received %d bytes\n", who, stat);
            retry = 1;
            received += stat;
            bufp = &((char*)bufp)[stat];
            stat = 0;

            if (received < sizeof(*read_data)) {
                /*
                 * Received some of it try again but not for more than 10 tries
                 */
                tries -= 1;
                if(tries) continue;
            }
        }

        break;
    } while (TRUE);

    if ((stat < 0)
        || ((received != 0) && (received != sizeof(*read_data)))) {
        /*
         * A real error
         */
        log_event(ups, LOG_WARNING,
                  "Slave read failed %s! Down slave %d",
                  strerror(errno), who);
        if (slaves[who].remote_state == RMT_CONNECTED) {
          slaves[who].remote_state = RMT_RECONNECT;
          slave_disconnected = TRUE;
          close_slave(ups, who);
        } else
          down_slave(ups, who);
        stat = 3;
    } else if (received == 0) {
        Dmsg1(100, "slave %d read stalled\n", who);
        stat = 7;
    }

    return stat;
}

/***********************************************************************
 * Process a packet from a slave
 */
static int process_packet(UPSINFO *ups, int who, struct netdata *read_data)
{
    int stat = 0;
  
    /*
     * We got a good read from the slave
     */
    read_data->apcmagic[APC_MAGIC_SIZE-1] = 0;
    read_data->usermagic[APC_MAGIC_SIZE-1] = 0;
    Dmsg3(100, "Read from slave %d magic=%s usr=%s\n",
          who, read_data->apcmagic, read_data->usermagic);
    if (strcmp(APC_MAGIC, read_data->apcmagic) == 0) {
        /*
         * new non-disconnecting slaves send 1000 back in the
         * UPS_CHANGEBATT field
         */
        slaves[who].disconnecting_slave
          = ntohl(read_data->ChangeBatt) != 1000;
        Dmsg3(100,
              "Slave %s disconnecting_slave=%d ChangeBatt value=%d\n",
              slaves[who].name, slaves[who].disconnecting_slave,
              ntohl(read_data->ChangeBatt));
        if (slaves[who].remote_state == RMT_CONNECTING3) {
            strcpy(slaves[who].usermagic, read_data->usermagic);
        } else {
            if (strcmp(slaves[who].usermagic, read_data->usermagic) != 0) {
                Dmsg3(100, "Bad magic from slave %d, our %s, his %s\n",
                      who, slaves[who].usermagic, read_data->usermagic);
                stat = 3;     /* bad magic */
                down_slave(ups, who);
            }
        }
    } else {
        Dmsg3(100, "Bad magic from slave %d, our %s, his %s\n",
              who, APC_MAGIC, read_data->apcmagic);
        stat = 3;             /* bad magic */
        down_slave(ups, who);
    }
      
    return stat;
}

/*********************************************************************
 * Check slave status
 * Called when a slave is not ready to write, cannot be read but is in
 * an exception state - usually a connection will have dropped off
 * This can also be called when trying to read or write to a slave and
 * things go wrong - in any such case we mark the slave as not connected
 * and try again later
 */
static int check_slave(UPSINFO *ups, int who)
{
    if (slaves[who].remote_state == RMT_CONNECTED) {
        slaves[who].remote_state = RMT_RECONNECT;
        slave_disconnected = TRUE;
        close_slave(ups, who);
    } else
        down_slave(ups, who);
    return 5;
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
 *          0 OK
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

    /*
     * Turn on keep alive code 
     */
    if (setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &turnon, sizeof(turnon)) < 0) {
        log_event(ups, LOG_WARNING, "Cannot set SO_KEEPALIVE on socket: %s\n" , strerror(errno));
    }

    /* memset is more portable than bzero */
    memset((char *) &my_adr, 0, sizeof(struct sockaddr_in));
    my_adr.sin_family = AF_INET;
    my_adr.sin_addr = local_ip;
    my_adr.sin_port = htons(ups->NetUpsPort);

    bound = FALSE;
    for (i=0; i<30; i++) {        /* retry every 30 seconds for 15 minutes */
        if (bind(socketfd, (struct sockaddr *) &my_adr, sizeof(my_adr)) < 0) {
            log_event(ups, LOG_WARNING, "Cannot bind to port %d, retrying ...", ups->NetUpsPort);
            sleep(30);
        } else {
            bound = TRUE;
            break;
        }
    }
    if (!bound) {
        log_event(ups, LOG_ERR, "Cannot bind port %d, probably already in use", ups->NetUpsPort);
        Dmsg1(100, "close master %d\n", socketfd);
        close(socketfd);
        return 1;
    }

    if (listen(socketfd, 1) == -1) {
        log_event(ups, LOG_ERR, "Listen Failure");
        Dmsg1(100, "close master %d\n", socketfd);
        close(socketfd);
        return 1;
    }

    Dmsg1(100, "Slave listening on port %d\n", ups->NetUpsPort);
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
 *          non-zero is error, see update_from_master();
 */
static int get_data_from_master(UPSINFO *ups) 
 {
    int stat;
    int ShutDown;

    fd_set rfds;
    struct timeval tv;

    if (newsocketfd == -1) {
        set_ups(UPS_COMMLOST);
        masterlen = sizeof(master_adr);
select_again2:
        FD_ZERO(&rfds);
        FD_SET(socketfd, &rfds);
        tv.tv_sec = MASTER_TIMEOUT;   /* 120 secs */
        tv.tv_usec = 0;
        Dmsg0(100, "get_data_from_master on select before accept.\n");
        switch (select(socketfd+1, &rfds, NULL, NULL, &tv)) {
        case 0:              /* No chars available in 2 minutes. */
        case -1:             /* error */
            if (errno == EINTR) {
               goto select_again2;
            }
            slaves[0].remote_state = RMT_ERROR;
            Dmsg0(100, "select timeout return 6.\n"); 
            return 6;
        default:
            break;
        }
        Dmsg0(100, "Doing accept\n");
        do {
           newsocketfd = accept(socketfd, (struct sockaddr *) &master_adr, 
                                &masterlen);
        } while (newsocketfd < 0 && errno == EINTR);
        Dmsg1(100, "Accept returned %d\n", newsocketfd);
        if (newsocketfd < 0) {
            slaves[0].remote_state = RMT_DOWN;
            slaves[0].down_time = time(NULL);
            slaves[0].ms_errno = errno;
            newsocketfd = -1;
            Dmsg1(100, "Accept error: %s\n", strerror(errno));
            return 1;
        }
        Dmsg0(100, "Done with accept()\n");

#ifdef HAVE_LIBWRAP
        /*
         * This function checks the incoming client and if it's not
         * allowed closes the connection.
         */
        if (check_wrappers(argvalue, newsocketfd) == FAILURE) {
            slaves[0].remote_state = RMT_DOWN;
            slaves[0].down_time = time(NULL);
            shutdown(newsocketfd, SHUT_RDWR);
            Dmsg1(100, "close newsock %d\n", newsocketfd);
            close(newsocketfd);
            newsocketfd = -1;
            Dmsg0(100, "Wrappers reject return 2\n");
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
            Dmsg1(100, "close newsock %d\n", newsocketfd);
            close(newsocketfd);
            newsocketfd = -1;
            Dmsg0(100, "Remove address does not correspond. return 2\n");
            return 2;
        }
    } /* end if newsocketfd */

select_again3:
    FD_ZERO(&rfds);
    FD_SET(newsocketfd, &rfds);
    tv.tv_sec = MASTER_TIMEOUT;
    tv.tv_usec = 0;
    Dmsg1(100, "Doing select to get master data on %d.\n", newsocketfd);
    switch (select(newsocketfd+1, &rfds, NULL, NULL, &tv)) {
    case 0:              /* No chars available in 2 minutes. */
    case -1:             /* error */
        if (errno == EINTR) {
           goto select_again3;
        }
        Dmsg2(100, "close newsock %d socket err=%s\n", newsocketfd,
           strerror(errno));
        slaves[0].remote_state = RMT_ERROR;
        shutdown(newsocketfd, SHUT_RDWR);
        Dmsg1(100, "close newsock %d\n", newsocketfd);
        close(newsocketfd);
        newsocketfd = -1;
        set_ups(UPS_COMMLOST);
        Dmsg0(100, "select timed out return 6");
        return 6;                 /* master not responding */
    default:
        break;
    }
    Dmsg0(100, "Doing read from master.\n");
    do {
        stat=read(newsocketfd, &get_data, sizeof(get_data));
    } while (stat < 0 && errno == EINTR);
    if (stat != sizeof(get_data)){
        get_data.apcmagic[APC_MAGIC_SIZE-1] = 0;
        get_data.usermagic[APC_MAGIC_SIZE-1] = 0;
        slaves[0].remote_state = RMT_ERROR;
        slaves[0].ms_errno = errno;
        Dmsg3(100, "Read error fd=%d stat=%d err: %s\n", 
           newsocketfd, stat, strerror(errno));
        shutdown(newsocketfd, SHUT_RDWR);
        Dmsg1(100, "close newsock after read %d\n", newsocketfd);
        close(newsocketfd);
        newsocketfd = -1;
        set_ups(UPS_COMMLOST);
        return 3;                 /* read error */
    }
    get_data.apcmagic[APC_MAGIC_SIZE-1] = 0;
    get_data.usermagic[APC_MAGIC_SIZE-1] = 0;

    /* At this point, we read something */

    Dmsg1(100, "Got something stat=%d.\n", stat);
    if (strcmp(APC_MAGIC, get_data.apcmagic) != 0) { 
        slaves[0].remote_state = RMT_ERROR;
        shutdown(newsocketfd, SHUT_RDWR);
        Dmsg1(100, "close newsock %d\n", newsocketfd);
        close(newsocketfd);
        newsocketfd = -1;
        set_ups(UPS_COMMLOST);
        Dmsg2(100, "magic does not compare: mine %s\nhis %s\n",
           APC_MAGIC, get_data.apcmagic);
        return 4;
    }
          
    stat = ntohl(get_data.remote_state);
    /* If not connected, send him our user magic */
    if (stat == RMT_NOTCONNECTED || stat == RMT_RECONNECT) {
        strcpy(get_data.apcmagic, APC_MAGIC);
        strcpy(get_data.usermagic, ups->usermagic);
        get_data.ChangeBatt = htonl(1000); /* flag to say we are non-disconnecting */
        Dmsg1(100, "Slave sending non-disconnecting flag = %d.\n",
           ntohl(get_data.ChangeBatt));
        do {
           stat = write(newsocketfd, &get_data, sizeof(get_data));
        } while (stat < 0 && errno == EINTR);
    }

    if (strcmp(ups->usermagic, get_data.usermagic) == 0) {
        Dmsg0(100, "Got good data\n");
        if (ntohl(get_data.OnBatt)) {
            clear_ups_online();
        } else {
            set_ups_online();
        }
        if (ntohl(get_data.BattLow)) {
            set_ups(UPS_BATTLOW);
        } else {
            clear_ups(UPS_BATTLOW);
        }
        ups->BattChg       = ntohl(get_data.BattChg);
        ShutDown           = ntohl(get_data.ShutDown);
        ups->nettime       = ntohl(get_data.nettime);
        ups->TimeLeft      = ntohl(get_data.TimeLeft);
/*
 * Setting ChangeBatt triggers false alarms if the master goes
 * down and comes back up, so remove it for now.  KES 27Feb01
 *
 *      if (ntohl(get_data.ChangeBatt)) {
 *          set_ups(UPS_REPLACEBATT);
 *      } else {
 *          clear_ups(UPS_REPLACEBATT);
 *      }
 */
        if (ntohl(get_data.load)) {
            set_ups(UPS_SHUT_LOAD);
        } else {
            clear_ups(UPS_SHUT_LOAD);
        }
        if (ntohl(get_data.timedout)) {
            set_ups(UPS_SHUT_BTIME);
        } else {
            clear_ups(UPS_SHUT_BTIME);
        }
        if (ntohl(get_data.timelout)) {
            set_ups(UPS_SHUT_LTIME);
        } else {
            clear_ups(UPS_SHUT_LTIME);
        }
        if (ntohl(get_data.emergencydown)) {
            set_ups(UPS_SHUT_EMERG);
        } else {
            clear_ups(UPS_SHUT_EMERG);
        }
        ups->remote_state  = ntohl(get_data.remote_state);
        ups->UPS_Cap[CI_BATTLEV] = ntohl(get_data.cap_battlev);
        ups->UPS_Cap[CI_RUNTIM] = ntohl(get_data.cap_runtim);
    } else {
        Dmsg2(100, "User magic does not compare: mine %s his %s\n",
               ups->usermagic, get_data.usermagic);

        slaves[0].remote_state = RMT_ERROR;
        shutdown(newsocketfd, SHUT_RDWR);
        Dmsg1(100, "close newsock %d\n", newsocketfd);
        close(newsocketfd);
        newsocketfd = -1;
        set_ups(UPS_COMMLOST);
        return 5;
    }

    if (ShutDown) {                 /* if master has shutdown */
        set_ups(UPS_SHUT_REMOTE); /* we go down too */
    }
         
    /* 
     * Note if UPS_COMMLOST is set at this point, it is because it
     * was previously detected. If we get here, we have a good
     * connection, so generate the event and clear the flag.
     */
    if (is_ups_set(UPS_COMMLOST)) {
        log_event(ups, LOG_WARNING, "Connect from master %s succeeded",
                slaves[0].name);
        execute_command(ups, cmd[CMDMASTERCONN]);
        Dmsg0(100, "Clear UPS_COMMLOST\n");
        clear_ups(UPS_COMMLOST);
    } 
    slaves[0].remote_state = RMT_CONNECTED;
    ups->last_master_connect_time = time(NULL);
    Dmsg0(100, "Exit get_from_master\n");
    return 0;
}

/*********************************************************************
 *
 * Called from slave to get data from the master 
 * Returns: 0 on error
 *          1 OK
 */
static int update_from_master(UPSINFO *ups)
{
    int stat; 
    Dmsg0(100, "Enter update_from_master\n");
    stat =  get_data_from_master(ups);
    Dmsg1(100, "get_data_from_master=%d\n", stat);
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
    if (stat != 0) {
       slaves[0].errorcnt++;
    }
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

    set_ups(UPS_SLAVE);       /* We are networking not connected */

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
    time_t sleeptime = ups->nettime;
    int slave_stat; 
      
    init_thread_signals();

    while (1) {
        now = time(NULL);
        Dmsg1(100, "do_slaves starting loop %d\n", now);

        /* Send info to slaves if nettime expired (set by apcupsd.conf)
         * or in power fail situation (FASTPOLL) or some slave is 
         * not connected (slave_disconnected)
         */
        if (((now - last_time_net) > ups->nettime)
            || is_ups_set(UPS_FASTPOLL)
            || slave_disconnected) {;
            slave_stat = slave_disconnected;
            Dmsg0(100, "do_slaves calling send_to_all_slaves\n");
            send_to_all_slaves(ups);
            last_time_net = time(NULL);
            Dmsg1(100, "do_slaves, send_to_all_slaves return %d\n",
                  last_time_net);
            /*
             * If change in slave status, update Status
             */
            if (slave_stat != slave_disconnected) {
                Dmsg2(100, "do_slaves slave status changed from %d to %d\n",
                      slave_stat, slave_disconnected);
                write_lock(ups);
                if (slave_disconnected) {
                   set_ups(UPS_SLAVEDOWN);
                } else {
                   clear_ups(UPS_SLAVEDOWN);
                }
                write_unlock(ups);
            }
        }

        /*
         * Sleep up to the next nettime window
         * if it has already run out just go round again
         */
        if (is_ups_set(UPS_FASTPOLL)) {
            sleeptime = 1;
        } else {
            sleeptime = ups->nettime - (last_time_net - now); 
        }

        if (sleeptime <= 0) {
          Dmsg0(100, "do_slaves not sleeping nettime has already run out\n");
          continue;
        }

        Dmsg1(100, "do_slaves sleeping for %d seconds\n", sleeptime);
        sleep(sleeptime);
        Dmsg0(100, "do_slaves woke up\n");
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
