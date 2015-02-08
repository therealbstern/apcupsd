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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#include "apc.h"
#include "snmplite.h"
#include "snmplite-common.h"
#include "snmp.h"
#include "mibs.h"

SnmpLiteUpsDriver::SnmpLiteUpsDriver(UPSINFO *ups) :
   UpsDriver(ups),
   _host(NULL),
   _port(0),
   _vendor(NULL),
   _community(NULL),
   _snmp(NULL),
   _error_count(0),
   _commlost_time(0),
   _strategy(NULL),
   _traps(false)
{
   memset(_device, 0, sizeof(_device));
}

const char *SnmpLiteUpsDriver::snmplite_probe_community()
{
   const int sysDescrOid[] = {1, 3, 6, 1, 2, 1, 1, 1, -1};
   const char *communityList[] = { "private", "public", NULL };

   Dmsg(80, "Performing community autodetection\n");

   for (unsigned int i = 0; communityList[i]; i++)
   {
      Dmsg(80, "Probing community: \"%s\"\n", communityList[i]);

      _snmp->SetCommunity(communityList[i]);
      Snmp::Variable result;
      result.type = Asn::OCTETSTRING;
      if (_snmp->Get(sysDescrOid, &result) && result.valid)
         return communityList[i];
   }

   return NULL;
}

const MibStrategy *SnmpLiteUpsDriver::snmplite_probe_mib()
{
   Dmsg(80, "Performing MIB autodetection\n");

   // Every MIB strategy should have a CI_STATUS mapping in its OID map.
   // The generic probe method is simply to query for this OID and assume
   // we have found a supported MIB if the query succeeds.
   for (unsigned int i = 0; MibStrategies[i]; i++)
   {
      Dmsg(80, "Probing MIB: \"%s\"\n", MibStrategies[i]->name);
      for (unsigned int j = 0; MibStrategies[i]->mib[j].ci != -1; j++)
      {
         if (MibStrategies[i]->mib[j].ci == CI_STATUS)
         {
            Snmp::Variable result;
            result.type = MibStrategies[i]->mib[j].type;
            if (_snmp->Get(MibStrategies[i]->mib[j].oid, &result) && 
                result.valid)
               return MibStrategies[i];
         }
      }
   }

   return NULL;
}

