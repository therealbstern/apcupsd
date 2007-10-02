
/*
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
   Copyright (C) 2005-2007 Adam D. Kropelin <akropel1@rochester.rr.com>
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

/* How long to wait before declaring commlost */
#define COMMLOST_TIMEOUT_MS (20*1000)

/* Convert UPS response to enum and string */
SelfTestResult ApcSmartDriver::decode_testresult(char* str)
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
LastXferCause ApcSmartDriver::decode_lastxfer(char *str)
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

int ApcSmartDriver::apc_enable()
{
   /* Enable APC Smart UPS */
   smart_poll('Y');
   smart_poll('Y');
   return 1;
}

/********************************************************************* 
 *
 * Send a charcter to the UPS and get
 * its response. Returns a pointer to the response string.
 *
 */
char *ApcSmartDriver::smart_poll(char cmd)
{
   static char answer[2000];
   int stat, retry;

   *answer = 0;
   if (_ups->mode.type <= SHAREBASIC)
      return answer;

   /* Don't retry Y/SM command */
   retry = (cmd == 'Y') ? 0 : 2;

   do {
      write(_ups->fd, &cmd, 1);
      stat = getline(answer, sizeof answer);

      /* If nothing returned, the link is probably down */
      if (*answer == 0 && stat == FAILURE) {
         UPSlinkCheck();           /* wait for link to come up */
         *answer = 0; /* UPSlinkCheck invokes us recursively, so clean up */
      }
   } while (*answer == 0 && stat == FAILURE && retry--);

   return answer;
}

/*
 * If s == NULL we are just waiting on FD for status changes.
 * If s != NULL we are asking the UPS to tell us the value of something.
 *
 * If s == NULL there is a much more fine-grained locking.
 */
