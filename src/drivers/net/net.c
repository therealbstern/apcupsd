/*
 * net.c
 *
 * Network client driver.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
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
#include "net.h"

/*
 * List of variables that can be read by getupsvar().
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS 
 * output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 * line, and if 1 returns only to first space (e.g. integers,
 * and floating point values.
 */
const NetUpsDriver::CmdTrans NetUpsDriver::cmdtrans[] =
{
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
   {"nominv",     "NOMINV",   1},
   {"nomoutv",    "NOMOUTV",  1},
   {"nompower",   "NOMPOWER", 1},
   {"outputfreq", "LINEFREQ", 1},
   {"outputv",    "OUTPUTV",  1},
   {"version",    "VERSION",  1},
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

/* Convert UPS response to enum */
SelfTestResult NetUpsDriver::decode_testresult(char* str)
{
   if (!strncmp(str, "OK", 2))
      return TEST_PASSED;
   else if (!strncmp(str, "NO", 2))
      return TEST_NONE;
   else if (!strncmp(str, "BT", 2))
      return TEST_FAILCAP;
   else if (!strncmp(str, "NG", 2))
      return TEST_FAILED;
   else if (!strncmp(str, "WN", 2))
      return TEST_WARNING;
   else if (!strncmp(str, "IP", 2))
      return TEST_INPROGRESS;
   else
      return TEST_UNKNOWN;
}

/* Convert UPS response to enum */
LastXferCause NetUpsDriver::decode_lastxfer(char *str)
{
   Dmsg(80, "Transfer reason: %s\n", str);

   if (!strcmp(str, "No transfers since turnon"))
      return XFER_NONE;
   else if (!strcmp(str, "Automatic or explicit self test"))
      return XFER_SELFTEST;
   else if (!strcmp(str, "Forced by software"))
      return XFER_FORCED;
   else if (!strcmp(str, "Low line voltage"))
      return XFER_UNDERVOLT;
   else if (!strcmp(str, "High line voltage"))
      return XFER_OVERVOLT;
   else if (!strcmp(str, "Line voltage notch or spike"))
      return XFER_NOTCHSPIKE;
   else if (!strcmp(str, "Unacceptable line voltage changes"))
      return XFER_RIPPLE;
   else if (!strcmp(str, "Input frequency out of range"))
      return XFER_FREQ;
   else
      return XFER_UNKNOWN;
}

NetUpsDriver::NetUpsDriver(UPSINFO *ups) :
   UpsDriver(ups),
   _hostname(NULL),
   _port(0),
   _sockfd(INVALID_SOCKET),
   _got_caps(false),
   _got_static_data(false),
   _last_fill_time(0),
   _statlen(0),
   _tlog(0),
   _comm_err(false),
   _comm_loss(0)
{
   memset(_device, 0, sizeof(_device));
   memset(_statbuf, 0, sizeof(_statbuf));
}

/*
 * Returns 1 if var found
 *   answer has var
 * Returns 0 if variable name not found
 *   answer has "Not found" is variable name not found
 *   answer may have "N/A" if the UPS does not support this
 *           feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
bool NetUpsDriver::getupsvar(const char *request, char *answer, int anslen)
{
   int i;
   const char *stat_match = NULL;
   char *find;
   int nfields = 0;
   char format[21];

   for (i = 0; cmdtrans[i].request; i++) {
      if (!(strcmp(cmdtrans[i].request, request))) {
         stat_match = cmdtrans[i].upskeyword;
         nfields = cmdtrans[i].nfields;
      }
   }

   if (stat_match) {
      if ((find = strstr(_statbuf, stat_match)) != NULL) {
         if (nfields == 1) {       /* get one field */
            asnprintf(format, sizeof(format), "%%*s %%*s %%%ds", anslen);
            sscanf(find, format, answer);
         } else {                  /* get everything to eol */
            i = 0;
            find += 11;            /* skip label */

            while (*find != '\n' && i < anslen - 1)
               answer[i++] = *find++;

            answer[i] = 0;
         }
         if (strcmp(answer, "N/A") == 0) {
            return 0;
         }
         Dmsg(100, "Return 1 for getupsvar %s %s\n", request, answer);
         return true;
      }
   } else {
      Dmsg(100, "Hey!!! No match in getupsvar for %s!\n", request);
   }

   strlcpy(answer, "Not found", anslen);
   return false;
}

