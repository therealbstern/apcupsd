/*
 *  net.c -- network driver
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c		  -- Simple Daemon to catch power failure signals from a
 *		       BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		    -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  All rights reserved.
 *
 */

/*
 *			   GNU GENERAL PUBLIC LICENSE
 *			      Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *				 675 Mass Ave, Cambridge, MA 02139, USA
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
#include "net.h"

/*
 * List of variables that can be read by getupsvar()   
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS 
 * output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 * line, and if 1 returns only to first space (e.g. integers,
 * and floating point values.
 */
static struct { 	
   char *request;
   char *upskeyword;
   int nfields;
} cmdtrans[] = {
   {"battcap",    "BCHARGE",  1},
   {"battdate",   "BATTDATE", 1},
   {"battpct",    "BCHARGE",  1},
   {"battvolt",   "BATTV",    1},
   {"cable",      "CABLE",    0},
   {"date",       "DATE",     0},
   {"firmware",   "FIRMWARE", 0},
   {"highxfer",   "HITRANS",  1},
   {"hostname",   "HOSTNAME", 1},
   {"laststest",  "LASTSTEST",0},
   {"lastxfer",   "LASTXFER", 0},      /* reason for last xfer to batteries */
   {"linemax",    "MAXLINEV", 1},
   {"linemin",    "MINLINEV", 1},
   {"loadpct",    "LOADPCT",  1},
   {"lowbatt",    "DLOWBATT", 1},      /* low battery power off delay */
   {"lowxfer",    "LOTRANS",  1},
   {"maxtime",    "MAXTIME",  1},
   {"mbattchg",   "MBATTCHG", 1},
   {"mintimel",   "MINTIMEL", 1},
   {"model",      "MODEL",    0},
   {"nombattv",   "NOMBATTV", 1},
   {"outputfreq", "LINEFREQ", 1},
   {"outputv",    "OUTPUTV",  1},
   {"release",    "RELEASE",  1},
   {"retpct",     "RETPCT",   1},      /* min batt to turn on UPS */
   {"runtime",    "TIMELEFT", 1},
   {"selftest",   "SELFTEST", 1},      /* results of last self test */
   {"sense",      "SENSE",    1},
   {"serialno",   "SERIALNO", 1},
   {"status",     "STATFLAG", 1},
   {"transhi",    "HITRANS",  1},
   {"translo",    "LOTRANS",  1},
   {"upsload",    "LOADPCT",  1},
   {"upsmode",    "UPSMODE",  0},
   {"upsname",    "UPSNAME",  1},
   {"upstemp",    "ITEMP",    1},
   {"utility",    "LINEV",    1},
   {NULL, NULL}
};



/**********************************************************************
 * Set appropriate internal variables based on a Status word filled by
 * the UPS driver.
 *********************************************************************/
 /* *****FIXME**** there should be only one global copy of this */
static void test_status_bits(UPSINFO *ups)
{
    /*
     * Here we change UPSINFO values so lock the area.
     */
    write_lock(ups);

    if (ups->Status & UPS_ONBATT)
	ups->OnBatt = 1;	      /* On battery power */
    else
	ups->OnBatt = 0;
    if (ups->Status & UPS_BATTLOW)
	ups->BattLow = 1;		/* battery low */ 
    else
	ups->BattLow = 0;
    if (ups->Status & UPS_SMARTBOOST)
	ups->LineLevel = -1;		      /* LineVoltage Low */
    else if (ups->Status & UPS_SMARTTRIM)
	ups->LineLevel = 1;		    /* LineVoltage High */
    else
	ups->LineLevel = 0;		    /* LineVoltage Normal */

    if (ups->Status & UPS_REPLACEBATT) { /* Replace Battery */
	if (!ups->ChangeBatt) {       /* This is a counter, so check before */
	   ups->ChangeBatt = 1;       /* setting it */
	}
    }
    if (ups->Status & UPS_SHUTDOWN) {
       ups->remotedown = 1;	      /* if master is shutting down so do we */
    }
    write_unlock(ups);
}

/*
 * The remote server DEVICE entry in apcupsd.conf is
 * in the form:
 *
 * DEVICE hostname[:port]
 *
 */
static int initialize_device_data(UPSINFO *ups) {
    struct net_ups_internal_data *Nid = ups->driver_internal_data;
    char *cp;

    strcpy(Nid->device, ups->device);

    /*
     * Now split the device.
     */
    Nid->hostname = Nid->device;

    cp = strchr(Nid->device, ':');
    if (cp) {
        *cp = '\0';
	cp++;
	Nid->port = atoi(cp);
    } else {
	Nid->port = ups->statusport;  /* use NIS port as default */
    }

    Nid->statbuf[0] = '\0';
    Nid->statlen = BIGBUF;

    return SUCCESS;
}

