/*
 * snmp.c
 *
 * SNMP Lite UPS driver
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
#include "snmp.h"

extern struct CiOidMap CiOidMap[];

struct snmplite_ups_internal_data
{
   char device[MAXSTRING];             /* Copy of ups->device */
   char *host;                         /* hostname|IP of peer */
   unsigned short port;                /* Remote port, usually 161 */
   char *community;                    /* Community name */
   Snmp::SnmpEngine *snmp;
   int error_count;
   time_t commlost_time;
};

int snmplite_ups_open(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid;

   /* Allocate the internal data structure and link to UPSINFO. */
   sid = (struct snmplite_ups_internal_data *)
      malloc(sizeof(struct snmplite_ups_internal_data));
   if (sid == NULL) {
      log_event(ups, LOG_ERR, "Out of memory.");
      exit(1);
   }

   ups->driver_internal_data = sid;

   memset(sid, 0, sizeof(struct snmplite_ups_internal_data));
   sid->port = 161;
   sid->community = "private";
   
   if (ups->device == NULL || *ups->device == '\0') {
      log_event(ups, LOG_ERR, "snmplite Missing hostname");
      exit(1);
   }

   astrncpy(sid->device, ups->device, sizeof(sid->device));

   /*
    * Split the DEVICE statement and assign pointers to the various parts.
    * The DEVICE statement syntax in apcupsd.conf is:
    *
    *    DEVICE address:port:vendor:community
    *
    * vendor can be "APC" or "RFC".
    */

   char *cp = sid->host = sid->device;
   cp = strchr(cp, ':');
   if (cp)
   {
      *cp++ = '\0';
      sid->port = atoi(cp);
      if (sid->port == 0)
      {
         log_event(ups, LOG_ERR, "snmplite Bad port number");
         exit(1);
      }

      cp = strchr(cp, ':');
      if (cp)
      {
         *cp++ = '\0';
         cp = strchr(cp, ':');
         if (cp)
            sid->community = cp+1;
      }
   }

   sid->snmp = new Snmp::SnmpEngine();
   if (!sid->snmp->Open(sid->host, sid->port, sid->community))
      return 0;

   return 1;
}

int snmplite_ups_close(UPSINFO *ups)
{
   write_lock(ups);

   struct snmplite_ups_internal_data *sid = 
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   sid->snmp->Close();
   delete sid->snmp;
   free(sid);
   ups->driver_internal_data = NULL;
   write_unlock(ups);
   return 1;
}

int snmplite_ups_setup(UPSINFO *ups)
{
   return 1;
}

bool snmplite_ups_check_ci(int ci, Snmp::Variable &data)
{
   // Sanity check a few values that SNMP UPSes claim to report but seem
   // to always send zeros.
   switch (ci)
   {
   case CI_HUMID:
   case CI_ATEMP:
   case CI_NOMBATTV:
      return data.u32 != 0;
   }

   return true;
}

int snmplite_ups_get_capabilities(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   write_lock(ups);

   // Walk the OID map, issuing an SNMP query for each item, one at a time.
   // If the query suceeds, sanity check the returned value and set the
   // capabilities flag.
   for (unsigned int i = 0; CiOidMap[i].ci != -1; i++)
   {
      Snmp::Variable data;
      if (CiOidMap[i].oid && sid->snmp->Get(CiOidMap[i].oid, &data))
      {
         ups->UPS_Cap[CiOidMap[i].ci] =
            snmplite_ups_check_ci(CiOidMap[i].ci, data);
      }
   }

   write_unlock(ups);

   // Succeed if we found CI_STATUS
   return ups->UPS_Cap[CI_STATUS];
}

int snmplite_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   return 0;
}

int snmplite_ups_kill_power(UPSINFO *ups)
{
   // Implement me
   return 0;
}

int snmplite_ups_check_state(UPSINFO *ups)
{
   // SNMP trap catching goes here...
   sleep(ups->wait_time);
   return 1;
}

#define TIMETICKS_TO_SECS 100
#define SECS_TO_MINS      60

