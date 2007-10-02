/*
 * drv_rfc1628.c
 *
 * rfc1628 aka UPS-MIB driver
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

bool SnmpDriver::rfc_1628_check_alarms()
{
   struct snmp_session *s = &_session;
   ups_mib_t *data = (ups_mib_t *)_MIB;

   /*
    * Check the Ethernet COMMLOST first, then check the
    * Agent/SNMP->UPS serial COMMLOST together with all the other
    * alarms.
    */
   if (ups_mib_mgr_get_upsAlarmEntry(s, &(data->upsAlarmEntry)) == -1) {
      _ups->set_commlost();
      free(data->upsAlarmEntry);
      return false;
   } else {
      _ups->clear_commlost();
   }

   free(data->upsAlarmEntry);
   return true;
}

bool SnmpDriver::rfc1628_snmp_kill_ups_power()
{
   return false;
}

bool SnmpDriver::rfc1628_snmp_ups_get_capabilities()
{
   /*
    * Assume that an UPS with SNMP control has all the capabilities.
    * We know that the RFC1628 doesn't even implement some of the
    * capabilities. We do this way for sake of simplicity.
    */
   for (int i = 0; i <= CI_MAX_CAPS; i++)
      _ups->UPS_Cap[i] = TRUE;

   return true;
}

bool SnmpDriver::rfc1628_snmp_ups_read_static_data()
{
   struct snmp_session *s = &_session;
   ups_mib_t *data = (ups_mib_t *)_MIB;
   
   if (rfc_1628_check_alarms() == 0)
     return false;

   data->upsIdent = NULL;
   ups_mib_mgr_get_upsIdent(s, &(data->upsIdent));
   if (data->upsIdent) {
      SNMP_STRING(upsIdent, Model, upsmodel);
      SNMP_STRING(upsIdent, Name, upsname);
      free(data->upsIdent);
   }

   return true;
}

bool SnmpDriver::rfc1628_snmp_ups_read_volatile_data()
{  
   struct snmp_session *s = &_session;
   ups_mib_t *data = (ups_mib_t *)_MIB;

   if (rfc_1628_check_alarms() == 0)
     return false;

   data->upsBattery = NULL;
   ups_mib_mgr_get_upsBattery(s, &(data->upsBattery));
   if (data->upsBattery) {
      switch (data->upsBattery->__upsBatteryStatus) {
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

      _ups->BattChg = data->upsBattery->__upsEstimatedChargeRemaining;
      _ups->UPSTemp = data->upsBattery->__upsBatteryTemperature;
      _ups->TimeLeft = data->upsBattery->__upsEstimatedMinutesRemaining;

      free(data->upsBattery);
   }

   data->upsInputEntry = NULL;
   ups_mib_mgr_get_upsInputEntry(s, &(data->upsInputEntry));
   if (data->upsInputEntry) {
      _ups->LineVoltage = data->upsInputEntry->__upsInputVoltage;
      _ups->LineFreq    = data->upsInputEntry->__upsInputFrequency / 10;

      if (_ups->LineMax < _ups->LineVoltage) {
         _ups->LineMax = _ups->LineVoltage;
      }

      if (_ups->LineMin > _ups->LineVoltage || _ups->LineMin == 0) {
         _ups->LineMin = _ups->LineVoltage;
      }

      free(data->upsInputEntry);
   }

   data->upsOutputEntry = NULL;
   ups_mib_mgr_get_upsOutputEntry(s, &(data->upsOutputEntry));
   if (data->upsOutputEntry) {
      _ups->OutputVoltage = data->upsOutputEntry->__upsOutputVoltage;
      _ups->UPSLoad 	 = data->upsOutputEntry->__upsOutputPercentLoad;
      _ups->OutputCurrent = data->upsOutputEntry->__upsOutputCurrent;

      free(data->upsOutputEntry);
   }

   return true;
}

bool SnmpDriver::rfc1628_snmp_ups_check_state()
{
   /* Wait the required amount of time before bugging the device. */
   sleep(_ups->wait_time);

   write_lock(_ups);
   rfc_1628_check_alarms();
   write_unlock(_ups);

   return true;
}