/*
 * Returns 1 if var found
 *   answer has var
 * Returns 0 if variable name not found
 *   answer has "Not found" is variable name not found
 *   answer may have "N/A" if the UPS does not support this
 *	     feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
static int getupsvar(UPSINFO *ups, char *request, char *answer, int anslen)
{
    struct net_ups_internal_data *Nid = ups->driver_internal_data;
    int i;
    char *stat_match = NULL;
    char *find;
    int nfields = 0;
     
    for (i=0; cmdtrans[i].request; i++) 
	if (!(strcmp(cmdtrans[i].request, request))) {
	     stat_match = cmdtrans[i].upskeyword;
	     nfields = cmdtrans[i].nfields;
	}

    if (stat_match) {
	if ((find=strstr(Nid->statbuf, stat_match)) != NULL) {
	     if (nfields == 1) {      /* get one field */
                 sscanf (find, "%*s %*s %s", answer);
	     } else {			  /* get everything to eol */
		 i = 0;
		 find += 11;  /* skip label */
                 while (*find != '\n')
		     answer[i++] = *find++;
		 answer[i] = 0;
	     }
             if (strcmp(answer, "N/A") == 0) {
		 return 0;
	     }
             Dmsg2(100, "Return 1 for getupsvar %s %s\n", request, answer);
	     return 1;
	}
    } else {
       Dmsg1(100, "HEY!!! No match in getupsvar for %s!\n", request);
    }

    strcpy(answer, "Not found");
    return 0;
}

/*
 * Fill buffer with data from UPS network daemon   
 * Returns 0 on error
 * Returns 1 if OK
 */
static int fill_status_buffer(UPSINFO *ups) 
{
    struct net_ups_internal_data *Nid = ups->driver_internal_data;
    int n, stat = 1;
    char buf[1000];
    static time_t last_fill_time = 0;
    time_t now;

    /* Poll or fill the buffer maximum one time per second */
    now = time(NULL);
    if (now - last_fill_time < 2) {
       return 1;
    }
    last_fill_time = now;

    Nid->statbuf[0] = 0;
    Nid->statlen = 0;
    Dmsg2(20, "Opening connection to %s:%d\n",
	    Nid->hostname, Nid->port);
    if ((Nid->sockfd = net_open(Nid->hostname, NULL, Nid->port)) < 0) {
        log_event(ups, LOG_ERR, "fetch_data: tcp_open failed for %s port %d",
		Nid->hostname, Nid->port);
	return 0;
    }

    if (net_send(Nid->sockfd, "status", 6) != 6) {
        log_event(ups, LOG_ERR, "fill_buffer: write error on socket.");
	return 0;
    }

    Dmsg0(99, "===============\n");
    while ((n = net_recv(Nid->sockfd, buf, sizeof(buf)-1)) > 0) {
	buf[n] = 0;
	strcat(Nid->statbuf, buf);
        Dmsg3(99, "Partial buf (%d, %d):\n%s",n,strlen(Nid->statbuf), buf);
    }
    Dmsg0(99, "===============\n");

    if (n < 0) {
	stat = 0;
    }

    net_close(Nid->sockfd);

    Dmsg1(99, "Buffer:\n%s\n", Nid->statbuf);

    Nid->statlen = strlen(Nid->statbuf);
    return stat;
}

static int get_ups_status_flag(UPSINFO *ups, int fill)
{
    char answer[200];
    int stat = 1;

    write_lock(ups);

    if (fill) {
       fill_status_buffer(ups);
    }

    answer[0] = 0;
    if (!getupsvar(ups, "status", answer, sizeof(answer))) {
        log_event(ups, LOG_ERR, "getupsvar: failed for ""status"".");
        Dmsg0(100, "HEY!!! Couldn't get status flag.\n");
	stat = 0;
    } else {
       ups->Status = strtol(answer, NULL, 0);
    }
    Dmsg2(100, "Got Status = %s %03x\n", answer, ups->Status);

    write_unlock(ups);

    test_status_bits(ups);

    /* If we lost connection with master and we
     * are running on batteries, shutdown now
     */
    if (stat == 0 && ups->OnBatt) {
       write_lock(ups);
       ups->remotedown = TRUE;
       write_unlock(ups);
    }
    return stat;
}


int net_ups_open(UPSINFO *ups) 
{
    struct net_ups_internal_data *Nid;

    Nid = malloc(sizeof(struct net_ups_internal_data));
    if (Nid == NULL) {
        log_event(ups, LOG_ERR, "Out of memory.");
	exit(1);
    }
    memset(Nid, 0, sizeof(struct net_ups_internal_data));

    ups->driver_internal_data = Nid;

    initialize_device_data(ups);

    /*
     * Fake core code. Will go away when ups->fd will be cleaned up.
     */
    ups->fd = 1;

    return 1;
}

