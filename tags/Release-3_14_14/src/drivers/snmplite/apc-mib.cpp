/*
 * mib.cpp
 *
 * CI -> OID mapping for SNMP Lite UPS driver
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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
#include "snmplite-common.h"
#include "mibs.h"
#include "apc-oids.h"

using namespace Asn;

static struct CiOidMap CiOidMap[] =
{
//  CI                  OID                              type         dynamic?
   {CI_UPSMODEL,        upsBasicIdentModel,              OCTETSTRING, false},
   {CI_SERNO,           upsAdvIdentSerialNumber,         OCTETSTRING, false},
   {CI_IDEN,            upsBasicIdentName,               OCTETSTRING, false},
   {CI_REVNO,           upsAdvIdentFirmwareRevision,     OCTETSTRING, false},
   {CI_MANDAT,          upsAdvIdentDateOfManufacture,    OCTETSTRING, false},
   {CI_BATTDAT,         upsBasicBatteryLastReplaceDate,  OCTETSTRING, false},
   {CI_NOMBATTV,        upsAdvBatteryNominalVoltage,     INTEGER,     false},
   {CI_NOMOUTV,         upsAdvConfigRatedOutputVoltage,  INTEGER,     false},
   {CI_LTRANS,          upsAdvConfigLowTransferVolt,     INTEGER,     false},
   {CI_HTRANS,          upsAdvConfigHighTransferVolt,    INTEGER,     false},
   {CI_DWAKE,           upsAdvConfigReturnDelay,         TIMETICKS,   false},
   {CI_AlarmTimer,      upsAdvConfigAlarmTimer,          TIMETICKS,   false}, // Must be before CI_DALARM
   {CI_DALARM,          upsAdvConfigAlarm,               INTEGER,     false},
   {CI_DLBATT,          upsAdvConfigLowBatteryRunTime,   TIMETICKS,   false},
   {CI_DSHUTD,          upsAdvConfigShutoffDelay,        TIMETICKS,   false},
   {CI_RETPCT,          upsAdvConfigMinReturnCapacity,   INTEGER,     false},
   {CI_SENS,            upsAdvConfigSensitivity,         INTEGER,     false},
   {CI_EXTBATTS,        upsAdvBatteryNumOfBattPacks,     INTEGER,     false},
   {CI_STESTI,          upsAdvTestDiagnosticSchedule,    INTEGER,     false},
   {CI_VLINE,           upsAdvInputLineVoltage,          GAUGE,       true },
   {CI_VOUT,            upsAdvOutputVoltage,             GAUGE,       true },
   {CI_VBATT,           upsAdvBatteryActualVoltage,      INTEGER,     true },
   {CI_FREQ,            upsAdvInputFrequency,            GAUGE,       true },
   {CI_LOAD,            upsAdvOutputLoad,                GAUGE,       true },
   {CI_ITEMP,           upsAdvBatteryTemperature,        GAUGE,       true },
   {CI_ATEMP,           mUpsEnvironAmbientTemperature,   GAUGE,       true },
   {CI_HUMID,           mUpsEnvironRelativeHumidity,     GAUGE,       true },
   {CI_ST_STAT,         upsAdvTestDiagnosticsResults,    INTEGER,     true },
   {CI_BATTLEV,         upsAdvBatteryCapacity,           GAUGE,       true },
   {CI_RUNTIM,          upsAdvBatteryRunTimeRemaining,   TIMETICKS,   true },
   {CI_WHY_BATT,        upsAdvInputLineFailCause,        INTEGER,     true },
   {CI_BADBATTS,        upsAdvBatteryNumOfBadBattPacks,  INTEGER,     true },
   {CI_VMIN,            upsAdvInputMinLineVoltage,       GAUGE,       true },
   {CI_VMAX,            upsAdvInputMaxLineVoltage,       GAUGE,       true },

   // These 5 collectively are used to obtain the data for CI_STATUS.
   // All bits are available in upsBasicStateOutputState at once but 
   // the old AP960x cards do not appear to support that OID, so we use 
   // it only for the overload flag which is not available elsewhere.
   {CI_STATUS,          upsBasicOutputStatus,            INTEGER,     true },
   {CI_NeedReplacement, upsAdvBatteryReplaceIndicator,   INTEGER,     true },
   {CI_LowBattery,      upsBasicBatteryStatus,           INTEGER,     true },
   {CI_Calibration,     upsAdvTestCalibrationResults,    INTEGER,     true },
   {CI_Overload,        upsBasicStateOutputState,        OCTETSTRING, true },

   {-1, NULL, false}   /* END OF TABLE */
};

