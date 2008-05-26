/*
 * drv_powernet.c
 *
 * PowerNet driver
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@apcupsd.org>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "snmp.h"

bool SnmpDriver::powernet_check_comm_lost()
{
   struct timeval now;
   static struct timeval prev;
   struct snmp_session *s = &_session;
   powernet_mib_t *data = (powernet_mib_t *)_MIB;
   bool ret = true;

   /*
    * Check the Ethernet COMMLOST first, then check the
    * Web/SNMP->UPS serial COMMLOST.
    */
   data->upsComm = NULL;
   if (powernet_mib_mgr_get_upsComm(s, &(data->upsComm)) < 0 ||
       (data->upsComm && data->upsComm->__upsCommStatus == 2)) {

      if (!_ups->is_commlost()) {
         generate_event(_ups, CMDCOMMFAILURE);
         _ups->set_commlost();
         gettimeofday(&prev, NULL);
      }

      /* Log an event every 10 minutes */
      gettimeofday(&now, NULL);
      if (TV_DIFF_MS(prev, now) >= 10*60*1000) {
         log_event(_ups, event_msg[CMDCOMMFAILURE].level,
            event_msg[CMDCOMMFAILURE].msg);
         prev = now;
      }

      ret = false;
   }
   else if (_ups->is_commlost())
   {
      generate_event(_ups, CMDCOMMOK);
      _ups->clear_commlost();
   }

   if (data->upsComm)
      free(data->upsComm);

   return ret;
}


bool SnmpDriver::powernet_snmp_kill_ups_power()
{
   /* Was 1} change submitted by Kastus Shchuka (kastus@lists.sourceforge.net) 10Dec03 */
   oid upsBasicControlConserveBattery[] =
      { 1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 1, 1, 0 };
   struct snmp_session *s = &_session;
   struct snmp_session *peer;
   struct snmp_pdu *request, *response;
   int status;

   /*
    * Set up the SET request.
    */
   request = snmp_pdu_create(SNMP_MSG_SET);

   /*
    * Set upsBasicControlConserveBattery variable (INTEGER) to
    * turnOffUpsToConserveBattery(2) value. Will turn on the UPS only
    * when power returns.
    */
   if (snmp_add_var(request, upsBasicControlConserveBattery,
         sizeof(upsBasicControlConserveBattery) / sizeof(oid), 'i', "2")) {
      return false;
   }

   peer = snmp_open(s);

   if (!peer) {
      Dmsg0(0, "Can not open the SNMP connection.\n");
      return false;
   }

   status = snmp_synch_response(peer, request, &response);

   if (status != STAT_SUCCESS) {
      Dmsg0(0, "Unable to communicate with UPS.\n");
      return false;
   }

   if (response->errstat != SNMP_ERR_NOERROR) {
      Dmsg1(0, "Unable to kill UPS power: can not set SNMP variable (%d).\n", response->errstat);
      return false;
   }

   if (response)
      snmp_free_pdu(response);

   snmp_close(peer);

   return true;
}

bool SnmpDriver::powernet_snmp_ups_get_capabilities()
{
   /*
    * Assume that an UPS with Web/SNMP card has all the capabilities,
    * minus a few.
    */
   for (int i = 0; i <= CI_MAX_CAPS; i++)
   {
      if (i != CI_NOMBATTV &&
          i != CI_HUMID    &&
          i != CI_ATEMP    &&
          i != CI_VBATT    &&
          i != CI_NOMINV   &&
          i != CI_REG1     &&
          i != CI_REG2     &&
          i != CI_REG3)
         _ups->UPS_Cap[i] = TRUE;
   }

   if (powernet_check_comm_lost() == 0)
      return false;

   return true;
}