int net_ups_close(UPSINFO *ups) 
{
    free(ups->driver_internal_data);
    /*
     * Fake core code. Will go away when ups->fd will be cleaned up.
     */
    ups->fd = -1;
    return 1;
}

int net_ups_setup(UPSINFO *ups) 
{
    /*
     * Nothing to setup.
     */
    return 1;
}

int net_ups_get_capabilities(UPSINFO *ups) 
{
    char answer[200];

    write_lock(ups);

    fill_status_buffer(ups);   

    ups->UPS_Cap[CI_VLINE] = getupsvar(ups, "utility", answer, sizeof(answer));
    ups->UPS_Cap[CI_LOAD] = getupsvar(ups, "loadpct", answer, sizeof(answer));
    ups->UPS_Cap[CI_BATTLEV] =
        getupsvar(ups, "battcap", answer, sizeof(answer));
    ups->UPS_Cap[CI_RUNTIM] =
        getupsvar(ups, "runtime", answer, sizeof(answer));
    ups->UPS_Cap[CI_VMAX] = getupsvar(ups, "linemax", answer, sizeof(answer));
    ups->UPS_Cap[CI_VMIN] = getupsvar(ups, "linemin", answer, sizeof(answer));
    ups->UPS_Cap[CI_VOUT] = getupsvar(ups, "outputv", answer, sizeof(answer));
    ups->UPS_Cap[CI_SENS] = getupsvar(ups, "sense", answer, sizeof(answer));
    ups->UPS_Cap[CI_DLBATT] =
        getupsvar(ups, "lowbatt", answer, sizeof(answer));
    ups->UPS_Cap[CI_LTRANS] =
        getupsvar(ups, "lowxfer", answer, sizeof(answer));
    ups->UPS_Cap[CI_HTRANS] =
        getupsvar(ups, "highxfer", answer, sizeof(answer));
    ups->UPS_Cap[CI_RETPCT] =
        getupsvar(ups, "retpct", answer, sizeof(answer));
    ups->UPS_Cap[CI_ITEMP] =
        getupsvar(ups, "upstemp", answer, sizeof(answer));
    ups->UPS_Cap[CI_VBATT] =
        getupsvar(ups, "battvolt", answer, sizeof(answer));
    ups->UPS_Cap[CI_FREQ] =
        getupsvar(ups, "outputfreq", answer, sizeof(answer));
    ups->UPS_Cap[CI_WHY_BATT] =
        getupsvar(ups, "lastxfer", answer, sizeof(answer));
    ups->UPS_Cap[CI_ST_STAT] =
        getupsvar(ups, "selftest", answer, sizeof(answer));
    ups->UPS_Cap[CI_SERNO] =
        getupsvar(ups, "serialno", answer, sizeof(answer));
    ups->UPS_Cap[CI_BATTDAT] =
        getupsvar(ups, "battdate", answer, sizeof(answer));
    ups->UPS_Cap[CI_NOMBATTV] =
        getupsvar(ups, "nombattv", answer, sizeof(answer));
    ups->UPS_Cap[CI_REVNO] =
        getupsvar(ups, "firmware", answer, sizeof(answer));
       
    write_unlock(ups);
    return 1;
}

int net_ups_program_eeprom(UPSINFO *ups) 
{
    return 0;
}

int net_ups_kill_power(UPSINFO *ups) 
{
    /* Not possible */
    return 0;
}

int net_ups_check_state(UPSINFO *ups) 
{
    int sleep_time;

    sleep_time = ups->wait_time;
    if (ups->nettime && ups->nettime < ups->wait_time) {
       sleep_time = ups->nettime;
    }
    sleep(sleep_time);

    get_ups_status_flag(ups, 1);

    return 1;
}