bool SnmpLiteUpsDriver::Open()
{
   _port = 161;
   _community = NULL; // autodetect
   _vendor = NULL; // autodetect
   _traps = true;

   if (*_ups->device == '\0') {
      log_event(_ups, LOG_ERR, "snmplite Missing hostname");
      exit(1);
   }

   strlcpy(_device, _ups->device, sizeof(_device));

   /*
    * Split the DEVICE statement and assign pointers to the various parts.
    * The DEVICE statement syntax in apcupsd.conf is:
    *
    *    DEVICE address[:port[:vendor[:community]]]
    *
    * vendor can be "APC", "RFC", "MGE" or "*_NOTRAP".
    */

   char *cp = _device;
   _host = _device;
   cp = strchr(cp, ':');
   if (cp)
   {
      *cp++ = '\0';
      if (*cp != ':')
      {
         _port = atoi(cp);
         if (_port == 0)
         {
            log_event(_ups, LOG_ERR, "snmplite Bad port number");
            exit(1);
         }
      }

      cp = strchr(cp, ':');
      if (cp)
      {
         *cp++ = '\0';
         if (*cp != ':')
            _vendor = cp;

         cp = strchr(cp, ':');
         if (cp)
         {
            *cp++ = '\0';
            if (*cp)
               _community = cp;
         }
      }
   }

   // If user supplied a vendor, check for and remove "NOTRAP" and
   // optional underscore. Underscore is optional to allow use of vendor
   // "NOTRAP" to get autodetect with trap catching disabled.
   if (_vendor)
   {
      char *ptr = strstr((char*)_vendor, "NOTRAP");
      if (ptr)
      {
         // Trap catching is disabled
         _traps = false;

         // Remove "NOTRAP" from vendor string
         *ptr-- = '\0';

         // Remove optional underscore
         if (ptr >= _vendor && *ptr == '_')
            *ptr = '\0';

         // If nothing left, kill vendor to enable autodetect
         if (*_vendor == '\0')
            _vendor = NULL;
      }
   }

   Dmsg(80, "Trap catching: %sabled\n", _traps ? "En" : "Dis");

   // Create SNMP engine
   _snmp = new Snmp::SnmpEngine();
   if (!_snmp->Open(_host, _port, "dummy"))
   {
      log_event(_ups, LOG_ERR, "snmplite Unable to initialize SNMP");
      exit(1);
   }

   // Enable trap catching if user requested it
   if (_traps && !_snmp->EnableTraps())
   {
      // Failure to enable trap catching is not fatal. Probably just means
      // SNMP trap port is already in use by snmptrapd or another instance
      // of apcupsd. Warn user and continue.
      log_event(_ups, LOG_WARNING, "snmplite Failed to enable traps");
      _traps = false;
   }

   // If user did not specify a community, probe for one
   if (!_community)
   {
      _community = snmplite_probe_community();
      if (!_community)
      {
         log_event(_ups, LOG_ERR, "snmplite Unable to detect community");
         exit(1);
      }
   }

   _snmp->SetCommunity(_community);
   Dmsg(80, "Selected community: \"%s\"\n", _community);

   // If user supplied a vendor, search for a matching MIB strategy,
   // otherwise attempt to autodetect
   if (_vendor)
   {
      for (unsigned int i = 0; MibStrategies[i]; i++)
      {
         if (strcasecmp(MibStrategies[i]->name, _vendor) == 0)
         {
            _strategy = MibStrategies[i];
            break;
         }
      }
   }
   else
   {
      _strategy = snmplite_probe_mib();
   }

   if (!_strategy)
   {
      log_event(_ups, LOG_ERR, "snmplite Invalid vendor or unsupported MIB");
      exit(1);
   }

   Dmsg(80, "Selected MIB: \"%s\"\n", _strategy->name);

   return true;
}

bool SnmpLiteUpsDriver::Close()
{
   write_lock(_ups);
   _snmp->Close();
   delete _snmp;
   write_unlock(_ups);
   return true;
}

bool snmplite_ups_check_ci(int ci, Snmp::Variable &data)
{
   // Sanity check a few values that SNMP UPSes claim to report but seem
   // to always come back as zeros.
   switch (ci)
   {
   // SmartUPS 1000 is returning 0 or -1 for this via SNMP so screen it out
   // in case this is a common issue. Expected values would be 12/24/48 but
   // allow up to 120 for a really large UPS.
   case CI_NOMBATTV:
      return data.i32 > 0 && data.i32 <= 120;

   // SmartUPS 1000 is returning -1 for this.
   case CI_BADBATTS:
      return data.i32 > -1;

   // Generex CS121 SNMP/WEB Adapter using RFC1628 MIB is returning zero for
   // these values on a Newave Conceptpower DPA UPS.
   case CI_LTRANS:
   case CI_HTRANS:
   case CI_NOMOUTV:
   case CI_NOMINV:
   case CI_NOMPOWER:
      return data.i32 > 0;
   }

   return true;
}

bool SnmpLiteUpsDriver::get_capabilities()
{
   write_lock(_ups);

   // Walk the OID map, issuing an SNMP query for each item, one at a time.
   // If the query succeeds, sanity check the returned value and set the
   // capabilities flag.
   CiOidMap *mib = _strategy->mib;
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      Snmp::Variable data;
      data.type = mib[i].type;

      _ups->UPS_Cap[mib[i].ci] =
         _snmp->Get(mib[i].oid, &data) &&
         data.valid && snmplite_ups_check_ci(mib[i].ci, data);
   }

   write_unlock(_ups);

   // Succeed if we found CI_STATUS
   return _ups->UPS_Cap[CI_STATUS];
}

bool SnmpLiteUpsDriver::kill_power()
{
   if (_strategy->killpower_func)
      return _strategy->killpower_func(_snmp);

   return false;
}

