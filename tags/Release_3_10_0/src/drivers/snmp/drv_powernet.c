/*
 *  drv_powernet.c -- PowerNet driver
 *  Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@apcupsd.org>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
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
#include "snmp.h"
#include "snmp_private.h"

static int powernet_check_comm_lost(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    powernet_mib_t *data = Sid->MIB;
    int ret = 1;

    /*
     * Check the Ethernet COMMLOST first, then check the
     * Web/SNMP->UPS serial COMMLOST.
     */
    if (powernet_mib_mgr_get_upsComm(s, &(data->upsComm)) == -1) {
        UPS_SET(UPS_COMMLOST);
        ret = 0;
        goto out;
    } else {
        UPS_CLEAR(UPS_COMMLOST);
    }
    if (data->upsComm) {
        if (data->upsComm->__upsCommStatus == 2) {
            UPS_SET(UPS_COMMLOST);
            ret = 0;
            goto out;
        } else {
            UPS_CLEAR(UPS_COMMLOST);
        }
    }
out:
    free(data->upsComm);
    return ret;
}


int powernet_snmp_kill_ups_power(UPSINFO *ups) {
    oid upsBasicControlConserveBattery[] =
                    {1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 1, 1};
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
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
            sizeof(upsBasicControlConserveBattery)/sizeof(oid), 'i', "2")) {
        return 0;
    }

    peer = snmp_open(s);

    if (!peer) {
        Dmsg0(0, "Can not open the SNMP connection.\n");
        return 0;
    }

    status = snmp_synch_response(peer, request, &response);
    
    if (status != STAT_SUCCESS) {
        Dmsg0(0, "Unable to communicate with UPS.\n");
        return 0;
    }

    if (response->errstat != SNMP_ERR_NOERROR) {
        Dmsg0(0, "Unable to kill UPS power: can not set SNMP variable.\n");
        return 0;
    }

    if (response) snmp_free_pdu(response);

    snmp_close(peer);

    return 1;
}

int powernet_snmp_ups_get_capabilities(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    powernet_mib_t *data = Sid->MIB;
    int i = 0;

    /*
     * Assume that an UPS with Web/SNMP card has all the capabilities.
     */
    for (i = 0; i <= CI_MAX_CAPS; i++) {
        ups->UPS_Cap[i] = TRUE;
    }
    return 1;
}

