/*
 * mib.c
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

__attribute__((unused)) static int upsBasicIdentModel[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 1, 1, 0, -1};
__attribute__((unused)) static int upsBasicIdentName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 1, 2, 0, -1};
__attribute__((unused)) static int upsAdvIdentFirmwareRevision[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvIdentDateOfManufacture[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvIdentSerialNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvIdentFirmwareRevision2[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvIdentSkuNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 1, 2, 5, 0, -1};
__attribute__((unused)) static int upsBasicBatteryStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsBasicBatteryTimeOnBattery[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsBasicBatteryLastReplaceDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsAdvBatteryCapacity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvBatteryTemperature[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvBatteryRunTimeRemaining[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvBatteryReplaceIndicator[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvBatteryNumOfBattPacks[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvBatteryNumOfBadBattPacks[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 6, 0, -1};
__attribute__((unused)) static int upsAdvBatteryNominalVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 7, 0, -1};
__attribute__((unused)) static int upsAdvBatteryActualVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 8, 0, -1};
__attribute__((unused)) static int upsAdvBatteryCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 9, 0, -1};
__attribute__((unused)) static int upsAdvTotalDCCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 2, 10, 0, -1};
__attribute__((unused)) static int upsHighPrecBatteryCapacity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 1, 0, -1};
__attribute__((unused)) static int upsHighPrecBatteryTemperature[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 2, 0, -1};
__attribute__((unused)) static int upsHighPrecBatteryNominalVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 3, 0, -1};
__attribute__((unused)) static int upsHighPrecBatteryActualVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 4, 0, -1};
__attribute__((unused)) static int upsHighPrecBatteryCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 5, 0, -1};
__attribute__((unused)) static int upsHighPrecTotalDCCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 2, 3, 6, 0, -1};
__attribute__((unused)) static int upsBasicInputPhase[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 1, 1, 0, -1};
__attribute__((unused)) static int upsAdvInputLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvInputMaxLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvInputMinLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvInputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvInputLineFailCause[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 2, 5, 0, -1};
__attribute__((unused)) static int upsHighPrecInputLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 3, 1, 0, -1};
__attribute__((unused)) static int upsHighPrecInputMaxLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 3, 2, 0, -1};
__attribute__((unused)) static int upsHighPrecInputMinLineVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 3, 3, 0, -1};
__attribute__((unused)) static int upsHighPrecInputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 3, 3, 4, 0, -1};
__attribute__((unused)) static int upsBasicOutputStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 1, 1, 0, -1};
__attribute__((unused)) static int upsBasicOutputPhase[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 1, 2, 0, -1};
__attribute__((unused)) static int upsBasicSystemStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 1, 3, 0, -1};
__attribute__((unused)) static int upsAdvOutputVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvOutputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvOutputLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvOutputCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvOutputRedundancy[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvOutputKVACapacity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 2, 6, 0, -1};
__attribute__((unused)) static int upsHighPrecOutputVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 3, 1, 0, -1};
__attribute__((unused)) static int upsHighPrecOutputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 3, 2, 0, -1};
__attribute__((unused)) static int upsHighPrecOutputLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 3, 3, 0, -1};
__attribute__((unused)) static int upsHighPrecOutputCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 4, 3, 4, 0, -1};
__attribute__((unused)) static int upsBasicConfigNumDevices[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 1, 1, 0, -1};
__attribute__((unused)) static int deviceIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 1, 2, 1, 1, 0, -1};
__attribute__((unused)) static int deviceName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 1, 2, 1, 2, 0, -1};
__attribute__((unused)) static int vaRating[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 1, 2, 1, 3, 0, -1};
__attribute__((unused)) static int acceptThisDevice[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 1, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsAdvConfigRatedOutputVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvConfigHighTransferVolt[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvConfigLowTransferVolt[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvConfigAlarm[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvConfigAlarmTimer[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvConfigMinReturnCapacity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 6, 0, -1};
__attribute__((unused)) static int upsAdvConfigSensitivity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 7, 0, -1};
__attribute__((unused)) static int upsAdvConfigLowBatteryRunTime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 8, 0, -1};
__attribute__((unused)) static int upsAdvConfigReturnDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 9, 0, -1};
__attribute__((unused)) static int upsAdvConfigShutoffDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 10, 0, -1};
__attribute__((unused)) static int upsAdvConfigUpsSleepTime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 11, 0, -1};
__attribute__((unused)) static int upsAdvConfigSetEEPROMDefaults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 12, 0, -1};
__attribute__((unused)) static int dipSwitchIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 13, 1, 1, 0, -1};
__attribute__((unused)) static int dipSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 13, 1, 2, 0, -1};
__attribute__((unused)) static int upsAdvConfigBattExhaustThresh[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 14, 0, -1};
__attribute__((unused)) static int upsAdvConfigPassword[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 15, 0, -1};
__attribute__((unused)) static int apcUpsConfigFieldIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 16, 1, 1, 0, -1};
__attribute__((unused)) static int apcUpsConfigFieldOID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 16, 1, 2, 0, -1};
__attribute__((unused)) static int apcUpsConfigFieldValueRange[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 16, 1, 3, 0, -1};
__attribute__((unused)) static int upsAdvConfigBattCabAmpHour[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 17, 0, -1};
__attribute__((unused)) static int upsAdvConfigPositionSelector[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 18, 0, -1};
__attribute__((unused)) static int upsAdvConfigOutputFreqRange[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 19, 0, -1};
__attribute__((unused)) static int upsAdvConfigUPSFail[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 20, 0, -1};
__attribute__((unused)) static int upsAdvConfigAlarmRedundancy[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 21, 0, -1};
__attribute__((unused)) static int upsAdvConfigAlarmLoadOver[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 22, 0, -1};
__attribute__((unused)) static int upsAdvConfigAlarmRuntimeUnder[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 23, 0, -1};
__attribute__((unused)) static int upsAdvConfigVoutReporting[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 24, 0, -1};
__attribute__((unused)) static int upsAdvConfigNumExternalBatteries[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 25, 0, -1};
__attribute__((unused)) static int upsAdvConfigSimpleSignalShutdowns[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 26, 0, -1};
__attribute__((unused)) static int upsAdvConfigMaxShutdownTime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 27, 0, -1};
__attribute__((unused)) static int upsAsiUpsControlServerRequestShutdown[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 28, 0, -1};
__attribute__((unused)) static int upsAdvConfigMinReturnRuntime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 29, 0, -1};
__attribute__((unused)) static int upsAdvConfigBasicSignalLowBatteryDuration[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 30, 0, -1};
__attribute__((unused)) static int upsAdvConfigBypassPhaseLockRequired[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 31, 0, -1};
__attribute__((unused)) static int upsAdvConfigOutputFreqSlewRate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 32, 0, -1};
__attribute__((unused)) static int upsAdvConfigChargerLevel[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 33, 0, -1};
__attribute__((unused)) static int upsAdvConfigBypassToleranceSetting[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 34, 0, -1};
__attribute__((unused)) static int upsAdvConfigMainsSetting[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 35, 0, -1};
__attribute__((unused)) static int upsAdvConfigACWiringSetting[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 36, 0, -1};
__attribute__((unused)) static int upsAdvConfigUpperOutputVoltTolerance[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 37, 0, -1};
__attribute__((unused)) static int upsAdvConfigLowerOutputVoltTolerance[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 38, 0, -1};
__attribute__((unused)) static int upsAdvConfigUpperBypassVoltTolerance[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 39, 0, -1};
__attribute__((unused)) static int upsAdvConfigLowerBypassVoltTolerance[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 40, 0, -1};
__attribute__((unused)) static int upsAdvConfigOutofSyncBypassTransferDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 5, 2, 41, 0, -1};
__attribute__((unused)) static int upsBasicControlConserveBattery[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 1, 1, 0, -1};
__attribute__((unused)) static int upsAdvControlUpsOff[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvControlRebootShutdownUps[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvControlUpsSleep[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvControlSimulatePowerFail[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvControlFlashAndBeep[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvControlTurnOnUPS[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 6, 0, -1};
__attribute__((unused)) static int upsAdvControlBypassSwitch[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 7, 0, -1};
__attribute__((unused)) static int upsAdvControlRebootUpsWithOrWithoutAC[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 8, 0, -1};
__attribute__((unused)) static int upsAdvControlFirmwareUpdate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 2, 9, 0, -1};
__attribute__((unused)) static int upsAdvTestDiagnosticSchedule[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvTestDiagnostics[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvTestDiagnosticsResults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvTestLastDiagnosticsDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvTestRuntimeCalibration[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvTestCalibrationResults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 6, 0, -1};
__attribute__((unused)) static int upsAdvTestCalibrationDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 7, 0, -1};
__attribute__((unused)) static int upsAdvTestDiagnosticTime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 8, 0, -1};
__attribute__((unused)) static int upsAdvTestDiagnosticDay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 7, 2, 9, 0, -1};
__attribute__((unused)) static int upsCommStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 8, 1, 0, -1};
__attribute__((unused)) static int upsPhaseResetMaxMinValues[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 1, 1, 0, -1};
__attribute__((unused)) static int upsPhaseNumInputs[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 1, 0, -1};
__attribute__((unused)) static int upsPhaseInputTableIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsPhaseNumInputPhases[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsPhaseInputVoltageOrientation[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsPhaseInputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsPhaseInputType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsPhaseInputName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsPhaseInputPhaseTableIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 1, 0, -1};
__attribute__((unused)) static int upsPhaseInputPhaseIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 2, 0, -1};
__attribute__((unused)) static int upsPhaseInputVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 3, 0, -1};
__attribute__((unused)) static int upsPhaseInputMaxVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 4, 0, -1};
__attribute__((unused)) static int upsPhaseInputMinVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 5, 0, -1};
__attribute__((unused)) static int upsPhaseInputCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 6, 0, -1};
__attribute__((unused)) static int upsPhaseInputMaxCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 7, 0, -1};
__attribute__((unused)) static int upsPhaseInputMinCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 8, 0, -1};
__attribute__((unused)) static int upsPhaseInputPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 9, 0, -1};
__attribute__((unused)) static int upsPhaseInputMaxPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 10, 0, -1};
__attribute__((unused)) static int upsPhaseInputMinPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 2, 3, 1, 11, 0, -1};
__attribute__((unused)) static int upsPhaseNumOutputs[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 1, 0, -1};
__attribute__((unused)) static int upsPhaseOutputTableIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsPhaseNumOutputPhases[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsPhaseOutputVoltageOrientation[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsPhaseOutputFrequency[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsPhaseOutputPhaseTableIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 1, 0, -1};
__attribute__((unused)) static int upsPhaseOutputPhaseIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 2, 0, -1};
__attribute__((unused)) static int upsPhaseOutputVoltage[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 3, 0, -1};
__attribute__((unused)) static int upsPhaseOutputCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 4, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMaxCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 5, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMinCurrent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 6, 0, -1};
__attribute__((unused)) static int upsPhaseOutputLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 7, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMaxLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 8, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMinLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 9, 0, -1};
__attribute__((unused)) static int upsPhaseOutputPercentLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 10, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMaxPercentLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 11, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMinPercentLoad[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 12, 0, -1};
__attribute__((unused)) static int upsPhaseOutputPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 13, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMaxPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 14, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMinPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 15, 0, -1};
__attribute__((unused)) static int upsPhaseOutputPercentPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 16, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMaxPercentPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 17, 0, -1};
__attribute__((unused)) static int upsPhaseOutputMinPercentPower[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 9, 3, 3, 1, 18, 0, -1};
__attribute__((unused)) static int upsSCGMembershipGroupNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 1, 1, 0, -1};
__attribute__((unused)) static int upsSCGActiveMembershipStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 1, 2, 0, -1};
__attribute__((unused)) static int upsSCGPowerSynchronizationDelayTime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 1, 3, 0, -1};
__attribute__((unused)) static int upsSCGReturnBatteryCapacityOffset[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 1, 4, 0, -1};
__attribute__((unused)) static int upsSCGMultiCastIP[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 1, 5, 0, -1};
__attribute__((unused)) static int upsSCGNumOfGroupMembers[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 2, 1, 0, -1};
__attribute__((unused)) static int upsSCGStatusTableIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 2, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsSCGMemberIP[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 2, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsSCGACInputStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 2, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsSCGACOutputStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 10, 2, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsBasicStateOutputState[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 1, 1, 0, -1};
__attribute__((unused)) static int upsAdvStateAbnormalConditions[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 1, 0, -1};
__attribute__((unused)) static int upsAdvStateSymmetra3PhaseSpecificFaults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 2, 0, -1};
__attribute__((unused)) static int upsAdvStateDP300ESpecificFaults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 3, 0, -1};
__attribute__((unused)) static int upsAdvStateSymmetraSpecificFaults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 4, 0, -1};
__attribute__((unused)) static int upsAdvStateSmartUPSSpecificFaults[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 5, 0, -1};
__attribute__((unused)) static int upsAdvStateSystemMessages[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 11, 2, 6, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusGroupState[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusCommandPending[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsOutletGroupStatusOutletType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 1, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigPowerOnDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigPowerOffDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigRebootDuration[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigMinReturnRuntime[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigOutletType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 7, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedControlSkipOffDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 8, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedControlAutoRestart[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 9, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedControlTimeOnBattery[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 10, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedControlRuntimeRemaining[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 11, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedControlInOverload[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 12, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedTimeOnBattery[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 13, 0, -1};
__attribute__((unused)) static int upsOutletGroupConfigLoadShedRuntimeRemaining[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 2, 2, 1, 14, 0, -1};
__attribute__((unused)) static int upsOutletGroupControlTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 3, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupControlIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 3, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsOutletGroupControlName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 3, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsOutletGroupControlCommand[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 3, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsOutletGroupControlOutletType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 12, 3, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagIMTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagIMIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagIMType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagIMFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagIMSlaveFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagIMHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagIMSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagIMManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 1, 2, 1, 8, 0, -1};
__attribute__((unused)) static int upsDiagPMTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 1, 0, -1};
__attribute__((unused)) static int upsDiagPMIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagPMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagPMFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagPMHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagPMSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagPMManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 2, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagBatteryTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 1, 0, -1};
__attribute__((unused)) static int upsDiagBatteryFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagBatteryIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagBatteryStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagBatterySerialNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagBatteryFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagBatteryManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagBatteryType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 3, 2, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameType[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysFrameManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 2, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysIntBypSwitchManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 4, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysBattMonitorManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 6, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalSwitchGearManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 8, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDisplayInterfaceCardTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 9, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDisplayInterfaceCardFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 10, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDisplayInterfaceCardIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 10, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDisplayInterfaceCardStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 10, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDCCircuitBreakerTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 11, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDCCircuitBreakerFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 12, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDCCircuitBreakerIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 12, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysDCCircuitBreakerStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 12, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 13, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplySerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysSystemPowerSupplyManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 14, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 15, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 16, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 16, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 16, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 16, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysXRCommunicationCardSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 16, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalPowerFrameBoardTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 17, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalPowerFrameBoardFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 18, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalPowerFrameBoardIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 18, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysExternalPowerFrameBoardStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 18, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysChargerTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 19, 0, -1};
__attribute__((unused)) static int upsDiagSubSysChargerFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 20, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysChargerIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 20, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysChargerStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 20, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 21, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysInverterManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 22, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 23, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionFirmwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionSerialNum[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysPowerFactorCorrectionManufactureDate[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 24, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 25, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardModelNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardSerialNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardDateOfManufacture[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardHardwareRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardFirmwareAppRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 6, 0, -1};
__attribute__((unused)) static int upsDiagSubSysNetworkComCardFirmwareAppOSRev[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 4, 26, 1, 7, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 1, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearInputSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 2, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearOutputSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearBypassSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearBypassInputSwitchStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 5, 0, -1};
__attribute__((unused)) static int upsDiagSwitchGearBreakerTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 6, 0, -1};
__attribute__((unused)) static int switchgearBreakerIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 7, 1, 1, 0, -1};
__attribute__((unused)) static int switchgearBreakerPresent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 7, 1, 2, 0, -1};
__attribute__((unused)) static int switchgearBreakerName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 7, 1, 3, 0, -1};
__attribute__((unused)) static int upsDiagSubFeedBreakerTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 8, 0, -1};
__attribute__((unused)) static int subfeedBreakerIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 9, 1, 1, 0, -1};
__attribute__((unused)) static int subfeedBreakerPresent[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 9, 1, 2, 0, -1};
__attribute__((unused)) static int subfeedBreakerRating[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 9, 1, 3, 0, -1};
__attribute__((unused)) static int subfeedBreakerUpperAcceptPowerWarning[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 1, 9, 1, 4, 0, -1};
__attribute__((unused)) static int upsDiagMCCBBoxStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 2, 1, 0, -1};
__attribute__((unused)) static int upsDiagTransformerStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 5, 3, 1, 0, -1};
__attribute__((unused)) static int upsDiagComBusInternalMIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 6, 1, 0, -1};
__attribute__((unused)) static int upsDiagComBusInternalRIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 6, 2, 0, -1};
__attribute__((unused)) static int upsDiagComBusMIMtoRIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 6, 3, 0, -1};
__attribute__((unused)) static int upsDiagComBusExternalMIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 6, 4, 0, -1};
__attribute__((unused)) static int upsDiagComBusExternalRIMStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 13, 6, 5, 0, -1};
__attribute__((unused)) static int upsParallelSysLocalAddress[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 14, 1, 0, -1};
__attribute__((unused)) static int upsParallelSysRemoteAddress[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 14, 2, 0, -1};
__attribute__((unused)) static int upsParallelSysRedundancy[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 14, 3, 0, -1};
__attribute__((unused)) static int upsIOFrameLayoutPositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 1, 0, -1};
__attribute__((unused)) static int upsBottomFeedFrameLayoutPositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 2, 0, -1};
__attribute__((unused)) static int upsSwitchGearLayoutPositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 3, 0, -1};
__attribute__((unused)) static int upsBatteryFrameLayoutTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 4, 0, -1};
__attribute__((unused)) static int batteryFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 5, 1, 1, 0, -1};
__attribute__((unused)) static int batteryFramePositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 5, 1, 2, 0, -1};
__attribute__((unused)) static int upsSideCarFrameLayoutTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 6, 0, -1};
__attribute__((unused)) static int sideCarFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 7, 1, 1, 0, -1};
__attribute__((unused)) static int sideCarFramePositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 7, 1, 2, 0, -1};
__attribute__((unused)) static int upsPowerFrameLayoutTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 8, 0, -1};
__attribute__((unused)) static int powerFrameIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 9, 1, 1, 0, -1};
__attribute__((unused)) static int powerFramePositionID[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 15, 9, 1, 2, 0, -1};
__attribute__((unused)) static int upsIntegratedATSSelectedSource[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 1, 0, -1};
__attribute__((unused)) static int upsIntegratedATSPreferredSource[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 2, 0, -1};
__attribute__((unused)) static int upsIntegratedATSUpsReturnStaggering[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 3, 0, -1};
__attribute__((unused)) static int upsIntegratedATSSourceTableSize[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 4, 0, -1};
__attribute__((unused)) static int upsIntegratedATSSourceIndex[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 5, 1, 1, 0, -1};
__attribute__((unused)) static int upsIntegratedATSSourceName[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 5, 1, 2, 0, -1};
__attribute__((unused)) static int upsIntegratedATSSourceStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 5, 1, 3, 0, -1};
__attribute__((unused)) static int upsIntegratedATSLineFailDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 5, 1, 4, 0, -1};
__attribute__((unused)) static int upsIntegratedATSLineStabilityDelay[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 16, 5, 1, 5, 0, -1};
__attribute__((unused)) static int mUpsEnvironAmbientTemperature[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 1, 1, 0, -1};
__attribute__((unused)) static int mUpsEnvironRelativeHumidity[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 1, 2, 0, -1};
__attribute__((unused)) static int mUpsEnvironAmbientTemperature2[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 1, 3, 0, -1};
__attribute__((unused)) static int mUpsEnvironRelativeHumidity2[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 1, 4, 0, -1};
__attribute__((unused)) static int mUpsContactNumContacts[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 1, 0, -1};
__attribute__((unused)) static int contactNumber[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 2, 1, 1, 0, -1};
__attribute__((unused)) static int normalState[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 2, 1, 2, 0, -1};
__attribute__((unused)) static int description[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 2, 1, 3, 0, -1};
__attribute__((unused)) static int monitoringStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 2, 1, 4, 0, -1};
__attribute__((unused)) static int currentStatus[] = {1, 3, 6, 1, 4, 1, 318, 1, 1, 2, 2, 2, 1, 5, 0, -1};

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
