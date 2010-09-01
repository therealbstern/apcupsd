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
#include "snmplite-common.h"
#include "snmp.h"
#include "mibs.h"

static bool snmplite_generic_probe(UPSINFO *ups, const MibStrategy *strategy)
{
   struct snmplite_ups_internal_data *sid = 
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   // Every MIB strategy should have a CI_STATUS mapping in its OID map.
   // The generic probe method is simply to query for this OID and assume
   // we have found a supported MIB if the query succeeds.
   CiOidMap *mib = strategy->mib;
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      if (mib[i].ci == CI_STATUS)
      {
         Snmp::Variable result;
         return sid->snmp->Get(mib[i].oid, &result);
      }
   }

   return false;
}

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
   sid->vendor = NULL; // autodetect
   sid->traps = true;

   if (ups->device == NULL || *ups->device == '\0') {
      log_event(ups, LOG_ERR, "snmplite Missing hostname");
      exit(1);
   }

   astrncpy(sid->device, ups->device, sizeof(sid->device));

   /*
    * Split the DEVICE statement and assign pointers to the various parts.
    * The DEVICE statement syntax in apcupsd.conf is:
    *
    *    DEVICE address[:port[:vendor[:community]]]
    *
    * vendor can be "APC", "RFC", "MGE" or "*_NOTRAP".
    */

   char *cp = sid->device;
   sid->host = sid->device;
   cp = strchr(cp, ':');
   if (cp)
   {
      *cp++ = '\0';
      if (*cp != ':')
      {
         sid->port = atoi(cp);
         if (sid->port == 0)
         {
            log_event(ups, LOG_ERR, "snmplite Bad port number");
            exit(1);
         }
      }

      cp = strchr(cp, ':');
      if (cp)
      {
         *cp++ = '\0';
         if (*cp != ':')
            sid->vendor = cp;

         cp = strchr(cp, ':');
         if (cp)
         {
            *cp++ = '\0';
            sid->community = cp;
         }
      }
   }

   // If user supplied a vendor, check for and remove "NOTRAP" and
   // optional underscore. Underscore is optional to allow use of vendor
   // "NOTRAP" to get autodetect with trap catching disabled.
   if (sid->vendor)
   {
      char *ptr = strstr((char*)sid->vendor, "NOTRAP");
      if (ptr)
      {
         // Trap catching is disabled
         sid->traps = false;

         // Remove "NOTRAP" from vendor string
         *ptr = '\0';

         // Remove optional underscore
         if (ptr > sid->vendor && *(ptr-1) == '_')
            *(ptr-1) = '\0';

         // If nothing left, kill vendor to enable autodetect
         if (*sid->vendor == '\0')
            sid->vendor = NULL;
      }
   }

   Dmsg1(80, "Trap catching: %sabled\n", sid->traps ? "En" : "Dis");

   // Create SNMP engine
   sid->snmp = new Snmp::SnmpEngine();
   if (!sid->snmp->Open(sid->host, sid->port, sid->community, sid->traps))
      return 0;

   // If user supplied a vendor, search for a matching MIB strategy,
   // otherwise attempt to autodetect
   if (sid->vendor)
   {
      for (unsigned int i = 0; MibStrategies[i]; i++)
      {
         if (strcmp(MibStrategies[i]->name, sid->vendor) == 0)
         {
            sid->strategy = MibStrategies[i];
            break;
         }
      }
   }
   else
   {
      Dmsg0(80, "Performing MIB autodetection\n");

      // For each strategy, run generic probe
      for (unsigned int i = 0; MibStrategies[i]; i++)
      {
         Dmsg1(80, "Probing MIB: \"%s\"\n", MibStrategies[i]->name);
         if (snmplite_generic_probe(ups, MibStrategies[i]))
         {
            sid->strategy = MibStrategies[i];
            sid->vendor = MibStrategies[i]->name;
            break;
         }
      }
   }

   if (!sid->strategy)
   {
      log_event(ups, LOG_ERR, "snmplite Invalid vendor or unsupported MIB");
      exit(1);
   }

   Dmsg1(80, "Selected MIB: \"%s\"\n", sid->strategy->name);

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
   // to always come back as zeros.
   switch (ci)
   {
   // SmartUPS 1000 is returning 0 for this via SNMP so screen it out
   // in case this is a common issue.
   case CI_NOMBATTV:
   // Generex CS121 SNMP/WEB Adapter using RFC1628 MIB is returning zero for
   // these values on a Newave Conceptpower DPA UPS.
   case CI_LTRANS:
   case CI_HTRANS:
   case CI_NOMOUTV:
   case CI_NOMINV:
   case CI_NOMPOWER:
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
   // If the query succeeds, sanity check the returned value and set the
   // capabilities flag.
   CiOidMap *mib = sid->strategy->mib;
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      Snmp::Variable data;
      if (sid->snmp->Get(mib[i].oid, &data))
      {
         ups->UPS_Cap[mib[i].ci] =
            snmplite_ups_check_ci(mib[i].ci, data);
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
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   if (sid->strategy->killpower_func)
      return sid->strategy->killpower_func(ups);

   return 0;
}

int snmplite_ups_shutdown(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   if (sid->strategy->shutdown_func)
      return sid->strategy->shutdown_func(ups);

   return 0;
}

int snmplite_ups_check_state(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   if (sid->traps)
   {
      // Simple trap handling: Any valid trap causes us to return and thus
      // new data will be fetched from the UPS.
      Snmp::TrapMessage *trap = sid->snmp->TrapWait(ups->wait_time * 1000);
      if (trap)
      {
         Dmsg2(80, "Got TRAP: generic=%d, specific=%d\n", 
            trap->Generic(), trap->Specific());
         delete trap;
      }
   }
   else
      sleep(ups->wait_time);

   return 1;
}

static int snmplite_ups_update_cis(UPSINFO *ups, bool dynamic)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;
   CiOidMap *mib = sid->strategy->mib;

   // Walk OID map and build a query for all parameters we have that
   // match the requested 'dynamic' setting
   Snmp::SnmpEngine::OidVar oidvar;
   alist<Snmp::SnmpEngine::OidVar> oids;
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      if (ups->UPS_Cap[mib[i].ci] && mib[i].dynamic == dynamic)
      {
         oidvar.oid = mib[i].oid;
         oids.append(oidvar);
      }
   }

   // Issue the query, bail if it fails
   if (!sid->snmp->Get(oids))
      return 0;

   // Walk the OID map again to correlate results with CIs. Check that type 
   // matches what we expect and invoke the update function to set the values.
   alist<Snmp::SnmpEngine::OidVar>::iterator iter = oids.begin();
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      if (ups->UPS_Cap[mib[i].ci] && 
          mib[i].dynamic == dynamic &&
          mib[i].type == (*iter).data.type)
      {
         sid->strategy->update_ci_func(ups, mib[i].ci, (*iter).data);
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
      sid->snmp->Open(sid->host, sid->port, sid->community, sid->traps);

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