int powernet_snmp_ups_read_static_data(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    powernet_mib_t *data = Sid->MIB;

    if (powernet_check_comm_lost(ups) == 0) {
        return 0;
    }

    powernet_mib_mgr_get_upsBasicIdent(s, &(data->upsBasicIdent));
    if (data->upsBasicIdent) {
        strncpy(ups->upsmodel, data->upsBasicIdent->upsBasicIdentModel,
                sizeof(ups->upsmodel));
        strncpy(ups->upsname, data->upsBasicIdent->upsBasicIdentName,
                sizeof(ups->upsname));
        free(data->upsBasicIdent);
    }

    powernet_mib_mgr_get_upsAdvIdent(s, &(data->upsAdvIdent));
    if (data->upsAdvIdent) {
        strncpy(ups->firmrev, data->upsAdvIdent->upsAdvIdentFirmwareRevision,
                sizeof(ups->firmrev));
        strncpy(ups->birth, data->upsAdvIdent->upsAdvIdentDateOfManufacture,
                sizeof(ups->birth));
        strncpy(ups->serial, data->upsAdvIdent->upsAdvIdentSerialNumber,
                sizeof(ups->serial));
        free(data->upsAdvIdent);
    }

    powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
    if (data->upsBasicBattery) {
        strncpy(ups->battdat,
                data->upsBasicBattery->upsBasicBatteryLastReplaceDate,
                sizeof(ups->battdat));
        free(data->upsBasicBattery);
    }

    powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
    if (data->upsAdvBattery) {
        ups->extbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBattPacks;
        ups->badbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBadBattPacks;
        ups->nombattv = 0.0; /* PowerNet MIB doesn't give this value */
        free(data->upsAdvBattery);
    }

    powernet_mib_mgr_get_upsAdvConfig(s, &(data->upsAdvConfig));
    if (data->upsAdvConfig) {
        ups->NomOutputVoltage =
            data->upsAdvConfig->__upsAdvConfigRatedOutputVoltage;
        ups->hitrans = data->upsAdvConfig->__upsAdvConfigHighTransferVolt;
        ups->lotrans = data->upsAdvConfig->__upsAdvConfigLowTransferVolt;
        switch(data->upsAdvConfig->__upsAdvConfigAlarm) {
            case 1:
                if (data->upsAdvConfig->__upsAdvConfigAlarmTimer/100 < 30) {
                    strncpy(ups->beepstate, "0 Seconds",
                            sizeof(ups->beepstate));
                } else {
                    strncpy(ups->beepstate, "Timed",
                            sizeof(ups->beepstate));
                }
                break;
            case 2:
                strncpy(ups->beepstate, "LowBatt",
                            sizeof(ups->beepstate));
                break;
            case 3:
                strncpy(ups->beepstate, "NoAlarm",
                            sizeof(ups->beepstate));
                break;
            default:
                strncpy(ups->beepstate, "Timed",
                            sizeof(ups->beepstate));
                break;
        }
        ups->rtnpct = data->upsAdvConfig->__upsAdvConfigMinReturnCapacity;
        switch (data->upsAdvConfig->__upsAdvConfigSensitivity) {
            case 1:
                strncpy(ups->sensitivity, "auto", sizeof(ups->sensitivity));
                break;
            case 2:
                strncpy(ups->sensitivity, "low", sizeof(ups->sensitivity));
                break;
            case 3:
                strncpy(ups->sensitivity, "medium", sizeof(ups->sensitivity));
                break;
            case 4:
                strncpy(ups->sensitivity, "high", sizeof(ups->sensitivity));
                break;
            default:
                strncpy(ups->sensitivity, "unknown", sizeof(ups->sensitivity));
                break;
        }
        /* Data in Timeticks (1/100th sec). */
        ups->dlowbatt =
            data->upsAdvConfig->__upsAdvConfigLowBatteryRunTime/6000;
        ups->dwake = data->upsAdvConfig->__upsAdvConfigReturnDelay/100;
        ups->dshutd = data->upsAdvConfig->__upsAdvConfigShutoffDelay/100;
        free(data->upsAdvConfig);
    }

    powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
    if (data->upsAdvTest) {
        switch(data->upsAdvTest->__upsAdvTestDiagnosticSchedule) {
            case 1:
                strncpy(ups->selftest, "unknown", sizeof(ups->selftest));
                break;
            case 2:
                strncpy(ups->selftest, "biweekly", sizeof(ups->selftest));
                break;
            case 3:
                strncpy(ups->selftest, "weekly", sizeof(ups->selftest));
                break;
            case 4:
                strncpy(ups->selftest, "atTurnOn", sizeof(ups->selftest));
                break;
            case 5:
                strncpy(ups->selftest, "never", sizeof(ups->selftest));
                break;
            default:
                strncpy(ups->selftest, "unknown", sizeof(ups->selftest));
                break;
        }
        switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
            case 1:
                strncpy(ups->X, "OK", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test Ok",
                        sizeof(ups->selftestmsg));
                break;
            case 2:
                strncpy(ups->X, "BT", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test Failed",
                        sizeof(ups->selftestmsg));
                break;
            case 3:
                strncpy(ups->X, "BT", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Invalid Self Test",
                        sizeof(ups->selftestmsg));
                break;
            case 4:
                strncpy(ups->X, "NO", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test in Progress",
                        sizeof(ups->selftestmsg));
                break;
            default:
                strncpy(ups->X, "NO", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Unknown Result",
                        sizeof(ups->selftestmsg));
                break;
        }
    }
    free(data->upsAdvTest);
    return 1;
}