int ApcSmartDriver::getline(char *s, int len)
{
   int i = 0;
   int ending = 0;
   char c;
   int retval;
   int wait;

   if (s != NULL)
      wait = TIMER_FAST;   /* 1 sec, expect fast response */
   else
      wait = _ups->wait_time;

#ifdef HAVE_MINGW
   /* Set read() timeout since we have no select() support. */
   {
      COMMTIMEOUTS ct;
      HANDLE h = (HANDLE)_get_osfhandle(_ups->fd);
      ct.ReadIntervalTimeout = MAXDWORD;
      ct.ReadTotalTimeoutMultiplier = MAXDWORD;
      ct.ReadTotalTimeoutConstant = wait * 1000;
      ct.WriteTotalTimeoutMultiplier = 0;
      ct.WriteTotalTimeoutConstant = 0;
      SetCommTimeouts(h, &ct);
   }
#endif

   while (!ending) {
#if !defined(HAVE_MINGW)
      fd_set rfds;
      struct timeval tv;

      FD_ZERO(&rfds);
      FD_SET(_ups->fd, &rfds);
      tv.tv_sec = wait;
      tv.tv_usec = 0;

      errno = 0;
      retval = select((_ups->fd) + 1, &rfds, NULL, NULL, &tv);

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
         retval = read(_ups->fd, &c, 1);
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
            write_lock(_ups);
         _ups->clear_online();
         Dmsg0(80, "Got UPS ON BATT.\n");
         if (s == NULL) {
            write_unlock(_ups);
            ending = 1;
         }
         break;
      case UPS_REPLACE_BATTERY:   /* UPS_REPLACE_BATTERY = '#'   */
         if (s == NULL)
            write_lock(_ups);
         _ups->set_replacebatt();
         Dmsg0(80, "Got UPS REPLACE_BATT.\n");
         if (s == NULL) {
            write_unlock(_ups);
            ending = 1;
         }
         break;
      case UPS_ON_LINE:           /* UPS_ON_LINE = '$'   */
         if (s == NULL)
            write_lock(_ups);
         _ups->set_online();
         Dmsg0(80, "Got UPS ON LINE.\n");
         if (s == NULL) {
            write_unlock(_ups);
            ending = 1;
         }
         break;
      case BATT_LOW:              /* BATT_LOW    = '%'   */
         if (s == NULL)
            write_lock(_ups);
         _ups->set_battlow();
         Dmsg0(80, "Got UPS BATT_LOW.\n");
         if (s == NULL) {
            write_unlock(_ups);
            ending = 1;
         }
         break;
      case BATT_OK:               /* BATT_OK     = '+'   */
         if (s == NULL)
            write_lock(_ups);
         _ups->clear_battlow();
         Dmsg0(80, "Got UPS BATT_OK.\n");
         if (s == NULL) {
            write_unlock(_ups);
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
void ApcSmartDriver::UPSlinkCheck()
{
   struct timeval now, prev, start;
   static int linkcheck = FALSE;

   if (linkcheck)
      return;

   linkcheck = TRUE;               /* prevent recursion */

   tcflush(_ups->fd, TCIOFLUSH);
   if (strcmp(smart_poll('Y'), "SM") == 0) {
      linkcheck = FALSE;
      _ups->clear_commlost();
      return;
   }

   write_unlock(_ups);

   gettimeofday(&start, NULL);
   prev = start;

   tcflush(_ups->fd, TCIOFLUSH);
   while (strcmp(smart_poll('Y'), "SM") != 0) {
      /* Declare commlost only if COMMLOST_TIMEOUT_MS has expired */
      gettimeofday(&now, NULL);
      if (TV_DIFF_MS(start, now) >= COMMLOST_TIMEOUT_MS) {
         /* Generate commlost event if we've not done so yet */
         if (!_ups->is_commlost()) {
            _ups->set_commlost();
            generate_event(_ups, CMDCOMMFAILURE);
            prev = now;
         }

         /* Log an event every 10 minutes */
         if (TV_DIFF_MS(prev, now) >= 10*60*1000) {
            log_event(_ups, event_msg[CMDCOMMFAILURE].level,
               event_msg[CMDCOMMFAILURE].msg);
            prev = now;
         }
      }

      /*
       * This sleep should not be necessary since the smart_poll() 
       * routine normally waits TIMER_FAST (1) seconds. However,
       * in case the serial port is broken and generating spurious
       * characters, we sleep to reduce CPU consumption. 
       */
      sleep(1);
      tcflush(_ups->fd, TCIOFLUSH);
   }

   write_lock(_ups);

   if (_ups->is_commlost()) {
      _ups->clear_commlost();
      generate_event(_ups, CMDCOMMOK);
   }

   linkcheck = FALSE;
}

/********************************************************************* 
 *
 *  This subroutine is called to load our shared memory with
 *  information that is changing inside the UPS depending
 *  on the state of the UPS and the mains power.
 */
bool ApcSmartDriver::ReadVolatileData()
{
   time_t now;
   char *answer;

   /*
    * We need it for self-test start time.
    */
   now = time(NULL);

   write_lock(_ups);

   UPSlinkCheck();              /* make sure serial port is working */

   _ups->poll_time = time(NULL);    /* save time stamp */

   /* UPS_STATUS */
   if (_ups->UPS_Cap[CI_STATUS]) {
      char status[10];
      int retries = 5;             /* Number of retries on status read */

    again:
      answer = smart_poll(_ups->UPS_Cmd[CI_STATUS]);
      Dmsg1(80, "Got CI_STATUS: %s\n", answer);
      strncpy(status, answer, sizeof(status));

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

      _ups->Status &= ~0xFF;        /* clear APC byte */
      _ups->Status |= strtoul(status, NULL, 16) & 0xFF;  /* set APC byte */
   }

   /* ONBATT_STATUS_FLAG -- line quality */
   if (_ups->UPS_Cap[CI_LQUAL]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_LQUAL]);
      Dmsg1(80, "Got CI_LQUAL: %s\n", answer);
      strncpy(_ups->linequal, answer, sizeof(_ups->linequal));
   }

   /* Reason for last transfer to batteries */
   if (_ups->UPS_Cap[CI_WHY_BATT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_WHY_BATT]);
      Dmsg1(80, "Got CI_WHY_BATT: %s\n", answer);
      _ups->lastxfer = decode_lastxfer(answer);
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
   if (_ups->UPS_Cap[CI_ST_STAT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_ST_STAT]);
      Dmsg1(80, "Got CI_ST_STAT: %s\n", answer);
      _ups->testresult = decode_testresult(answer);
   }

   /* LINE_VOLTAGE */
   if (_ups->UPS_Cap[CI_VLINE]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_VLINE]);
      Dmsg1(80, "Got CI_VLINE: %s\n", answer);
      _ups->LineVoltage = atof(answer);
   }

   /* UPS_LINE_MAX */
   if (_ups->UPS_Cap[CI_VMAX]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_VMAX]);
      Dmsg1(80, "Got CI_VMAX: %s\n", answer);
      _ups->LineMax = atof(answer);
   }

   /* UPS_LINE_MIN */
   if (_ups->UPS_Cap[CI_VMIN]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_VMIN]);
      Dmsg1(80, "Got CI_VMIN: %s\n", answer);
      _ups->LineMin = atof(answer);
   }

   /* OUTPUT_VOLTAGE */
   if (_ups->UPS_Cap[CI_VOUT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_VOUT]);
      Dmsg1(80, "Got CI_VOUT: %s\n", answer);
      _ups->OutputVoltage = atof(answer);
   }

   /* BATT_FULL Battery level percentage */
   if (_ups->UPS_Cap[CI_BATTLEV]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_BATTLEV]);
      Dmsg1(80, "Got CI_BATTLEV: %s\n", answer);
      _ups->BattChg = atof(answer);
   }

   /* BATT_VOLTAGE */
   if (_ups->UPS_Cap[CI_VBATT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_VBATT]);
      Dmsg1(80, "Got CI_VBATT: %s\n", answer);
      _ups->BattVoltage = atof(answer);
   }

   /* UPS_LOAD */
   if (_ups->UPS_Cap[CI_LOAD]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_LOAD]);
      Dmsg1(80, "Got CI_LOAD: %s\n", answer);
      _ups->UPSLoad = atof(answer);
   }

   /* LINE_FREQ */
   if (_ups->UPS_Cap[CI_FREQ]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_FREQ]);
      Dmsg1(80, "Got CI_FREQ: %s\n", answer);
      _ups->LineFreq = atof(answer);
   }

   /* UPS_RUNTIME_LEFT */
   if (_ups->UPS_Cap[CI_RUNTIM]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_RUNTIM]);
      Dmsg1(80, "Got CI_RUNTIM: %s\n", answer);
      _ups->TimeLeft = atof(answer);
   }

   /* UPS_TEMP */
   if (_ups->UPS_Cap[CI_ITEMP]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_ITEMP]);
      Dmsg1(80, "Got CI_ITEMP: %s\n", answer);
      _ups->UPSTemp = atof(answer);
   }

   /* DIP_SWITCH_SETTINGS */
   if (_ups->UPS_Cap[CI_DIPSW]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_DIPSW]);
      Dmsg1(80, "Got CI_DIPSW: %s\n", answer);
      _ups->dipsw = strtoul(answer, NULL, 16);
   }

   /* Register 1 */
   if (_ups->UPS_Cap[CI_REG1]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_REG1]);
      Dmsg1(80, "Got CI_REG1: %s\n", answer);
      _ups->reg1 = strtoul(answer, NULL, 16);
   }

   /* Register 2 */
   if (_ups->UPS_Cap[CI_REG2]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_REG2]);
      Dmsg1(80, "Got CI_REG2: %s\n", answer);
      _ups->reg2 = strtoul(answer, NULL, 16);
      _ups->set_battpresent(!(_ups->reg2 & 0x20));
   }

   /* Register 3 */
   if (_ups->UPS_Cap[CI_REG3]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_REG3]);
      Dmsg1(80, "Got CI_REG3: %s\n", answer);
      _ups->reg3 = strtoul(answer, NULL, 16);
   }

   /* Humidity percentage */
   if (_ups->UPS_Cap[CI_HUMID]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_HUMID]);
      Dmsg1(80, "Got CI_HUMID: %s\n", answer);
      _ups->humidity = atof(answer);
   }

   /* Ambient temperature */
   if (_ups->UPS_Cap[CI_ATEMP]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_ATEMP]);
      Dmsg1(80, "Got CI_ATEMP: %s\n", answer);
      _ups->ambtemp = atof(answer);
   }

   /* Hours since self test */
   if (_ups->UPS_Cap[CI_ST_TIME]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_ST_TIME]);
      Dmsg1(80, "Got CI_ST_TIME: %s\n", answer);
      _ups->LastSTTime = atof(answer);
   }

   write_unlock(_ups);

   apc_enable();                /* reenable APC serial UPS */

   return SUCCESS;
}

