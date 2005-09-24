
/*
 *  apcsmart.c -- The decoding of the chatty little beasts.
 *                  THE LOOK-A-LIKE ( UPSlink(tm) Language )
 *
 *  apcupsd.c  -- Simple Daemon to catch power failure signals from a
 *                  BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *               -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 * Parts of the information below was taken from apcd.c & apcd.h
 *
 * Definitons file for APC SmartUPS daemon
 *
 *  Copyright (c) 1995 Pavel Korensky
 *  All rights reserved
 *
 *  IN NO EVENT SHALL PAVEL KORENSKY BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 *  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF PAVEL KORENSKY
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  PAVEL KORENSKY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 *  BASIS, AND PAVEL KORENSKY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 *  UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  Pavel Korensky            pavelk@dator3.anet.cz
 *
 *  8.11.1995
 *
 *  P.S. I have absolutely no connection with company APC. I didn't sign any
 *  non-disclosure agreement and I didn't got the protocol description anywhere.
 *  The whole protocol decoding was made with a small program for capturing
 *  serial data on the line. So, I think that everybody can use this software
 *  without any problem.
 *
 */

/*
   Copyright (C) 1999-2004 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#include "apc.h"
#include "apcsmart.h"

/* Convert UPS response to enum and string */
static SelfTestResult decode_testresult(char* str)
{
   /*
    * Responses are:
    * "OK" - good battery, 
    * "BT" - failed due to insufficient capacity, 
    * "NG" - failed due to overload, 
    * "NO" - no results available (no test performed in last 5 minutes) 
    */
   if (str[0] == 'O' && str[1] == 'K')
      return TEST_PASSED;
   else if (str[0] == 'B' && str[1] == 'T')
      return TEST_FAILCAP;
   else if (str[0] == 'N' && str[1] == 'G')
      return TEST_FAILLOAD;

   return TEST_NONE;
}

/* Convert UPS response to enum and string */
static LastXferCause decode_lastxfer(char *str)
{
   Dmsg1(80, "Transfer reason: %c\n", *str);

   switch (*str) {
   case 'N':
      return XFER_NA;
   case 'R':
      return XFER_RIPPLE;
   case 'H':
      return XFER_OVERVOLT;
   case 'L':
      return XFER_UNDERVOLT;
   case 'T':
      return XFER_NOTCHSPIKE;
   case 'O':
      return XFER_NONE;
   case 'K':
      return XFER_FORCED;
   case 'S':
      return XFER_SELFTEST;
   default:
      return XFER_UNKNOWN;
   }
}

int apc_enable(UPSINFO *ups)
{
   /* Enable APC Smart UPS */
   smart_poll('Y', ups);
   smart_poll('Y', ups);
   return 1;
}

/********************************************************************* 
 *
 * Send a charcter to the UPS and get
 * its response. Returns a pointer to the response string.
 *
 */
char *smart_poll(char cmd, UPSINFO *ups)
{
   static char answer[2000];
   int stat;

   *answer = 0;
   if (ups->mode.type <= SHAREBASIC)
      return answer;
   write(ups->fd, &cmd, 1);
   stat = getline(answer, sizeof answer, ups);
   /* If nothing returned, the link is probably down */
   if (*answer == 0 && stat == FAILURE) {
      UPSlinkCheck(ups);           /* wait for link to come up */
      *answer = 0;
   }

   return answer;
}

/*
 * If s == NULL we are just waiting on FD for status changes.
 * If s != NULL we are asking the UPS to tell us the value of something.
 *
 * If s == NULL there is a much more fine-grained locking.
 */