bool NetUpsDriver::poll_ups()
{
   int n, stat = 1;
   char buf[1000];

   _statbuf[0] = 0;
   _statlen = 0;

   Dmsg(20, "Opening connection to %s:%d\n", _hostname, _port);
   if ((_sockfd = net_open(_hostname, NULL, _port)) < 0) {
      Dmsg(90, "Exit poll_ups 0 comm lost: %s\n", strerror(-_sockfd));
      if (!_ups->is_commlost()) {
         _ups->set_commlost();
      }
      return false;
   }

   if ((n = net_send(_sockfd, "status", 6)) != 6) {
      net_close(_sockfd);
      Dmsg(90, "Exit poll_ups 0 net_send fails: %s\n", strerror(-n));
      _ups->set_commlost();
      return false;
   }

   Dmsg(99, "===============\n");
   while ((n = net_recv(_sockfd, buf, sizeof(buf) - 1)) > 0) {
      buf[n] = 0;
      strlcat(_statbuf, buf, sizeof(_statbuf));
      Dmsg(99, "Partial buf (%d, %d):\n%s", n, strlen(_statbuf), buf);
   }
   Dmsg(99, "===============\n");

   if (n < 0) {
      stat = 0;
      Dmsg(90, "Exit poll_ups 0 bad stat net_recv: %s\n", strerror(-n));
      _ups->set_commlost();
   } else {
      _ups->clear_commlost();
   }
   net_close(_sockfd);

   Dmsg(99, "Buffer:\n%s\n", _statbuf);
   _statlen = strlen(_statbuf);
   Dmsg(90, "Exit poll_ups, stat=%d\n", stat);
   return stat;
}

/*
 * Fill buffer with data from UPS network daemon   
 * Returns false on error
 * Returns true if OK
 */
#define SLEEP_TIME 2
bool NetUpsDriver::fill_status_buffer()
{
   time_t now;

   /* Poll or fill the buffer maximum one time per second */
   now = time(NULL);
   if ((now - _last_fill_time) < 2) {
      Dmsg(90, "Exit fill_status_buffer OK less than 2 sec\n");
      return true;
   }

   if (!poll_ups()) {
      /* generate event once */
      if (!_comm_err) {
         execute_command(_ups, ups_event[CMDCOMMFAILURE]);
         _comm_err = true;
      }

      /* log every 10 minutes */
      if (now - _tlog >= 10 * 60) {
         _tlog = now;
         log_event(_ups, event_msg[CMDCOMMFAILURE].level,
            event_msg[CMDCOMMFAILURE].msg);
      }
   } else {
      if (_comm_err) {
         generate_event(_ups, CMDCOMMOK);
         _tlog = 0;
         _comm_err = false;
      }

      _last_fill_time = now;

      if (!_got_caps)
         get_capabilities();

      if (_got_caps && !_got_static_data)
         read_static_data();
   }

   return !_comm_err;
}