bool SnmpDriver::powernet_snmp_ups_read_static_data()
{
   struct snmp_session *s = &_session;
   powernet_mib_t *data = (powernet_mib_t *)_MIB;

   if (powernet_check_comm_lost() == 0)
      return false;

   data->upsBasicIdent = NULL;
   powernet_mib_mgr_get_upsBasicIdent(s, &(data->upsBasicIdent));
   if (data->upsBasicIdent) {
      SNMP_STRING(upsBasicIdent, Model, upsmodel);
      SNMP_STRING(upsBasicIdent, Name, upsname);
      free(data->upsBasicIdent);
   }

   data->upsAdvIdent = NULL;
   powernet_mib_mgr_get_upsAdvIdent(s, &(data->upsAdvIdent));
   if (data->upsAdvIdent) {
      SNMP_STRING(upsAdvIdent, FirmwareRevision, firmrev);
      SNMP_STRING(upsAdvIdent, DateOfManufacture, birth);
      SNMP_STRING(upsAdvIdent, SerialNumber, serial);
      free(data->upsAdvIdent);
   }

   data->upsBasicBattery = NULL;
   powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
   if (data->upsBasicBattery) {
      SNMP_STRING(upsBasicBattery, LastReplaceDate, battdat);
      free(data->upsBasicBattery);
   }

   data->upsAdvBattery = NULL;
   powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
   if (data->upsAdvBattery) {
      _ups->extbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBattPacks;
      _ups->badbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBadBattPacks;
      free(data->upsAdvBattery);
   }

   data->upsAdvConfig = NULL;
   powernet_mib_mgr_get_upsAdvConfig(s, &(data->upsAdvConfig));
   if (data->upsAdvConfig) {
      _ups->NomOutputVoltage = data->upsAdvConfig->__upsAdvConfigRatedOutputVoltage;
      _ups->hitrans = data->upsAdvConfig->__upsAdvConfigHighTransferVolt;
      _ups->lotrans = data->upsAdvConfig->__upsAdvConfigLowTransferVolt;
      switch (data->upsAdvConfig->__upsAdvConfigAlarm) {
      case 1:
         if (data->upsAdvConfig->__upsAdvConfigAlarmTimer / 100 < 30)
            astrncpy(_ups->beepstate, "0 Seconds", sizeof(_ups->beepstate));
         else
            astrncpy(_ups->beepstate, "Timed", sizeof(_ups->beepstate));
         break;
      case 2:
         astrncpy(_ups->beepstate, "LowBatt", sizeof(_ups->beepstate));
         break;
      case 3:
         astrncpy(_ups->beepstate, "NoAlarm", sizeof(_ups->beepstate));
         break;
      default:
         astrncpy(_ups->beepstate, "Timed", sizeof(_ups->beepstate));
         break;
      }

      _ups->rtnpct = data->upsAdvConfig->__upsAdvConfigMinReturnCapacity;

      switch (data->upsAdvConfig->__upsAdvConfigSensitivity) {
      case 1:
         astrncpy(_ups->sensitivity, "Auto", sizeof(_ups->sensitivity));
         break;
      case 2:
         astrncpy(_ups->sensitivity, "Low", sizeof(_ups->sensitivity));
         break;
      case 3:
         astrncpy(_ups->sensitivity, "Medium", sizeof(_ups->sensitivity));
         break;
      case 4:
         astrncpy(_ups->sensitivity, "High", sizeof(_ups->sensitivity));
         break;
      default:
         astrncpy(_ups->sensitivity, "Unknown", sizeof(_ups->sensitivity));
         break;
      }

      /* Data in Timeticks (1/100th sec). */
      _ups->dlowbatt = data->upsAdvConfig->__upsAdvConfigLowBatteryRunTime / 6000;
      _ups->dwake = data->upsAdvConfig->__upsAdvConfigReturnDelay / 100;
      _ups->dshutd = data->upsAdvConfig->__upsAdvConfigShutoffDelay / 100;
      free(data->upsAdvConfig);
   }

   data->upsAdvTest = NULL;
   powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
   if (data->upsAdvTest) {
      switch (data->upsAdvTest->__upsAdvTestDiagnosticSchedule) {
      case 1:
         astrncpy(_ups->selftest, "unknown", sizeof(_ups->selftest));
         break;
      case 2:
         astrncpy(_ups->selftest, "biweekly", sizeof(_ups->selftest));
         break;
      case 3:
         astrncpy(_ups->selftest, "weekly", sizeof(_ups->selftest));
         break;
      case 4:
         astrncpy(_ups->selftest, "atTurnOn", sizeof(_ups->selftest));
         break;
      case 5:
         astrncpy(_ups->selftest, "never", sizeof(_ups->selftest));
         break;
      default:
         astrncpy(_ups->selftest, "unknown", sizeof(_ups->selftest));
         break;
      }

      switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
      case 1:  /* Passed */
         _ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
      case 3:  /* Invalid test */
         _ups->testresult = TEST_FAILED;
         break;
      case 4:  /* Test in progress */
         _ups->testresult = TEST_INPROGRESS;
         break;
      default:
         _ups->testresult = TEST_UNKNOWN;
         break;
      }

      free(data->upsAdvTest);
   }

   return true;
}