int getline(char *s, int len, UPSINFO *ups)
{
   int i = 0;
   int ending = 0;
   char c;
   int retval;

   while (!ending) {
#ifndef HAVE_CYGWIN
      fd_set rfds;
      struct timeval tv;

      FD_ZERO(&rfds);
      FD_SET(ups->fd, &rfds);
      if (s != NULL) {
         tv.tv_sec = TIMER_FAST;   /* 1 sec, expect fast response */
      } else {
         tv.tv_sec = ups->wait_time;
      }
      tv.tv_usec = 0;

      errno = 0;
      retval = select((ups->fd) + 1, &rfds, NULL, NULL, &tv);

      switch (retval) {
      case 0:                     /* No chars available in TIMER seconds. */
         return FAILURE;
      case -1:
         if (errno == EINTR || errno == EAGAIN) {       /* assume SIGCHLD */
            continue;
         }
         Error_abort1("Select error on UPS FD. %s\n", strerror(errno));
         break;
      default:
         break;
      }

#endif

      do {
         retval = read(ups->fd, &c, 1);
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));
      if (retval == 0) {
         return FAILURE;
      }

      switch (c) {
         /*
          * Here we can be called in two ways:
          * 
          * s == NULL
          *     The shm lock is not held so we must hold it here.
          *
          * s != NULL
          *     We are called from a routine that have 
          *     already held the shm lock so no need to hold it
          *     another time. Simply update the UPS structure
          *     fields and the shm will be updated when
          *     write_unlock is called by the calling
          *     routine.
          *
          * If something changes on the UPS, a special character is
          * sent over the serial line but no \n\r sequence is sent:
          * only a single character. This way if s == NULL, if we
          * receive a character like this we must return immediately
          * and not wait for a string completion.
          */
      case UPS_ON_BATT:           /* UPS_ON_BATT = '!'   */
         if (s == NULL)
            write_lock(ups);
         ups->clear_online();
         Dmsg0(80, "Got UPS ON BATT.\n");
         if (s == NULL) {
            write_unlock(ups);
            ending = 1;
         }
         break;
      case UPS_REPLACE_BATTERY:   /* UPS_REPLACE_BATTERY = '#'   */
         if (s == NULL)
            write_lock(ups);
         ups->set_replacebatt();
         if (s == NULL) {
            write_unlock(ups);
            ending = 1;
         }
         break;
      case UPS_ON_LINE:           /* UPS_ON_LINE = '$'   */
         if (s == NULL)
            write_lock(ups);
         ups->set_online();
         Dmsg0(80, "Got UPS ON LINE.\n");
         if (s == NULL) {
            write_unlock(ups);
            ending = 1;
         }
         break;
      case BATT_LOW:              /* BATT_LOW    = '%'   */
         if (s == NULL)
            write_lock(ups);
         ups->set_battlow();
         if (s == NULL) {
            write_unlock(ups);
            ending = 1;
         }
         break;
      case BATT_OK:               /* BATT_OK     = '+'   */
         if (s == NULL)
            write_lock(ups);
         ups->clear_battlow();
         if (s == NULL) {
            write_unlock(ups);
            ending = 1;
         }
         break;

      case UPS_EPROM_CHANGE:      /* UPS_EPROM_CHANGE = '|' */
      case UPS_TRAILOR:           /* UPS_TRAILOR = ':'      */
         break;

         /* NOTE: The UPS terminates what it sends to us
          * with a \r\n. Thus the line feed signals the
          * end of what we are to receive.
          */
      case UPS_LF:                /* UPS_LF      = '\n'  */
         if (s != NULL)
            ending = 1;            /* This what we waited for */
         break;
      case UPS_CR:                /* UPS_CR      = '\r'  */
         break;
      default:
         if (s != NULL) {
            if (i + 1 < len)
               s[i++] = c;
            else
               ending = 1;         /* no more room in buffer */
         }
         break;
      }
   }

   if (s != NULL) {
      s[i] = '\0';
   }
   return SUCCESS;
}

/*********************************************************************/
/* Note this routine MUST be called with the UPS write lock held! */
void UPSlinkCheck(UPSINFO *ups)
{
   struct timeval now, prev;
   static int linkcheck = FALSE;

   if (linkcheck)
      return;

   linkcheck = TRUE;               /* prevent recursion */

   tcflush(ups->fd, TCIOFLUSH);
   if (strcmp(smart_poll('Y', ups), "SM") == 0) {
      linkcheck = FALSE;
      ups->clear_commlost();
      return;
   }

   ups->set_commlost();
   tcflush(ups->fd, TCIOFLUSH);
   generate_event(ups, CMDCOMMFAILURE);

   write_unlock(ups);

   gettimeofday(&prev, NULL);
   do {
      /* Log an event every 10 minutes */
      gettimeofday(&now, NULL);
      if (TV_DIFF_MS(prev, now) >= 10*60*1000) {
         log_event(ups, event_msg[CMDCOMMFAILURE].level,
            event_msg[CMDCOMMFAILURE].msg);
         prev = now;
      }

      /*
       * This sleep should not be necessary since the smart_poll() 
       * routine normally waits TIMER_FAST (1) seconds. However,
       * in case the serial port is broken and generating spurious
       * characters, we sleep to reduce CPU consumption. 
       */
      sleep(1);
      tcflush(ups->fd, TCIOFLUSH);
   }
   while (strcmp(smart_poll('Y', ups), "SM") != 0);

   tcflush(ups->fd, TCIOFLUSH);
   generate_event(ups, CMDCOMMOK);

   write_lock(ups);
   ups->clear_commlost();
   linkcheck = FALSE;
}