bool NetUpsDriver::get_ups_status_flag(int fill)
{
   char answer[200];
   int stat = 1;
   int32_t newStatus;              /* this really should be uint32_t! */
   int32_t masterStatus;           /* status from master */

   if (!_got_caps) {
      get_capabilities();
      if (!_got_caps)
         return false;
   }

   if (!_got_static_data) {
      read_static_data();
      if (!_got_static_data)
         return false;
   }

   if (fill) {
      /* 
       * Not locked because no one else should be writing in the 
       * status buffer, and we don't want to lock across I/O
       * operations. 
       */
      stat = fill_status_buffer();
   }

   write_lock(_ups);
   answer[0] = 0;
   if (!getupsvar("status", answer, sizeof(answer))) {
      Dmsg(100, "HEY!!! Couldn't get status flag.\n");
      stat = 0;
      masterStatus = 0;
   } else {
      /*
       * Make sure we don't override local bits, and that
       * all non-local bits are set/cleared correctly.
       *
       * local bits = UPS_commlost|UPS_shutdown|UPS_slave|UPS_slavedown|
       *              UPS_prev_onbatt|UPS_prev_battlow|UPS_onbatt_msg|
       *              UPS_fastpoll|UPS_plugged|UPS_dev_setup
       */

      /* First transfer set or not set all non-local bits */
      masterStatus = strtol(answer, NULL, 0);
      newStatus = masterStatus & ~UPS_LOCAL_BITS;  /* clear local bits */
      _ups->Status &= UPS_LOCAL_BITS;               /* clear non-local bits */
      _ups->Status |= newStatus;                    /* set new non-local bits */

      /*
       * Now set any special bits, note this is set only, we do
       * not clear these bits, but let our own core code clear them
       */
      newStatus = masterStatus & (UPS_commlost | UPS_fastpoll);
      _ups->Status |= newStatus;
   }

   Dmsg(100, "Got Status = %s 0x%x\n", answer, _ups->Status);

   if (masterStatus & UPS_shutdown && !_ups->is_shut_remote()) {
      _ups->set_shut_remote();    /* if master is shutting down so do we */
      log_event(_ups, LOG_ERR, "Shutdown because NIS master is shutting down.");
      Dmsg(100, "Set SHUT_REMOTE because of master status.\n");
   }

   /*
    * If we lost connection with master and we
    * are running on batteries, shutdown on the fourth
    * consequtive pass here. While on batteries, this code
    * is called once per second.
    */
   if (stat == 0 && _ups->is_onbatt()) {
      if (_comm_loss++ == 4 && !_ups->is_shut_remote()) {
         _ups->set_shut_remote();
         log_event(_ups, LOG_ERR,
            "Shutdown because loss of comm with NIS master while on batteries.");
         Dmsg(100, "Set SHUT_REMOTE because of loss of comm on batteries.\n");
      }
   } else {
      _comm_loss = 0;
   }

   write_unlock(_ups);
   return stat;
}


bool NetUpsDriver::Open()
{
   strlcpy(_device, _ups->device, sizeof(_device));
   strlcpy(_ups->master_name, _ups->device, sizeof(_ups->master_name));
   strlcpy(_ups->upsclass.long_name, "Net Slave", sizeof(_ups->upsclass.long_name));

   /* Now split the device. */
   _hostname = _device;

   char *cp = strchr(_device, ':');
   if (cp) {
      *cp = '\0';
      cp++;
      _port = atoi(cp);
   } else {
      /* use NIS port as default */
      _port = _ups->statusport;
   }

   _statbuf[0] = 0;
   _statlen = 0;

   /* Fake core code. Will go away when _ups->fd is cleaned up. */
   _ups->fd = 1;

   return true;
}

bool NetUpsDriver::Close()
{
   /* Fake core code. Will go away when _ups->fd will be cleaned up. */
   _ups->fd = -1;

   return true;
}