int powernet_snmp_ups_read_volatile_data(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    powernet_mib_t *data = Sid->MIB;

    if (powernet_check_comm_lost(ups) == 0) {
        return 0;
    }

    powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
    if (data->upsBasicBattery) {
        switch(data->upsBasicBattery->__upsBasicBatteryStatus) {
            case 2:
                UPS_CLEAR(UPS_BATTLOW);
                break;
            case 3:
                UPS_SET(UPS_BATTLOW);
                break;
            default: /* Unknown, assume battery is ok */
                UPS_CLEAR(UPS_BATTLOW);
                break;
        }
        free(data->upsBasicBattery);
    }
    
    powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
    if (data->upsAdvBattery) {
        ups->BattChg = data->upsAdvBattery->__upsAdvBatteryCapacity;
        ups->UPSTemp = data->upsAdvBattery->__upsAdvBatteryTemperature;
        ups->UPSTemp = data->upsAdvBattery->__upsAdvBatteryTemperature;
        ups->TimeLeft =
            data->upsAdvBattery->__upsAdvBatteryRunTimeRemaining/6000;
        if (data->upsAdvBattery->__upsAdvBatteryReplaceIndicator == 2) {
            UPS_SET(UPS_REPLACEBATT);
        } else {
            UPS_CLEAR(UPS_REPLACEBATT);
        }
        free(data->upsAdvBattery);
    }

    powernet_mib_mgr_get_upsBasicInput(s, &(data->upsBasicInput));
    if (data->upsBasicInput) {
        ups->InputPhase = data->upsBasicInput->__upsBasicInputPhase;
        free(data->upsBasicInput);
    }

    powernet_mib_mgr_get_upsAdvInput(s, &(data->upsAdvInput));
    if (data->upsAdvInput) {
        ups->LineVoltage = data->upsAdvInput->__upsAdvInputLineVoltage;
        ups->LineMax = data->upsAdvInput->__upsAdvInputMaxLineVoltage;
        ups->LineMin = data->upsAdvInput->__upsAdvInputMinLineVoltage;
        ups->LineFreq = data->upsAdvInput->__upsAdvInputFrequency;
        switch(data->upsAdvInput->__upsAdvInputLineFailCause) {
            case 1:
                strncpy(ups->G, "O-No Transfer", sizeof(ups->G));
                break;
            case 2:
                strncpy(ups->G, "High Line Voltage", sizeof(ups->G));
                break;
            case 3:
                strncpy(ups->G, "R-Brownout", sizeof(ups->G));
                break;
            case 4:
                strncpy(ups->G, "R-Blackout", sizeof(ups->G));
                break;
            case 5:
                strncpy(ups->G, "T-Small Sag", sizeof(ups->G));
                break;
            case 6:
                strncpy(ups->G, "T-Deep Sag", sizeof(ups->G));
                break;
            case 7:
                strncpy(ups->G, "T-Small Spike", sizeof(ups->G));
                break;
            case 8:
                strncpy(ups->G, "T-Deep Spike", sizeof(ups->G));
                break;
            case 9:
                strncpy(ups->G, "Self Test", sizeof(ups->G));
                break;
            case 10:
                strncpy(ups->G, "R-Rate of Volt Change", sizeof(ups->G));
                break;
            default:
                strncpy(ups->G, "Unknown", sizeof(ups->G));
                break;
        }
        free(data->upsAdvInput);
    }

    powernet_mib_mgr_get_upsBasicOutput(s, &(data->upsBasicOutput));
    if (data->upsBasicOutput) {
        /* Clear the following flags: only one status will be TRUE */
        Dmsg1(99, "Status before clearing: %d\n", ups->Status);
        UPS_CLEAR(UPS_ONLINE);
        UPS_CLEAR(UPS_ONBATT);
        UPS_CLEAR(UPS_SMARTBOOST);
        UPS_CLEAR(UPS_SMARTTRIM);
        Dmsg1(99, "Status after clearing: %d\n", ups->Status);
        switch(data->upsBasicOutput->__upsBasicOutputStatus) {
            case 2:
                UPS_SET(UPS_ONLINE);
                break;
            case 3:
                UPS_SET(UPS_ONBATT);
                break;
            case 4:
                UPS_SET(UPS_SMARTBOOST);
                break;
            case 12:
                UPS_SET(UPS_SMARTTRIM);
                break;
            case 1: /* unknown */
            case 5: /* timed sleeping */
            case 6: /* software bypass */
            case 7: /* UPS off */
            case 8: /* UPS rebooting */
            case 9: /* switched bypass */
            case 10: /* hardware failure bypass */
            case 11: /* sleeping until power returns */
            default: /* unknown */
                break;
        }
        ups->OutputPhase = data->upsBasicOutput->__upsBasicOutputPhase;
        free(data->upsBasicOutput);
    }

    powernet_mib_mgr_get_upsAdvOutput(s, &(data->upsAdvOutput));
    if (data->upsAdvOutput) {
        ups->OutputVoltage = data->upsAdvOutput->__upsAdvOutputVoltage;
        ups->OutputFreq = data->upsAdvOutput->__upsAdvOutputFrequency;
        ups->UPSLoad = data->upsAdvOutput->__upsAdvOutputLoad;
        ups->OutputCurrent = data->upsAdvOutput->__upsAdvOutputCurrent;
        free(data->upsAdvOutput);
    }

    powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
    if (data->upsAdvTest) {
        switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
            case 1:
                strncpy(ups->X, "OK", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test Ok",
                        sizeof(ups->selftestmsg));
                break;
            case 2:
                strncpy(ups->X, "BT", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test Failed",
                        sizeof(ups->selftestmsg));
                break;
            case 3:
                strncpy(ups->X, "BT", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Invalid Self Test",
                        sizeof(ups->selftestmsg));
                break;
            case 4:
                strncpy(ups->X, "NO", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Self Test in Progress",
                        sizeof(ups->selftestmsg));
                break;
            default:
                strncpy(ups->X, "NO", sizeof(ups->X));
                strncpy(ups->selftestmsg, "Unknown Result",
                        sizeof(ups->selftestmsg));
                break;
        }
        /* Not implemented. Needs transform date(mm/dd/yy)->hours. */
        // ups->LastSTTime = data->upsAdvTest->upsAdvTestLastDiagnosticsDate;
        if (data->upsAdvTest->__upsAdvTestCalibrationResults == 3) {
            UPS_SET(UPS_CALIBRATION);
        } else {
            UPS_CLEAR(UPS_CALIBRATION);
        }
        free(data->upsAdvTest);
    }

    return 1;
}