bool SnmpLiteUpsDriver::shutdown()
{
   if (_strategy->shutdown_func)
      return _strategy->shutdown_func(_snmp);

   return false;
}

bool SnmpLiteUpsDriver::check_state()
{
   if (_traps)
   {
      // Simple trap handling: Any valid trap causes us to return and thus
      // new data will be fetched from the UPS.
      Snmp::TrapMessage *trap = _snmp->TrapWait(_ups->wait_time * 1000);
      if (trap)
      {
         Dmsg(80, "Got TRAP: generic=%d, specific=%d\n", 
            trap->Generic(), trap->Specific());
         delete trap;
      }
   }
   else
      sleep(_ups->wait_time);

   return true;
}

bool SnmpLiteUpsDriver::update_cis(bool dynamic)
{
   CiOidMap *mib = _strategy->mib;

   // Walk OID map and build a query for all parameters we have that
   // match the requested 'dynamic' setting
   Snmp::SnmpEngine::OidVar oidvar;
   alist<Snmp::SnmpEngine::OidVar> oids;
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      if (_ups->UPS_Cap[mib[i].ci] && mib[i].dynamic == dynamic)
      {
         oidvar.oid = mib[i].oid;
         oidvar.data.type = mib[i].type;
         oids.append(oidvar);
      }
   }

   // Issue the query, bail if it fails
   if (!_snmp->Get(oids))
      return false;

   // Walk the OID map again to correlate results with CIs and invoke the 
   // update function to set the values.
   alist<Snmp::SnmpEngine::OidVar>::iterator iter = oids.begin();
   for (unsigned int i = 0; mib[i].ci != -1; i++)
   {
      if (_ups->UPS_Cap[mib[i].ci] && 
          mib[i].dynamic == dynamic &&
          (*iter).data.valid &&
          ((*iter).data.type != Asn::SEQUENCE || // Skip update if sequence
           (*iter).data.seq.size() != 0))        // is empty
      {
         _strategy->update_ci_func(_ups, mib[i].ci, (*iter).data);
         ++iter;
      }
   }

   return true;
}

bool SnmpLiteUpsDriver::read_volatile_data()
{
   write_lock(_ups);

   int ret = update_cis(true);

   time_t now = time(NULL);
   if (ret)
   {
      // Successful query
      _error_count = 0;
      _ups->poll_time = now;    /* save time stamp */

      // If we were commlost, we're not any more
      if (_ups->is_commlost())
      {
         _ups->clear_commlost();
         generate_event(_ups, CMDCOMMOK);
      }
   }
   else
   {
      // Query failed. Close and reopen SNMP to help recover.
      _snmp->Close();
      _snmp->Open(_host, _port, _community);
      if (_traps)
         _snmp->EnableTraps();

      if (_ups->is_commlost())
      {
         // We already know we're commlost.
         // Log an event every 10 minutes.
         if ((now - _commlost_time) >= 10*60)
         {
            _commlost_time = now;
            log_event(_ups, event_msg[CMDCOMMFAILURE].level,
               event_msg[CMDCOMMFAILURE].msg);            
         }
      }
      else
      {
         // Check to see if we've hit enough errors to declare commlost.
         // If we have, set commlost flag and log an event.
         if (++_error_count >= 3)
         {
            _commlost_time = now;
            _ups->set_commlost();
            generate_event(_ups, CMDCOMMFAILURE);
         }
      }
   }

   write_unlock(_ups);
   return ret;
}

bool SnmpLiteUpsDriver::read_static_data()
{
   write_lock(_ups);
   int ret = update_cis(false);
   write_unlock(_ups);
   return ret;
}

bool SnmpLiteUpsDriver::entry_point(int command, void *data)
{
   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg(80, "Checking self test.\n");
      /* Reason for last transfer to batteries */
      if (_ups->UPS_Cap[CI_WHY_BATT] && update_cis(true))
      {
         Dmsg(80, "Transfer reason: %d\n", _ups->lastxfer);

         /* See if this is a self test rather than power failure */
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
   default:
      return FAILURE;
   }

   return SUCCESS;
}