bool NetUpsDriver::get_capabilities()
{
   char answer[200];

   write_lock(_ups);

   if (poll_ups()) {
      _ups->UPS_Cap[CI_VLINE] = getupsvar("utility", answer, sizeof(answer));
      _ups->UPS_Cap[CI_LOAD] = getupsvar("loadpct", answer, sizeof(answer));
      _ups->UPS_Cap[CI_BATTLEV] = getupsvar("battcap", answer, sizeof(answer));
      _ups->UPS_Cap[CI_RUNTIM] = getupsvar("runtime", answer, sizeof(answer));
      _ups->UPS_Cap[CI_VMAX] = getupsvar("linemax", answer, sizeof(answer));
      _ups->UPS_Cap[CI_VMIN] = getupsvar("linemin", answer, sizeof(answer));
      _ups->UPS_Cap[CI_VOUT] = getupsvar("outputv", answer, sizeof(answer));
      _ups->UPS_Cap[CI_SENS] = getupsvar("sense", answer, sizeof(answer));
      _ups->UPS_Cap[CI_DLBATT] = getupsvar("lowbatt", answer, sizeof(answer));
      _ups->UPS_Cap[CI_LTRANS] = getupsvar("lowxfer", answer, sizeof(answer));
      _ups->UPS_Cap[CI_HTRANS] = getupsvar("highxfer", answer, sizeof(answer));
      _ups->UPS_Cap[CI_RETPCT] = getupsvar("retpct", answer, sizeof(answer));
      _ups->UPS_Cap[CI_ITEMP] = getupsvar("upstemp", answer, sizeof(answer));
      _ups->UPS_Cap[CI_VBATT] = getupsvar("battvolt", answer, sizeof(answer));
      _ups->UPS_Cap[CI_FREQ] = getupsvar("outputfreq", answer, sizeof(answer));
      _ups->UPS_Cap[CI_WHY_BATT] = getupsvar("lastxfer", answer, sizeof(answer));
      _ups->UPS_Cap[CI_ST_STAT] = getupsvar("selftest", answer, sizeof(answer));
      _ups->UPS_Cap[CI_SERNO] = getupsvar("serialno", answer, sizeof(answer));
      _ups->UPS_Cap[CI_BATTDAT] = getupsvar("battdate", answer, sizeof(answer));
      _ups->UPS_Cap[CI_NOMBATTV] = getupsvar("nombattv", answer, sizeof(answer));
      _ups->UPS_Cap[CI_NOMINV] = getupsvar("nominv", answer, sizeof(answer));
      _ups->UPS_Cap[CI_NOMOUTV] = getupsvar("nomoutv", answer, sizeof(answer));
      _ups->UPS_Cap[CI_NOMPOWER] = getupsvar("nompower", answer, sizeof(answer));
      _ups->UPS_Cap[CI_REVNO] = getupsvar("firmware", answer, sizeof(answer));
      _ups->UPS_Cap[CI_UPSMODEL] = getupsvar("model", answer, sizeof(answer));
      _got_caps = true;
   } else {
      _got_caps = false;
   }

   write_unlock(_ups);
   return true;
}

bool NetUpsDriver::check_state()
{
   int sleep_time;

   sleep_time = _ups->wait_time;

   Dmsg(100, "Sleep %d secs.\n", sleep_time);
   sleep(sleep_time);
   get_ups_status_flag(1);

   return true;
}

#define GETVAR(ci,str) \
   (_ups->UPS_Cap[ci] && getupsvar(str, answer, sizeof(answer)))

bool NetUpsDriver::read_volatile_data()
{
   char answer[200];

   if (!fill_status_buffer())
      return 0;

   write_lock(_ups);
   _ups->set_slave();

   /* ***FIXME**** poll time needs to be scanned */
   _ups->poll_time = time(NULL);
   _ups->last_master_connect_time = _ups->poll_time;

   if (GETVAR(CI_VLINE, "utility"))
      _ups->LineVoltage = atof(answer);

   if (GETVAR(CI_LOAD, "loadpct"))
      _ups->UPSLoad = atof(answer);

   if (GETVAR(CI_BATTLEV, "battcap"))
      _ups->BattChg = atof(answer);

   if (GETVAR(CI_RUNTIM, "runtime"))
      _ups->TimeLeft = atof(answer);

   if (GETVAR(CI_VMAX, "linemax"))
      _ups->LineMax = atof(answer);

   if (GETVAR(CI_VMIN, "linemin"))
      _ups->LineMin = atof(answer);

   if (GETVAR(CI_VOUT, "outputv"))
      _ups->OutputVoltage = atof(answer);

   if (GETVAR(CI_SENS, "sense"))
      _ups->sensitivity[0] = answer[0];

   if (GETVAR(CI_DLBATT, "lowbatt"))
      _ups->dlowbatt = (int)atof(answer);

   if (GETVAR(CI_LTRANS, "lowxfer"))
      _ups->lotrans = (int)atof(answer);

   if (GETVAR(CI_HTRANS, "highxfer"))
      _ups->hitrans = (int)atof(answer);

   if (GETVAR(CI_RETPCT, "retpct"))
      _ups->rtnpct = (int)atof(answer);

   if (GETVAR(CI_ITEMP, "upstemp"))
      _ups->UPSTemp = atof(answer);

   if (GETVAR(CI_VBATT, "battvolt"))
      _ups->BattVoltage = atof(answer);

   if (GETVAR(CI_FREQ, "outputfreq"))
      _ups->LineFreq = atof(answer);

   if (GETVAR(CI_WHY_BATT, "lastxfer"))
      _ups->lastxfer = decode_lastxfer(answer);

   if (GETVAR(CI_ST_STAT, "selftest"))
      _ups->testresult = decode_testresult(answer);

   write_unlock(_ups);

   get_ups_status_flag(0);

   return true;
}