int powernet_snmp_ups_check_state(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    powernet_mib_t *data = Sid->MIB;

    if (powernet_check_comm_lost(ups) == 0) {
        return 0;
    }

    powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
    if (data->upsBasicBattery) {
        switch(data->upsBasicBattery->__upsBasicBatteryStatus) {
            case 2:
                UPS_CLEAR(UPS_BATTLOW);
                break;
            case 3:
                UPS_SET(UPS_BATTLOW);
                break;
            default: /* Unknown, assume battery is ok */
                UPS_CLEAR(UPS_BATTLOW);
                break;
        }
        free(data->upsBasicBattery);
    }
    
    powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
    if (data->upsAdvBattery) {
        if (data->upsAdvBattery->__upsAdvBatteryReplaceIndicator == 2) {
            UPS_SET(UPS_REPLACEBATT);
        } else {
            UPS_CLEAR(UPS_REPLACEBATT);
        }
        free(data->upsAdvBattery);
    }

    powernet_mib_mgr_get_upsBasicOutput(s, &(data->upsBasicOutput));
    if (data->upsBasicOutput) {
        /* Clear the following flags: only one status will be TRUE */
        Dmsg1(99, "Status before clearing: %d\n", ups->Status);
        UPS_CLEAR(UPS_ONLINE);
        UPS_CLEAR(UPS_ONBATT);
        UPS_CLEAR(UPS_SMARTBOOST);
        UPS_CLEAR(UPS_SMARTTRIM);
        Dmsg1(99, "Status after clearing: %d\n", ups->Status);
        switch(data->upsBasicOutput->__upsBasicOutputStatus) {
            case 2:
                UPS_SET(UPS_ONLINE);
                break;
            case 3:
                UPS_SET(UPS_ONBATT);
                break;
            case 4:
                UPS_SET(UPS_SMARTBOOST);
                break;
            case 12:
                UPS_SET(UPS_SMARTTRIM);
                break;
            case 1: /* unknown */
            case 5: /* timed sleeping */
            case 6: /* software bypass */
            case 7: /* UPS off */
            case 8: /* UPS rebooting */
            case 9: /* switched bypass */
            case 10: /* hardware failure bypass */
            case 11: /* sleeping until power returns */
            default: /* unknown */
                break;
        }
        free(data->upsBasicOutput);
    }

    powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
    if (data->upsAdvTest) {
        if (data->upsAdvTest->__upsAdvTestCalibrationResults == 3) {
            UPS_SET(UPS_CALIBRATION);
        } else {
            UPS_CLEAR(UPS_CALIBRATION);
        }
        free(data->upsAdvTest);
    }

    return 1;
}