int net_ups_read_volatile_data(UPSINFO *ups) 
{
    char answer[200];

    write_lock(ups);

    /* ***FIXME**** poll time needs to be scanned */
    ups->poll_time = time(NULL);
    ups->last_master_connect_time = ups->poll_time;
    fill_status_buffer(ups);

    if (ups->UPS_Cap[CI_VLINE] && 
          getupsvar(ups, "utility", answer, sizeof(answer))) {
	ups->LineVoltage = atof(answer);
    }
    if (ups->UPS_Cap[CI_LOAD] && 
          getupsvar(ups, "loadpct", answer, sizeof(answer))) {
	ups->UPSLoad = atof(answer);
    }
    if (ups->UPS_Cap[CI_BATTLEV] && 
          getupsvar(ups, "battcap", answer, sizeof(answer))) {
	ups->BattChg = atof(answer);
    }
    if (ups->UPS_Cap[CI_RUNTIM] && 
          getupsvar(ups, "runtime", answer, sizeof(answer))) {
	ups->TimeLeft = atof(answer);
    }
    if (ups->UPS_Cap[CI_VMAX] && 
          getupsvar(ups, "linemax", answer, sizeof(answer))) {
	ups->LineMax = atof(answer);
    }
    if (ups->UPS_Cap[CI_VMIN] && 
          getupsvar(ups, "linemin", answer, sizeof(answer))) {
	ups->LineMin = atof(answer);
    }
    if (ups->UPS_Cap[CI_VOUT] && 
          getupsvar(ups, "outputv", answer, sizeof(answer))) {
	ups->OutputVoltage = atof(answer);
    }
    if (ups->UPS_Cap[CI_SENS] && 
          getupsvar(ups, "sense", answer, sizeof(answer))) {
	ups->sensitivity[0] = answer[0];
    }
    if (ups->UPS_Cap[CI_DLBATT] && 
          getupsvar(ups, "lowbatt", answer, sizeof(answer))) {
	ups->dlowbatt = atof(answer);
    }
    if (ups->UPS_Cap[CI_LTRANS] && 
          getupsvar(ups, "lowxfer", answer, sizeof(answer))) {
	ups->lotrans = atof(answer);
    }
    if (ups->UPS_Cap[CI_HTRANS] && 
          getupsvar(ups, "highxfer", answer, sizeof(answer))) {
	ups->hitrans = atof(answer);
    }
    if (ups->UPS_Cap[CI_RETPCT] && 
          getupsvar(ups, "retpct", answer, sizeof(answer))) {
	ups->rtnpct = atof(answer);
    }
    if (ups->UPS_Cap[CI_ITEMP] && 
          getupsvar(ups, "upstemp", answer, sizeof(answer))) {
	ups->UPSTemp = atof(answer);
    }
    if (ups->UPS_Cap[CI_VBATT] && 
          getupsvar(ups, "battvolt", answer, sizeof(answer))) {
	ups->BattVoltage = atof(answer);
    }
    if (ups->UPS_Cap[CI_FREQ] && 
          getupsvar(ups, "outputfreq", answer, sizeof(answer))) {
	ups->LineFreq = atof(answer);
    }

    if (ups->UPS_Cap[CI_VBATT] && 
          getupsvar(ups, "battvolt", answer, sizeof(answer))) {
        if (!strcmp(answer, "No transfers since turnon"))
            ups->G[0] = 'O';
        if (!strcmp(answer, "Automatic or explicit self test"))
            ups->G[0] = 'S';
        if (!strcmp(answer, "Low line voltage"))
            ups->G[0] = 'L';
        if (!strcmp(answer, "High line voltage"))
            ups->G[0] = 'H';
        if (!strcmp(answer, "Line voltage notch or spike"))
            ups->G[0] = 'T';
        if (!strcmp(answer, "Unacceptable line voltage changes"))
            ups->G[0] = 'R';
        if (!strncmp(answer, "UNKNOWN EVENT", 13))
	    ups->G[0] = *(answer+15);
    }
    if (getupsvar(ups, "selftest", answer, sizeof(answer))) {
	strcpy(ups->X, answer);
    }

    write_unlock(ups);

    get_ups_status_flag(ups, 0);

    return 1;
}

int net_ups_read_static_data(UPSINFO *ups) 
{
    char answer[200];

    write_lock(ups);

    fill_status_buffer(ups);

    if (!getupsvar(ups, "model", ups->mode.long_name,
		sizeof(ups->mode.long_name))) {
        log_event(ups, LOG_ERR, "getupsvar: failed for \"model\".");
    }
    if (!getupsvar(ups, "upsmode", ups->class.long_name,
		sizeof(ups->class.long_name))) {
        log_event(ups, LOG_ERR, "getupsvar: failed for \"upsmode\".");
    }
    if (ups->UPS_Cap[CI_SERNO] &&
            getupsvar(ups, "serialno", answer, sizeof(answer))) {
	strcpy(ups->serial, answer);
    }
    if (ups->UPS_Cap[CI_BATTDAT] &&
            getupsvar(ups, "battdate", answer, sizeof(answer))) {
	strcpy(ups->battdat, answer);
    }
    if (ups->UPS_Cap[CI_NOMBATTV] &&
            getupsvar(ups, "nombattv", answer, sizeof(answer))) {
	ups->nombattv = atof(answer);
    }
    if (ups->UPS_Cap[CI_REVNO] &&
            getupsvar(ups, "firmware", answer, sizeof(answer))) {
	strcpy(ups->firmrev, answer);
    }

    write_unlock(ups);
    return 1;
}

int net_ups_entry_point(UPSINFO *ups, int command, void *data) 
{
    return 0;
}