/********************************************************************* 
 *
 *  This subroutine is called to load our shared memory with
 *  information that is static inside the UPS.        Hence it
 *  normally would only be called once when starting up the
 *  UPS.
 */
bool ApcSmartDriver::ReadStaticData()
{
   char *answer;

   /* Everything from here on down is non-volitile, that is
    * we do not expect it to change while the UPS is running
    * unless we explicitly change it.
    */

   /* SENSITIVITY */
   if (_ups->UPS_Cap[CI_SENS]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_SENS]);
      Dmsg1(80, "Got CI_SENS: %s\n", answer);
      strncpy(_ups->sensitivity, answer, sizeof(_ups->sensitivity));
   }

   /* WAKEUP_DELAY */
   if (_ups->UPS_Cap[CI_DWAKE]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_DWAKE]);
      Dmsg1(80, "Got CI_DWAKE: %s\n", answer);
      _ups->dwake = (int)atof(answer);
   }

   /* SLEEP_DELAY */
   if (_ups->UPS_Cap[CI_DSHUTD]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_DSHUTD]);
      Dmsg1(80, "Got CI_DSHUTD: %s\n", answer);
      _ups->dshutd = (int)atof(answer);
   }

   /* LOW_TRANSFER_LEVEL */
   if (_ups->UPS_Cap[CI_LTRANS]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_LTRANS]);
      Dmsg1(80, "Got CI_LTRANS: %s\n", answer);
      _ups->lotrans = (int)atof(answer);
   }

   /* HIGH_TRANSFER_LEVEL */
   if (_ups->UPS_Cap[CI_HTRANS]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_HTRANS]);
      Dmsg1(80, "Got CI_HTRANS: %s\n", answer);
      _ups->hitrans = (int)atof(answer);
   }

   /* UPS_BATT_CAP_RETURN */
   if (_ups->UPS_Cap[CI_RETPCT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_RETPCT]);
      Dmsg1(80, "Got CI_RETPCT: %s\n", answer);
      _ups->rtnpct = (int)atof(answer);
   }

   /* ALARM_STATUS */
   if (_ups->UPS_Cap[CI_DALARM]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_DALARM]);
      Dmsg1(80, "Got CI_DALARM: %s\n", answer);
      strncpy(_ups->beepstate, answer, sizeof(_ups->beepstate));
   }

   /* LOWBATT_SHUTDOWN_LEVEL */
   if (_ups->UPS_Cap[CI_DLBATT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_DLBATT]);
      Dmsg1(80, "Got CI_DLBATT: %s\n", answer);
      _ups->dlowbatt = (int)atof(answer);
   }

   /* UPS_NAME */
   if (_ups->upsname[0] == 0 && _ups->UPS_Cap[CI_IDEN]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_IDEN]);
      Dmsg1(80, "Got CI_IDEN: %s\n", answer);
      strncpy(_ups->upsname, answer, sizeof(_ups->upsname));
   }

   /* UPS_SELFTEST */
   if (_ups->UPS_Cap[CI_STESTI]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_STESTI]);
      Dmsg1(80, "Got CI_STESTI: %s\n", answer);
      strncpy(_ups->selftest, answer, sizeof(_ups->selftest));
   }

   /* UPS_MANUFACTURE_DATE */
   if (_ups->UPS_Cap[CI_MANDAT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_MANDAT]);
      Dmsg1(80, "Got CI_MANDAT: %s\n", answer);
      strncpy(_ups->birth, answer, sizeof(_ups->birth));
   }

   /* UPS_SERIAL_NUMBER */
   if (_ups->UPS_Cap[CI_SERNO]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_SERNO]);
      Dmsg1(80, "Got CI_SERNO: %s\n", answer);
      strncpy(_ups->serial, answer, sizeof(_ups->serial));
   }

   /* UPS_BATTERY_REPLACE */
   if (_ups->UPS_Cap[CI_BATTDAT]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_BATTDAT]);
      Dmsg1(80, "Got CI_BATTDAT: %s\n", answer);
      strncpy(_ups->battdat, answer, sizeof(_ups->battdat));
   }

   /* Nominal output voltage when on batteries */
   if (_ups->UPS_Cap[CI_NOMOUTV]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_NOMOUTV]);
      Dmsg1(80, "Got CI_NOMOUTV: %s\n", answer);
      _ups->NomOutputVoltage = (int)atof(answer);
   }

   /* Nominal battery voltage */
   if (_ups->UPS_Cap[CI_NOMBATTV]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_NOMBATTV]);
      Dmsg1(80, "Got CI_NOMBATTV: %s\n", answer);
      _ups->nombattv = atof(answer);
   }

   /* Firmware revision */
   if (_ups->UPS_Cap[CI_REVNO]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_REVNO]);
      Dmsg1(80, "Got CI_REVNO: %s\n", answer);
      strncpy(_ups->firmrev, answer, sizeof(_ups->firmrev));
   }

   /* Number of external batteries installed */
   if (_ups->UPS_Cap[CI_EXTBATTS]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_EXTBATTS]);
      Dmsg1(80, "Got CI_EXTBATTS: %s\n", answer);
      _ups->extbatts = (int)atof(answer);
   }

   /* Number of bad batteries installed */
   if (_ups->UPS_Cap[CI_BADBATTS]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_BADBATTS]);
      Dmsg1(80, "Got CI_BADBATTS: %s\n", answer);
      _ups->badbatts = (int)atof(answer);
   }

   /* Old firmware revision */
   if (_ups->UPS_Cap[CI_UPSMODEL]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_UPSMODEL]);
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", answer);
      strncpy(_ups->upsmodel, answer, sizeof(_ups->upsmodel));
   }

   /* EPROM Capabilities */
   if (_ups->UPS_Cap[CI_EPROM]) {
      answer = smart_poll(_ups->UPS_Cmd[CI_EPROM]);
      Dmsg1(80, "Got CI_EPROM: %s\n", answer);
      strncpy(_ups->eprom, answer, sizeof(_ups->eprom));
   }

   get_apc_model();

   return SUCCESS;
}