#define TIMETICKS_TO_SECS 100
#define SECS_TO_MINS      60

// Seen in the field: ITEMP OID occasionally sent by UPS is bogus. Value
// appears to come from CI_RUNTIM OID. Confirmed via Wireshark that error is on 
// UPS Web/SNMP card side. As a workaround in apcupsd we will filter out 
// obviously bogus ITEMPs.
//
// Issue was observed on:
//    UPS: Smart-UPS RT 5000 XL
//    SNMP module:
//     MN:AP9619 HR:A10 MD:07/12/2007
//     MB:v3.8.6 PF:v3.5.5 PN:apc_hw02_aos_355.bin AF1:v3.5.5 AN1:apc_hw02_sumx_355.bin
#define MAX_SANE_ITEMP    200

static void apc_update_ci(UPSINFO *ups, int ci, Snmp::Variable &data)
{
   static unsigned int alarmtimer = 0;

   switch (ci)
   {
   case CI_VLINE:
      Dmsg(80, "Got CI_VLINE: %d\n", data.u32);
      ups->LineVoltage = data.u32;
      break;

   case CI_VOUT:
      Dmsg(80, "Got CI_VOUT: %d\n", data.u32);
      ups->OutputVoltage = data.u32;
      break;

   case CI_VBATT:
      Dmsg(80, "Got CI_VBATT: %d\n", data.u32);
      ups->BattVoltage = data.u32;
      break;

   case CI_FREQ:
      Dmsg(80, "Got CI_FREQ: %d\n", data.u32);
      ups->LineFreq = data.u32;
      break;

   case CI_LOAD:
      Dmsg(80, "Got CI_LOAD: %d\n", data.u32);
      ups->UPSLoad = data.u32;
      break;

   case CI_ITEMP:
      Dmsg(80, "Got CI_ITEMP: %d\n", data.u32);
      if (data.u32 <= MAX_SANE_ITEMP)
         ups->UPSTemp = data.u32;
      else
         Dmsg(10, "Ignoring out-of-range ITEMP: %d\n", data.u32);
      break;

   case CI_ATEMP:
      Dmsg(80, "Got CI_ATEMP: %d\n", data.u32);
      ups->ambtemp = data.u32;
      break;

   case CI_HUMID:
      Dmsg(80, "Got CI_HUMID: %d\n", data.u32);
      ups->humidity = data.u32;
      break;

   case CI_NOMBATTV:
      Dmsg(80, "Got CI_NOMBATTV: %d\n", data.u32);
      ups->nombattv = data.u32;
      break;

   case CI_NOMOUTV:
      Dmsg(80, "Got CI_NOMOUTV: %d\n", data.u32);
      ups->NomOutputVoltage = data.u32;
      break;

   case CI_NOMINV:
      Dmsg(80, "Got CI_NOMINV: %d\n", data.u32);
      ups->NomInputVoltage = data.u32;
      break;

   case CI_NOMPOWER:
      Dmsg(80, "Got CI_NOMPOWER: %d\n", data.u32);
      ups->NomPower = data.u32;
      break;

   case CI_LTRANS:
      Dmsg(80, "Got CI_LTRANS: %d\n", data.u32);
      ups->lotrans = data.u32;
      break;

   case CI_HTRANS:
      Dmsg(80, "Got CI_HTRANS: %d\n", data.u32);
      ups->hitrans = data.u32;
      break;

   case CI_DWAKE:
      Dmsg(80, "Got CI_DWAKE: %d\n", data.u32);
      ups->dwake = data.u32;
      break;

   case CI_ST_STAT:
      Dmsg(80, "Got CI_ST_STAT: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
      case 3:  /* Invalid test */
         ups->testresult = TEST_FAILED;
         break;
      case 4:  /* Test in progress */
         ups->testresult = TEST_INPROGRESS;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }
      break;

   case CI_AlarmTimer:
      Dmsg(80, "Got CI_AlarmTimer: %d\n", data.u32);
      // Remember alarm timer setting; we will use it for CI_DALARM
      alarmtimer = data.u32;
      break;

   case CI_DALARM:
      Dmsg(80, "Got CI_DALARM: %d\n", data.u32);
      switch (data.u32)
      {
      case 1: // Timed (uses CI_AlarmTimer)
         if (ups->UPS_Cap[CI_AlarmTimer] && alarmtimer < 30)
            strlcpy(ups->beepstate, "0", sizeof(ups->beepstate)); // 5 secs
         else
            strlcpy(ups->beepstate, "T", sizeof(ups->beepstate)); // 30 secs
         break;
      case 2: // LowBatt
         strlcpy(ups->beepstate, "L", sizeof(ups->beepstate));
         break;
      case 3: // None
         strlcpy(ups->beepstate, "N", sizeof(ups->beepstate));
         break;
      default:
         strlcpy(ups->beepstate, "T", sizeof(ups->beepstate));
         break;
      }
      break;

   case CI_UPSMODEL:
      Dmsg(80, "Got CI_UPSMODEL: %s\n", data.str.str());
      strlcpy(ups->upsmodel, data.str, sizeof(ups->upsmodel));
      break;

   case CI_SERNO:
      Dmsg(80, "Got CI_SERNO: %s\n", data.str.str());
      strlcpy(ups->serial, data.str, sizeof(ups->serial));
      break;

   case CI_MANDAT:
      Dmsg(80, "Got CI_MANDAT: %s\n", data.str.str());
      strlcpy(ups->birth, data.str, sizeof(ups->birth));
      break;

   case CI_BATTLEV:
      Dmsg(80, "Got CI_BATTLEV: %d\n", data.u32);
      ups->BattChg = data.u32;
      break;

   case CI_RUNTIM:
      Dmsg(80, "Got CI_RUNTIM: %d\n", data.u32);
      ups->TimeLeft = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_BATTDAT:
      Dmsg(80, "Got CI_BATTDAT: %s\n", data.str.str());
      strlcpy(ups->battdat, data.str, sizeof(ups->battdat));
      break;

   case CI_IDEN:
      Dmsg(80, "Got CI_IDEN: %s\n", data.str.str());
      strlcpy(ups->upsname, data.str, sizeof(ups->upsname));
      break;

   case CI_STATUS:
      Dmsg(80, "Got CI_STATUS: %d\n", data.u32);
      /* Clear the following flags: only one status will be TRUE */
      ups->clear_online();
      ups->clear_onbatt();
      ups->clear_boost();
      ups->clear_trim();
      switch (data.u32) {
      case 2:
         ups->set_online();
         break;
      case 3:
         ups->set_onbatt();
         break;
      case 4:
         ups->set_online();
         ups->set_boost();
         break;
      case 12:
         ups->set_online();
         ups->set_trim();
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
      break;

   case CI_NeedReplacement:
      Dmsg(80, "Got CI_NeedReplacement: %d\n", data.u32);
      if (data.u32 == 2)
         ups->set_replacebatt();
      else
         ups->clear_replacebatt();
      break;

   case CI_LowBattery:
      Dmsg(80, "Got CI_LowBattery: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_battlow();
      else
         ups->clear_battlow();
      break;

   case CI_Calibration:
      Dmsg(80, "Got CI_Calibration: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_calibration();
      else
         ups->clear_calibration();
      break;

   case CI_Overload:
      Dmsg(80, "Got CI_Overload: %c\n", data.str[8]);
      if (data.str[8] == '1')
         ups->set_overload();
      else
         ups->clear_overload();
      break;

   case CI_DSHUTD:
      Dmsg(80, "Got CI_DSHUTD: %d\n", data.u32);
      ups->dshutd = data.u32 / TIMETICKS_TO_SECS;
      break;

   case CI_RETPCT:
      Dmsg(80, "Got CI_RETPCT: %d\n", data.u32);
      ups->rtnpct = data.u32;
      break;

   case CI_WHY_BATT:
      Dmsg(80, "Got CI_WHY_BATT: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:
         ups->lastxfer = XFER_NONE;
         break;
      case 2:  /* High line voltage */
         ups->lastxfer = XFER_OVERVOLT;
         break;
      case 3:  /* Brownout */
      case 4:  /* Blackout */
         ups->lastxfer = XFER_UNDERVOLT;
         break;
      case 5:  /* Small sag */
      case 6:  /* Deep sag */
      case 7:  /* Small spike */
      case 8:  /* Deep spike */
         ups->lastxfer = XFER_NOTCHSPIKE;
         break;
      case 9:
         ups->lastxfer = XFER_SELFTEST;
         break;
      case 10:
         ups->lastxfer = XFER_RIPPLE;
         break;
      default:
         ups->lastxfer = XFER_UNKNOWN;
         break;
      }
      break;

   case CI_SENS:
      Dmsg(80, "Got CI_SENS: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:
         strlcpy(ups->sensitivity, "Auto", sizeof(ups->sensitivity));
         break;
      case 2:
         strlcpy(ups->sensitivity, "Low", sizeof(ups->sensitivity));
         break;
      case 3:
         strlcpy(ups->sensitivity, "Medium", sizeof(ups->sensitivity));
         break;
      case 4:
         strlcpy(ups->sensitivity, "High", sizeof(ups->sensitivity));
         break;
      default:
         strlcpy(ups->sensitivity, "Unknown", sizeof(ups->sensitivity));
         break;
      }
      break;

   case CI_REVNO:
      Dmsg(80, "Got CI_REVNO: %s\n", data.str.str());
      strlcpy(ups->firmrev, data.str, sizeof(ups->firmrev));
      break;

   case CI_EXTBATTS:
      Dmsg(80, "Got CI_EXTBATTS: %d\n", data.u32);
      ups->extbatts = data.u32;
      break;
   
   case CI_BADBATTS:
      Dmsg(80, "Got CI_BADBATTS: %d\n", data.u32);
      ups->badbatts = data.u32;
      break;

   case CI_DLBATT:
      Dmsg(80, "Got CI_DLBATT: %d\n", data.u32);
      ups->dlowbatt = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_STESTI:
      Dmsg(80, "Got CI_STESTI: %d\n", data.u32);
      switch (data.u32) {
      case 2:
         strlcpy(ups->selftest, "336", sizeof(ups->selftest));
         break;
      case 3:
         strlcpy(ups->selftest, "168", sizeof(ups->selftest));
         break;
      case 4:
         strlcpy(ups->selftest, "ON", sizeof(ups->selftest));
         break;
      case 1:
      case 5:
      default:
         strlcpy(ups->selftest, "OFF", sizeof(ups->selftest));
         break;
      }
      break;

   case CI_VMIN:
      Dmsg(80, "Got CI_VMIN: %d\n", data.u32);
      ups->LineMin = data.u32;
      break;

   case CI_VMAX:
      Dmsg(80, "Got CI_VMAX: %d\n", data.u32);
      ups->LineMax = data.u32;
      break;
   }
}

static int apc_killpower(Snmp::SnmpEngine *snmp)
{
   Snmp::Variable var(Asn::INTEGER, 2);
   return snmp->Set(upsBasicControlConserveBattery, &var);
}

static int apc_shutdown(Snmp::SnmpEngine *snmp)
{
   Snmp::Variable var(Asn::INTEGER, 2);
   return snmp->Set(upsAdvControlUpsOff, &var);
}

// Export strategy to snmplite.cpp
struct MibStrategy ApcMibStrategy =
{
   "APC",
   CiOidMap,
   apc_update_ci,
   apc_killpower,
   apc_shutdown,
};