bool NetUpsDriver::read_static_data()
{
   char answer[200];

   write_lock(_ups);

   if (poll_ups()) {
      if (!getupsvar(
            "upsname", _ups->upsname,
            sizeof(_ups->upsname))) {
         log_event(_ups, LOG_ERR, "getupsvar: failed for \"upsname\".");
      }
      if (!getupsvar(
            "model", _ups->upsmodel, 
            sizeof(_ups->upsmodel))) {
         log_event(_ups, LOG_ERR, "getupsvar: failed for \"model\".");
      }
      if (!getupsvar(
            "upsmode", _ups->upsclass.long_name,
            sizeof(_ups->upsclass.long_name))) {
         log_event(_ups, LOG_ERR, "getupsvar: failed for \"upsmode\".");
      }

      if (GETVAR(CI_SERNO, "serialno"))
         strlcpy(_ups->serial, answer, sizeof(_ups->serial));

      if (GETVAR(CI_BATTDAT, "battdate"))
         strlcpy(_ups->battdat, answer, sizeof(_ups->battdat));

      if (GETVAR(CI_NOMBATTV, "nombattv"))
         _ups->nombattv = atof(answer);

      if (GETVAR(CI_NOMINV, "nominv"))
         _ups->NomInputVoltage = (int)atof(answer);

      if (GETVAR(CI_NOMOUTV, "nomoutv"))
         _ups->NomOutputVoltage = (int)atof(answer);

      if (GETVAR(CI_NOMPOWER, "nompower"))
         _ups->NomPower = (int)atof(answer);

      if (GETVAR(CI_REVNO, "firmware"))
         strlcpy(_ups->firmrev, answer, sizeof(_ups->firmrev));

      _got_static_data = true;
   } else {
      _got_static_data = false;
   }

   write_unlock(_ups);
   return true;
}

bool NetUpsDriver::entry_point(int command, void *data)
{
   char answer[200];

   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg(80, "Checking self test.\n");
      /*
       * XXX FIXME
       *
       * One day we will do this test inside the driver and not as an
       * entry point.
       */
      /* Reason for last transfer to batteries */
      if (GETVAR(CI_WHY_BATT, "lastxfer")) {
         _ups->lastxfer = decode_lastxfer(answer);
         Dmsg(80, "Transfer reason: %d\n", _ups->lastxfer);

         /* See if this is a self test rather than power false */
         if (_ups->lastxfer == XFER_SELFTEST) {
            /*
             * set Self Test start time
             */
            _ups->SelfTest = time(NULL);
            Dmsg(80, "Self Test time: %s", ctime(&_ups->SelfTest));
         }
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      if (!GETVAR(CI_ST_STAT, "selftest"))
         return false;

      _ups->testresult = decode_testresult(answer);
      break;

   default:
      return false;
   }

   return true;
}