bool ApcSmartDriver::EntryPoint(int command, void *data)
{
   int retries = 5;                /* Number of retries if reason is NA (see below) */
   char ans[20];

   switch (command) {
   case DEVICE_CMD_SET_DUMB_MODE:
      /* Set dumb mode for a smart UPS */
      write(_ups->fd, "R", 1);      /* enter dumb mode */
      *ans = 0;
      getline(ans, sizeof(ans));
      printf("Going dumb: %s\n", ans);
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      /* Results of last self test */
      if (_ups->UPS_Cap[CI_ST_STAT]) {
         _ups->testresult = decode_testresult(
            smart_poll(_ups->UPS_Cmd[CI_ST_STAT]));
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
      if (_ups->UPS_Cap[CI_WHY_BATT]) {
         _ups->lastxfer = XFER_NA;
         while (_ups->lastxfer == XFER_NA && retries--) {
            _ups->lastxfer = decode_lastxfer(
               smart_poll(_ups->UPS_Cmd[CI_WHY_BATT]));
            if (_ups->lastxfer == XFER_NA) {
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
            } else if (_ups->lastxfer == XFER_SELFTEST) {
               _ups->SelfTest = time(NULL);
               Dmsg1(80, "Self Test time: %s", ctime(&_ups->SelfTest));
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