bool SnmpDriver::powernet_snmp_ups_read_volatile_data()
{
   struct snmp_session *s = &_session;
   powernet_mib_t *data = (powernet_mib_t *)_MIB;

   if (powernet_check_comm_lost() == 0)
      return false;

   data->upsBasicBattery = NULL;
   powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
   if (data->upsBasicBattery) {
      switch (data->upsBasicBattery->__upsBasicBatteryStatus) {
      case 2:
         _ups->clear_battlow();
         break;
      case 3:
         _ups->set_battlow();
         break;
      default:                    /* Unknown, assume battery is ok */
         _ups->clear_battlow();
         break;
      }
      free(data->upsBasicBattery);
   }

   data->upsAdvBattery = NULL;
   powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
   if (data->upsAdvBattery) {
      _ups->BattChg = data->upsAdvBattery->__upsAdvBatteryCapacity;
      _ups->UPSTemp = data->upsAdvBattery->__upsAdvBatteryTemperature;
      _ups->TimeLeft = data->upsAdvBattery->__upsAdvBatteryRunTimeRemaining / 6000;

      if (data->upsAdvBattery->__upsAdvBatteryReplaceIndicator == 2)
         _ups->set_replacebatt();
      else
         _ups->clear_replacebatt();

      free(data->upsAdvBattery);
   }

   data->upsBasicInput = NULL;
   powernet_mib_mgr_get_upsBasicInput(s, &(data->upsBasicInput));
   if (data->upsBasicInput) {
      _ups->InputPhase = data->upsBasicInput->__upsBasicInputPhase;
      free(data->upsBasicInput);
   }

   data->upsAdvInput = NULL;
   powernet_mib_mgr_get_upsAdvInput(s, &(data->upsAdvInput));
   if (data->upsAdvInput) {
      _ups->LineVoltage = data->upsAdvInput->__upsAdvInputLineVoltage;
      _ups->LineMax = data->upsAdvInput->__upsAdvInputMaxLineVoltage;
      _ups->LineMin = data->upsAdvInput->__upsAdvInputMinLineVoltage;
      _ups->LineFreq = data->upsAdvInput->__upsAdvInputFrequency;
      switch (data->upsAdvInput->__upsAdvInputLineFailCause) {
      case 1:
         _ups->lastxfer = XFER_NONE;
         break;
      case 2:  /* High line voltage */
         _ups->lastxfer = XFER_OVERVOLT;
         break;
      case 3:  /* Brownout */
      case 4:  /* Blackout */
         _ups->lastxfer = XFER_UNDERVOLT;
         break;
      case 5:  /* Small sag */
      case 6:  /* Deep sag */
      case 7:  /* Small spike */
      case 8:  /* Deep spike */
         _ups->lastxfer = XFER_NOTCHSPIKE;
         break;
      case 9:
         _ups->lastxfer = XFER_SELFTEST;
         break;
      case 10:
         _ups->lastxfer = XFER_RIPPLE;
         break;
      default:
         _ups->lastxfer = XFER_UNKNOWN;
         break;
      }
      free(data->upsAdvInput);
   }

   data->upsBasicOutput = NULL;
   powernet_mib_mgr_get_upsBasicOutput(s, &(data->upsBasicOutput));
   if (data->upsBasicOutput) {
      /* Clear the following flags: only one status will be TRUE */
      Dmsg1(99, "Status before clearing: 0x%08x\n", _ups->Status);
      _ups->clear_online();
      _ups->clear_onbatt();
      _ups->clear_boost();
      _ups->clear_trim();
      Dmsg1(99, "Status after clearing: 0x%08x\n", _ups->Status);

      switch (data->upsBasicOutput->__upsBasicOutputStatus) {
      case 2:
         _ups->set_online();
         break;
      case 3:
         _ups->set_onbatt();
         break;
      case 4:
         _ups->set_boost();
         break;
      case 12:
         _ups->set_trim();
         break;
      case 1:                     /* unknown */
      case 5:                     /* timed sleeping */
      case 6:                     /* software bypass */
      case 7:                     /* UPS off */
      case 8:                     /* UPS rebooting */
      case 9:                     /* switched bypass */
      case 10:                    /* hardware failure bypass */
      case 11:                    /* sleeping until power returns */
      default:                    /* unknown */
         break;
      }
      _ups->OutputPhase = data->upsBasicOutput->__upsBasicOutputPhase;
      free(data->upsBasicOutput);
   }

   data->upsAdvOutput = NULL;
   powernet_mib_mgr_get_upsAdvOutput(s, &(data->upsAdvOutput));
   if (data->upsAdvOutput) {
      _ups->OutputVoltage = data->upsAdvOutput->__upsAdvOutputVoltage;
      _ups->OutputFreq = data->upsAdvOutput->__upsAdvOutputFrequency;
      _ups->UPSLoad = data->upsAdvOutput->__upsAdvOutputLoad;
      _ups->OutputCurrent = data->upsAdvOutput->__upsAdvOutputCurrent;
      free(data->upsAdvOutput);
   }

   data->upsAdvTest = NULL;
   powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
   if (data->upsAdvTest) {
      switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
      case 1:  /* Passed */
         _ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
      case 3:  /* Invalid test */
         _ups->testresult = TEST_FAILED;
         break;
      case 4:  /* Test in progress */
         _ups->testresult = TEST_INPROGRESS;
         break;
      default:
         _ups->testresult = TEST_UNKNOWN;
         break;
      }

      /* Not implemented. Needs transform date(mm/dd/yy)->hours. */
      // _ups->LastSTTime = data->upsAdvTest->upsAdvTestLastDiagnosticsDate;

      if (data->upsAdvTest->__upsAdvTestCalibrationResults == 3)
         _ups->set_calibration();
      else
         _ups->clear_calibration();

      free(data->upsAdvTest);
   }

   return true;
}

