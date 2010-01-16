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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "snmplite.h"
#include "oids.h"

using namespace Asn;

struct CiOidMap CiOidMap[] =
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

// The OID used to issue the killpower (hibernate) command
const int *KillPowerOid = upsBasicControlConserveBattery;

// The OID used to issue the shutdown command
const int *ShutdownOid = upsAdvControlUpsOff;
