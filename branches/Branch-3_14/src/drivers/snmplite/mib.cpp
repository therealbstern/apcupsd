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

struct CiOidMap CiOidMap[] =
{
//  CI                  OID                              dynamic?
   {CI_UPSMODEL,        upsBasicIdentModel,              false},
   {CI_SERNO,           upsAdvIdentSerialNumber,         false},
   {CI_IDEN,            upsBasicIdentName,               false},
   {CI_REVNO,           upsAdvIdentFirmwareRevision,     false},
   {CI_MANDAT,          upsAdvIdentDateOfManufacture,    false},
   {CI_BATTDAT,         upsBasicBatteryLastReplaceDate,  false},
   {CI_NOMBATTV,        upsAdvBatteryNominalVoltage,     false},
   {CI_NOMOUTV,         upsAdvConfigRatedOutputVoltage,  false},
   {CI_LTRANS,          upsAdvConfigLowTransferVolt,     false},
   {CI_HTRANS,          upsAdvConfigHighTransferVolt,    false},
   {CI_DWAKE,           upsAdvConfigReturnDelay,         false},
   {CI_DALARM,          upsAdvConfigAlarm,               false},
   {CI_DLBATT,          upsAdvConfigLowBatteryRunTime,   false},
   {CI_DSHUTD,          upsAdvConfigShutoffDelay,        false},
   {CI_RETPCT,          upsAdvConfigMinReturnCapacity,   false},
   {CI_SENS,            upsAdvConfigSensitivity,         false},
   {CI_EXTBATTS,        upsAdvBatteryNumOfBattPacks,     false},
   {CI_STESTI,          upsAdvTestDiagnosticSchedule,    false},
   {CI_VLINE,           upsAdvInputLineVoltage,          true },
   {CI_VOUT,            upsAdvOutputVoltage,             true },
   {CI_VBATT,           upsAdvBatteryActualVoltage,      true },
   {CI_FREQ,            upsAdvInputFrequency,            true },
   {CI_LOAD,            upsAdvOutputLoad,                true },
   {CI_ITEMP,           upsAdvBatteryTemperature,        true },
   {CI_ATEMP,           mUpsEnvironAmbientTemperature,   true },
   {CI_HUMID,           mUpsEnvironRelativeHumidity,     true },
   {CI_ST_STAT,         upsAdvTestDiagnosticsResults,    true },
   {CI_BATTLEV,         upsAdvBatteryCapacity,           true },
   {CI_RUNTIM,          upsAdvBatteryRunTimeRemaining,   true },
   {CI_WHY_BATT,        upsAdvInputLineFailCause,        true },
   {CI_BADBATTS,        upsAdvBatteryNumOfBadBattPacks,  true },
   {CI_VMIN,            upsAdvInputMinLineVoltage,       true },
   {CI_VMAX,            upsAdvInputMaxLineVoltage,       true },

   // These 5 collectively are used to obtain the data for CI_STATUS.
   // All bits are available in upsBasicStateOutputState at once but 
   // the old AP960x cards do not appear to support that OID, so we use 
   // it only for the overload flag which is not available elsewhere.
   {CI_STATUS,          upsBasicOutputStatus,            true },
   {CI_NeedReplacement, upsAdvBatteryReplaceIndicator,   true },
   {CI_LowBattery,      upsBasicBatteryStatus,           true },
   {CI_Calibration,     upsAdvTestCalibrationResults,    true },
   {CI_Overload,        upsBasicStateOutputState,        true },

   {-1, NULL, false}   /* END OF TABLE */
};