/********************************************************************* 
 *
 *  This subroutine is called to load our shared memory with
 *  information that is changing inside the UPS depending
 *  on the state of the UPS and the mains power.
 */
int apcsmart_ups_read_volatile_data(UPSINFO *ups)
{
   time_t now;

   /*
    * We need it for self-test start time.
    */
   now = time(NULL);

   write_lock(ups);

   UPSlinkCheck(ups);              /* make sure serial port is working */

   ups->poll_time = time(NULL);    /* save time stamp */

   /* UPS_STATUS */
   if (ups->UPS_Cap[CI_STATUS]) {
      char status[10];
      int retries = 5;             /* Number of retries on status read */

    again:
      strncpy(status, smart_poll(ups->UPS_Cmd[CI_STATUS], ups), sizeof(status));

      Dmsg1(80, "Got status:%s\n", status);
      /*
       * The Status command may return "SM" probably because firmware
       * is in a state where it still didn't updated its internal status
       * register. In this case retry to read the register. To be sure
       * not to get stuck here, we retry only 5 times.
       *
       * XXX
       *
       * If this fails, apcupsd may not be able to detect a status
       * change and will have unpredictable behavior. This will be fixed
       * once we will handle correctly the own apcupsd Status word.
       */
      if (status[0] == 'S' && status[1] == 'M' && (retries-- > 0))
         goto again;

      ups->Status &= ~0xFF;        /* clear APC byte */
      ups->Status |= strtoul(status, NULL, 16) & 0xFF;  /* set APC byte */
   }

   /* ONBATT_STATUS_FLAG -- line quality */
   if (ups->UPS_Cap[CI_LQUAL])
      strncpy(ups->linequal, smart_poll(ups->UPS_Cmd[CI_LQUAL], ups),
         sizeof(ups->linequal));

   /* Reason for last transfer to batteries */
   if (ups->UPS_Cap[CI_WHY_BATT]) {
      ups->lastxfer = decode_lastxfer(
         smart_poll(ups->UPS_Cmd[CI_WHY_BATT], ups));
      /*
       * XXX
       *
       * See if this is a self test rather than power failure
       * But not now !
       * When we will be ready we will copy the code below inside
       * the driver entry point, for performing this check inside the
       * driver.
       */
   }

   /* Results of last self test */
   if (ups->UPS_Cap[CI_ST_STAT]) {
      ups->testresult = decode_testresult(
         smart_poll(ups->UPS_Cmd[CI_ST_STAT], ups));
   }

   /* LINE_VOLTAGE */
   if (ups->UPS_Cap[CI_VLINE])
      ups->LineVoltage = atof(smart_poll(ups->UPS_Cmd[CI_VLINE], ups));

   /* UPS_LINE_MAX */
   if (ups->UPS_Cap[CI_VMAX])
      ups->LineMax = atof(smart_poll(ups->UPS_Cmd[CI_VMAX], ups));

   /* UPS_LINE_MIN */
   if (ups->UPS_Cap[CI_VMIN])
      ups->LineMin = atof(smart_poll(ups->UPS_Cmd[CI_VMIN], ups));

   /* OUTPUT_VOLTAGE */
   if (ups->UPS_Cap[CI_VOUT])
      ups->OutputVoltage = atof(smart_poll(ups->UPS_Cmd[CI_VOUT], ups));

   /* BATT_FULL Battery level percentage */
   if (ups->UPS_Cap[CI_BATTLEV])
      ups->BattChg = atof(smart_poll(ups->UPS_Cmd[CI_BATTLEV], ups));

   /* BATT_VOLTAGE */
   if (ups->UPS_Cap[CI_VBATT])
      ups->BattVoltage = atof(smart_poll(ups->UPS_Cmd[CI_VBATT], ups));

   /* UPS_LOAD */
   if (ups->UPS_Cap[CI_LOAD])
      ups->UPSLoad = atof(smart_poll(ups->UPS_Cmd[CI_LOAD], ups));

   /* LINE_FREQ */
   if (ups->UPS_Cap[CI_FREQ])
      ups->LineFreq = atof(smart_poll(ups->UPS_Cmd[CI_FREQ], ups));

   /* UPS_RUNTIME_LEFT */
   if (ups->UPS_Cap[CI_RUNTIM])
      ups->TimeLeft = atof(smart_poll(ups->UPS_Cmd[CI_RUNTIM], ups));

   /* UPS_TEMP */
   if (ups->UPS_Cap[CI_ITEMP])
      ups->UPSTemp = atof(smart_poll(ups->UPS_Cmd[CI_ITEMP], ups));

   /* DIP_SWITCH_SETTINGS */
   if (ups->UPS_Cap[CI_DIPSW])
      ups->dipsw = strtoul(smart_poll(ups->UPS_Cmd[CI_DIPSW], ups), NULL, 16);

   /* Register 1 */
   if (ups->UPS_Cap[CI_REG1])
      ups->reg1 = strtoul(smart_poll(ups->UPS_Cmd[CI_REG1], ups), NULL, 16);

   /* Register 2 */
   if (ups->UPS_Cap[CI_REG2]) {
      ups->reg2 = strtoul(smart_poll(ups->UPS_Cmd[CI_REG2], ups), NULL, 16);
      ups->set_battpresent(!(ups->reg2 & 0x20));
   }

   /* Register 3 */
   if (ups->UPS_Cap[CI_REG3])
      ups->reg3 = strtoul(smart_poll(ups->UPS_Cmd[CI_REG3], ups), NULL, 16);

   /* Humidity percentage */
   if (ups->UPS_Cap[CI_HUMID])
      ups->humidity = atof(smart_poll(ups->UPS_Cmd[CI_HUMID], ups));

   /* Ambient temperature */
   if (ups->UPS_Cap[CI_ATEMP])
      ups->ambtemp = atof(smart_poll(ups->UPS_Cmd[CI_ATEMP], ups));

   /* Hours since self test */
   if (ups->UPS_Cap[CI_ST_TIME])
      ups->LastSTTime = atof(smart_poll(ups->UPS_Cmd[CI_ST_TIME], ups));

   write_unlock(ups);

   apc_enable(ups);                /* reenable APC serial UPS */

   return SUCCESS;
}