static void snmplite_ups_update_ci(UPSINFO *ups, int ci, Snmp::Variable &data)
{
   switch (ci)
   {
   case CI_VLINE:
      Dmsg1(80, "Got CI_VLINE: %d\n", data.u32);
      ups->LineVoltage = data.u32;
      break;

   case CI_VOUT:
      Dmsg1(80, "Got CI_VOUT: %d\n", data.u32);
      ups->OutputVoltage = data.u32;
      break;

   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %d\n", data.u32);
      ups->BattVoltage = data.u32;
      break;

   case CI_FREQ:
      Dmsg1(80, "Got CI_FREQ: %d\n", data.u32);
      ups->LineFreq = data.u32;
      break;

   case CI_LOAD:
      Dmsg1(80, "Got CI_LOAD: %d\n", data.u32);
      ups->UPSLoad = data.u32;
      break;

   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %d\n", data.u32);
      ups->UPSTemp = data.u32;
      break;

   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %d\n", data.u32);
      ups->ambtemp = data.u32;
      break;

   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %d\n", data.u32);
      ups->humidity = data.u32;
      break;

   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %d\n", data.u32);
      ups->nombattv = data.u32;
      break;

   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %d\n", data.u32);
      ups->NomOutputVoltage = data.u32;
      break;

   case CI_NOMINV:
      Dmsg1(80, "Got CI_NOMINV: %d\n", data.u32);
      ups->NomInputVoltage = data.u32;
      break;

   case CI_NOMPOWER:
      Dmsg1(80, "Got CI_NOMPOWER: %d\n", data.u32);
      ups->NomPower = data.u32;
      break;

   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %d\n", data.u32);
      ups->lotrans = data.u32;
      break;

   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %d\n", data.u32);
      ups->hitrans = data.u32;
      break;

   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %d\n", data.u32);
      ups->dwake = data.u32;
      break;

   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %d\n", data.u32);
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

   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:
         // ADK: Need to check alarm time here like old driver did
         astrncpy(ups->beepstate, "Timed", sizeof(ups->beepstate));
         break;
      case 2:
         astrncpy(ups->beepstate, "LowBatt", sizeof(ups->beepstate));
         break;
      case 3:
         astrncpy(ups->beepstate, "NoAlarm", sizeof(ups->beepstate));
         break;
      default:
         astrncpy(ups->beepstate, "Timed", sizeof(ups->beepstate));
         break;
      }
      break;

   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", data.str.str());
      astrncpy(ups->upsmodel, data.str, sizeof(ups->upsmodel));
      break;

   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", data.str.str());
      astrncpy(ups->serial, data.str, sizeof(ups->serial));
      break;

   case CI_MANDAT:
      Dmsg1(80, "Got CI_MANDAT: %s\n", data.str.str());
      astrncpy(ups->birth, data.str, sizeof(ups->birth));
      break;

   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %d\n", data.u32);
      ups->BattChg = data.u32;
      break;

   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %d\n", data.u32);
      ups->TimeLeft = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_BATTDAT:
      Dmsg1(80, "Got CI_BATTDAT: %s\n", data.str.str());
      astrncpy(ups->battdat, data.str, sizeof(ups->battdat));
      break;

   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", data.str.str());
      astrncpy(ups->upsname, data.str, sizeof(ups->upsname));
      break;

   case CI_STATUS:
      Dmsg1(80, "Got CI_STATUS: %d\n", data.u32);
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
      Dmsg1(80, "Got CI_NeedReplacement: %d\n", data.u32);
      if (data.u32 == 2)
         ups->set_replacebatt();
      else
         ups->clear_replacebatt();
      break;

   case CI_LowBattery:
      Dmsg1(80, "Got CI_LowBattery: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_battlow();
      else
         ups->clear_battlow();
      break;

   case CI_Calibration:
      Dmsg1(80, "Got CI_Calibration: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_calibration();
      else
         ups->clear_calibration();
      break;

   case CI_Overload:
      Dmsg1(80, "Got CI_Overload: %c\n", data.str[8]);
      if (data.str[8] == '1')
         ups->set_overload();
      else
         ups->clear_overload();
      break;

   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %d\n", data.u32);
      ups->dshutd = data.u32 / TIMETICKS_TO_SECS;
      break;

   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %d\n", data.u32);
      ups->rtnpct = data.u32;
      break;

   case CI_WHY_BATT:
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
      Dmsg1(80, "Got CI_SENS: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:
         astrncpy(ups->sensitivity, "Auto", sizeof(ups->sensitivity));
         break;
      case 2:
         astrncpy(ups->sensitivity, "Low", sizeof(ups->sensitivity));
         break;
      case 3:
         astrncpy(ups->sensitivity, "Medium", sizeof(ups->sensitivity));
         break;
      case 4:
         astrncpy(ups->sensitivity, "High", sizeof(ups->sensitivity));
         break;
      default:
         astrncpy(ups->sensitivity, "Unknown", sizeof(ups->sensitivity));
         break;
      }
      break;

   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", data.str.str());
      astrncpy(ups->firmrev, data.str, sizeof(ups->firmrev));
      break;

   case CI_EXTBATTS:
      Dmsg1(80, "Got CI_EXTBATTS: %d\n", data.u32);
      ups->extbatts = data.u32;
      break;
   
   case CI_BADBATTS:
      Dmsg1(80, "Got CI_BADBATTS: %d\n", data.u32);
      ups->badbatts = data.u32;
      break;

   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %d\n", data.u32);
      ups->dlowbatt = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %d\n", data.u32);
      switch (data.u32) {
      case 2:
         astrncpy(ups->selftest, "336", sizeof(ups->selftest));
         break;
      case 3:
         astrncpy(ups->selftest, "168", sizeof(ups->selftest));
         break;
      case 4:
         astrncpy(ups->selftest, "ON", sizeof(ups->selftest));
         break;
      case 1:
      case 5:
      default:
         astrncpy(ups->selftest, "OFF", sizeof(ups->selftest));
         break;
      }
      break;

   case CI_VMIN:
      Dmsg1(80, "Got CI_VMIN: %d\n", data.u32);
      ups->LineMin = data.u32;
      break;

   case CI_VMAX:
      Dmsg1(80, "Got CI_VMAX: %d\n", data.u32);
      ups->LineMax = data.u32;
      break;
   }
}

static int snmplite_ups_update_cis(UPSINFO *ups, bool dynamic)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   // Walk OID map and build a query for all parameters we have that
   // match the requested 'dynamic' setting
   Snmp::SnmpEngine::OidVar oidvar;
   alist<Snmp::SnmpEngine::OidVar> oids;
   for (unsigned int i = 0; CiOidMap[i].ci != -1; i++)
   {
      if (ups->UPS_Cap[CiOidMap[i].ci] && 
          CiOidMap[i].oid && CiOidMap[i].dynamic == dynamic)
      {
         oidvar.oid = CiOidMap[i].oid;
         oids.append(oidvar);
      }
   }

   // Issue the query, bail if it fails
   if (!sid->snmp->Get(oids))
      return 0;

   // Walk the OID map again to correlate results with CIs.
   // Invoke the update function to set the values.
   alist<Snmp::SnmpEngine::OidVar>::iterator iter = oids.begin();
   for (unsigned int i = 0; CiOidMap[i].ci != -1; i++)
   {
      if (ups->UPS_Cap[CiOidMap[i].ci] && 
          CiOidMap[i].oid && CiOidMap[i].dynamic == dynamic)
      {
         snmplite_ups_update_ci(ups, CiOidMap[i].ci, (*iter).data);
         ++iter;
      }
   }
   
   return 1;
}