/* Callback invoked by SNMP library when an async event arrives */
int SnmpDriver::powernet_snmp_callback(
   int operation, snmp_session *session, 
   int reqid, snmp_pdu *pdu, void *magic)
{
   SnmpDriver *_this = (SnmpDriver*)magic;

   Dmsg1(100, "powernet_snmp_callback: %d\n", reqid);

   if (reqid == 0)
      _this->_trap_received = true;

   return true;
}

bool SnmpDriver::powernet_snmp_ups_check_state()
{
   fd_set fds;
   int numfds, rc, block;
   struct timeval tmo, exit, now;
   int sleep_time;

   /* Check for commlost under lock since UPS status might be changed */
   write_lock(_ups);
   rc = powernet_check_comm_lost();
   write_unlock(_ups);
   if (rc == 0)
      return false;

   sleep_time = ups->wait_time;

   /* If we're not doing SNMP traps, just sleep and exit */
   if (!_trap_session) {
      sleep(sleep_time);
      return true;
   }

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += sleep_time;

   while(1)
   {
      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_usec >= exit.tv_usec)) {
         /* Done already? How time flies... */
         return 0;
      }

      tmo.tv_sec = exit.tv_sec - now.tv_sec;
      tmo.tv_usec = exit.tv_usec - now.tv_usec;
      if (tmo.tv_usec < 0) {
         tmo.tv_sec--;              /* Normalize */
         tmo.tv_usec += 1000000;
      }

      /* Get select parameters from SNMP library */
      FD_ZERO(&fds);
      block = 0;
      numfds = 0;
      snmp_select_info(&numfds, &fds, &tmo, &block);

      /* Wait for something to happen */
      rc = select(numfds, &fds, NULL, NULL, &tmo);
      switch (rc) {
      case 0:  /* Timeout */
         /* Tell SNMP library about the timeout */
         snmp_timeout();
         break;

      case -1: /* Error */
         if (errno == EINTR || errno == EAGAIN)
            continue;            /* assume SIGCHLD */

         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         return false;

      default: /* Data available */
         /* Reset trap flag and run callback processing */
         _trap_received = false;
         snmp_read(&fds);

         /* If callback processing set the flag, we got a trap */
         if (_trap_received)
            return true;

         break;
      }
   }
}

bool SnmpDriver::powernet_snmp_ups_open()
{
   struct snmp_session *s = &_session;
   struct snmp_session tmp;

   /*
    * If we're configured to not use traps, simply rename
    * DeviceVendor to 'APC' and exit.
    */
   if (!strcmp(_DeviceVendor, "APC_NOTRAP")) {
      _DeviceVendor[3] = '\0';
      Dmsg0(100, "User requested no traps\n");
      return true;
   }

   /* Trap session is a copy of client session with some tweaks */
   tmp = *s;
   tmp.peername = (char*)"0.0.0.0:162";  /* Listen to snmptrap port on all interfaces */
   tmp.local_port = 1;                   /* We're a server, not a client */
   tmp.callback = powernet_snmp_callback;
   tmp.callback_magic = this;

   /*
    * Open the trap session and store it in my_data.
    * It's ok if this fails; the code will fall back to polling.
    */
   _trap_session = snmp_open(&tmp);
   if (!_trap_session) {
      Dmsg0(100, "Trap session failed to open\n");
      return true;
   }

   return true;
}