/********************************************************************* 
 *
 *  This subroutine is called to load our shared memory with
 *  information that is static inside the UPS.        Hence it
 *  normally would only be called once when starting up the
 *  UPS.
 */
int apcsmart_ups_read_static_data(UPSINFO *ups)
{
   /* Everything from here on down is non-volitile, that is
    * we do not expect it to change while the UPS is running
    * unless we explicitly change it.
    */

   /* SENSITIVITY */
   if (ups->UPS_Cap[CI_SENS])
      strncpy(ups->sensitivity, smart_poll(ups->UPS_Cmd[CI_SENS], ups),
         sizeof(ups->sensitivity));

   /* WAKEUP_DELAY */
   if (ups->UPS_Cap[CI_DWAKE])
      ups->dwake = (int)atof(smart_poll(ups->UPS_Cmd[CI_DWAKE], ups));

   /* SLEEP_DELAY */
   if (ups->UPS_Cap[CI_DSHUTD])
      ups->dshutd = (int)atof(smart_poll(ups->UPS_Cmd[CI_DSHUTD], ups));

   /* LOW_TRANSFER_LEVEL */
   if (ups->UPS_Cap[CI_LTRANS])
      ups->lotrans = (int)atof(smart_poll(ups->UPS_Cmd[CI_LTRANS], ups));

   /* HIGH_TRANSFER_LEVEL */
   if (ups->UPS_Cap[CI_HTRANS])
      ups->hitrans = (int)atof(smart_poll(ups->UPS_Cmd[CI_HTRANS], ups));

   /* UPS_BATT_CAP_RETURN */
   if (ups->UPS_Cap[CI_RETPCT])
      ups->rtnpct = (int)atof(smart_poll(ups->UPS_Cmd[CI_RETPCT], ups));

   /* ALARM_STATUS */
   if (ups->UPS_Cap[CI_DALARM])
      strncpy(ups->beepstate, smart_poll(ups->UPS_Cmd[CI_DALARM], ups),
         sizeof(ups->beepstate));

   /* LOWBATT_SHUTDOWN_LEVEL */
   if (ups->UPS_Cap[CI_DLBATT])
      ups->dlowbatt = (int)atof(smart_poll(ups->UPS_Cmd[CI_DLBATT], ups));

   /* UPS_NAME */
   if (ups->upsname[0] == 0 && ups->UPS_Cap[CI_IDEN])
      strncpy(ups->upsname, smart_poll(ups->UPS_Cmd[CI_IDEN], ups),
         sizeof(ups->upsname));

   /* UPS_SELFTEST */
   if (ups->UPS_Cap[CI_STESTI])
      strncpy(ups->selftest, smart_poll(ups->UPS_Cmd[CI_STESTI], ups),
         sizeof(ups->selftest));

   /* UPS_MANUFACTURE_DATE */
   if (ups->UPS_Cap[CI_MANDAT])
      strncpy(ups->birth, smart_poll(ups->UPS_Cmd[CI_MANDAT], ups),
         sizeof(ups->birth));

   /* UPS_SERIAL_NUMBER */
   if (ups->UPS_Cap[CI_SERNO])
      strncpy(ups->serial, smart_poll(ups->UPS_Cmd[CI_SERNO], ups),
         sizeof(ups->serial));

   /* UPS_BATTERY_REPLACE */
   if (ups->UPS_Cap[CI_BATTDAT])
      strncpy(ups->battdat, smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups),
         sizeof(ups->battdat));

   /* Nominal output voltage when on batteries */
   if (ups->UPS_Cap[CI_NOMOUTV])
      ups->NomOutputVoltage = (int)atof(smart_poll(ups->UPS_Cmd[CI_NOMOUTV], ups));

   /* Nominal battery voltage */
   if (ups->UPS_Cap[CI_NOMBATTV])
      ups->nombattv = atof(smart_poll(ups->UPS_Cmd[CI_NOMBATTV], ups));

   /*        Firmware revision */
   if (ups->UPS_Cap[CI_REVNO])
      strncpy(ups->firmrev, smart_poll(ups->UPS_Cmd[CI_REVNO], ups),
         sizeof(ups->firmrev));

   /*        Number of external batteries installed */
   if (ups->UPS_Cap[CI_EXTBATTS])
      ups->extbatts = (int)atof(smart_poll(ups->UPS_Cmd[CI_EXTBATTS], ups));

   /*        Number of bad batteries installed */
   if (ups->UPS_Cap[CI_BADBATTS])
      ups->badbatts = (int)atof(smart_poll(ups->UPS_Cmd[CI_BADBATTS], ups));

   /*        Old firmware revision */
   if (ups->UPS_Cap[CI_UPSMODEL])
      strncpy(ups->upsmodel, smart_poll(ups->UPS_Cmd[CI_UPSMODEL], ups),
         sizeof(ups->upsmodel));


   /*        EPROM Capabilities */
   if (ups->UPS_Cap[CI_EPROM]) {
      strncpy(ups->eprom, smart_poll(ups->UPS_Cmd[CI_EPROM], ups),
         sizeof(ups->eprom));
   }

   get_apc_model(ups);

   return SUCCESS;
}