int snmplite_ups_read_volatile_data(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   write_lock(ups);

   int ret = snmplite_ups_update_cis(ups, true);

   time_t now = time(NULL);
   if (ret)
   {
      // Successful query
      sid->error_count = 0;
      ups->poll_time = now;    /* save time stamp */

      // If we were commlost, we're not any more
      if (ups->is_commlost())
      {
         ups->clear_commlost();
         generate_event(ups, CMDCOMMOK);
      }
   }
   else
   {
      // Query failed. Close and reopen SNMP to help recover.
      sid->snmp->Close();
      sid->snmp->Open(sid->host, sid->port, sid->community);

      if (ups->is_commlost())
      {
         // We already know we're commlost.
         // Log an event every 10 minutes.
         if ((now - sid->commlost_time) >= 10*60)
         {
            sid->commlost_time = now;
            log_event(ups, event_msg[CMDCOMMFAILURE].level,
               event_msg[CMDCOMMFAILURE].msg);            
         }
      }
      else
      {
         // Check to see if we've hit enough errors to declare commlost.
         // If we have, set commlost flag and log an event.
         if (++sid->error_count >= 3)
         {
            sid->commlost_time = now;
            ups->set_commlost();
            generate_event(ups, CMDCOMMFAILURE);
         }
      }
   }

   write_unlock(ups);
   return ret;
}

int snmplite_ups_read_static_data(UPSINFO *ups)
{
   write_lock(ups);
   int ret = snmplite_ups_update_cis(ups, false);
   write_unlock(ups);
   return ret;
}

int snmplite_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   return 0;
}