int apcsmart_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   int retries = 5;                /* Number of retries if reason is NA (see below) */
   char ans[20];

   if (ups->is_slave()) {
      return SUCCESS;
   }

   switch (command) {
   case DEVICE_CMD_SET_DUMB_MODE:
      /* Set dumb mode for a smart UPS */
      write(ups->fd, "R", 1);      /* enter dumb mode */
      *ans = 0;
      getline(ans, sizeof(ans), ups);
      printf("Going dumb: %s\n", ans);
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      /* Results of last self test */
      if (ups->UPS_Cap[CI_ST_STAT]) {
         ups->testresult = decode_testresult(
            smart_poll(ups->UPS_Cmd[CI_ST_STAT], ups));
      }
      break;

   case DEVICE_CMD_CHECK_SELFTEST:

      Dmsg0(80, "Checking self test.\n");
      /*
       * XXX
       *
       * One day we will do this test inside the driver and not as an
       * entry point.
       */
      /* Reason for last transfer to batteries */
      if (ups->UPS_Cap[CI_WHY_BATT]) {
         ups->lastxfer = XFER_NA;
         while (ups->lastxfer == XFER_NA && retries--) {
            ups->lastxfer = decode_lastxfer(
               smart_poll(ups->UPS_Cmd[CI_WHY_BATT], ups));
            if (ups->lastxfer == XFER_NA) {
               Dmsg0(80, "Transfer reason still not available.\n");
               if (retries > 0)
                  sleep(2);           /* debounce */

               /*
                * Be careful because if we go out of here without
                * knowing the reason of transfer (i.e. the reason
                * is "NA", apcupsd will think this is a power failure
                * even if it is a self test. Not much of a problem
                * as this should not happen.
                * We allow 5 retries for reading reason from UPS before
                * giving up.
                */
            } else if (ups->lastxfer == XFER_SELFTEST) {
               ups->SelfTest = time(NULL);
               Dmsg1(80, "Self Test time: %s", ctime(&ups->SelfTest));
            } 
         }
      }
      break;

   default:
      return FAILURE;
      break;
   }

   return SUCCESS;
}
