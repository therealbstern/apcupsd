/*
 * snmp.c
 *
 * SNMP UPS driver
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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
#include "snmp_private.h"

SnmpUpsDriver::SnmpUpsDriver(UPSINFO *ups) :
   UpsDriver(ups),
   _peername(NULL),
   _remote_port(0),
   _DeviceVendor(NULL),
   _community(NULL),
   _mib(NULL),
   _trap_session(NULL),
   _trap_received(false)
{
   memset(_device, 0, sizeof(_device));
   memset(&_session, 0, sizeof(_session));
}

int SnmpUpsDriver::initialize_device_data()
{
   char *port_num = NULL;
   char *cp;

   if (_ups->device == NULL || *_ups->device == '\0') {
      log_event(_ups, LOG_ERR, "Wrong device for SNMP driver.");
      exit(1);
   }

   strlcpy(_device, _ups->device, sizeof(_device));

   /*
    * Split the DEVICE statement and assign pointers to the various parts.
    * The DEVICE statement syntax in apcupsd.conf is:
    *
    *    DEVICE address:port:vendor:community
    *
    * vendor can be "APC" or "RFC".
    */

   _peername = _device;

   cp = strchr(_device, ':');
   if (cp == NULL) {
      log_event(_ups, LOG_ERR, "Wrong port for SNMP driver.");
      exit(1);
   }

   /*
    * Note that we purposely keep :port as part of peername. Newer
    * versions of Net-SNMP appear to ignore sess->remote_port and you
    * can only specify a port number by including it in the peername.
    * Older versions also appear to be able to cope with :port
    * appended to peername.
    */

   port_num = cp + 1;
   cp = strchr(port_num, ':');
   if (cp == NULL) {
      log_event(_ups, LOG_ERR, "Wrong vendor for SNMP driver.");
      exit(1);
   }
   *cp = '\0';
   _remote_port = atoi(port_num);

   _DeviceVendor = cp + 1;

   cp = strchr(_DeviceVendor, ':');
   if (cp == NULL) {
      log_event(_ups, LOG_ERR, "Wrong community for SNMP driver.");
      exit(1);
   }
   *cp = '\0';

   /*
    * Convert DeviceVendor to upper case.
    * Reuse cp and in the end of while() it will point to the end
    * of _DeviceVendor in anyway.
    */
   cp = _DeviceVendor;
   while (*cp != '\0') {
      *cp = toupper(*cp);
      cp++;
   }

   _community = cp + 1;

   return 1;
}

bool SnmpUpsDriver::Open()
{
   write_lock(_ups);

   initialize_device_data();

   write_unlock(_ups);

   memset(&_session, 0, sizeof(struct snmp_session));
   _session.peername = _peername;
   _session.remote_port = _remote_port;

   /*
    * We will use Version 1 of SNMP protocol as it is the most widely
    * used.
    */
   _session.version = SNMP_VERSION_1;
   _session.community = (u_char *)_community;
   _session.community_len = strlen((const char *)_session.community);

   /* Set a maximum of 5 retries before giving up. */
   _session.retries = 5;
   _session.timeout = SNMP_DEFAULT_TIMEOUT;
   _session.authenticator = NULL;

   if (!strcmp(_DeviceVendor, "APC") ||
       !strcmp(_DeviceVendor, "APC_NOTRAP")) {
      _mib = malloc(sizeof(powernet_mib_t));
      if (_mib == NULL) {
         log_event(_ups, LOG_ERR, "Out of memory.");
         exit(1);
      }

      memset(_mib, 0, sizeof(powernet_mib_t));

      /* Run powernet specific init */
      return powernet_snmp_ups_open();
   }

   if (!strcmp(_DeviceVendor, "RFC")) {
      _mib = malloc(sizeof(ups_mib_t));
      if (_mib == NULL) {
         log_event(_ups, LOG_ERR, "Out of memory.");
         exit(1);
      }

      memset(_mib, 0, sizeof(ups_mib_t));
      return 1;
   }

   /* No mib for this vendor. */
   Dmsg(0, "No MIB defined for vendor %s\n", _DeviceVendor);

   return 0;
}

bool SnmpUpsDriver::Close()
{
   return 1;
}

bool SnmpUpsDriver::get_capabilities()
{
   int ret = 0;

   write_lock(_ups);

   if (!strcmp(_DeviceVendor, "APC"))
      ret = powernet_snmp_ups_get_capabilities();

   if (!strcmp(_DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_get_capabilities();

   write_unlock(_ups);

   return ret;
}

bool SnmpUpsDriver::kill_power()
{
   int ret = 0;

   if (!strcmp(_DeviceVendor, "APC"))
      ret = powernet_snmp_kill_ups_power();

   if (!strcmp(_DeviceVendor, "RFC"))
      ret = rfc1628_snmp_kill_ups_power();

   return ret;
}

bool SnmpUpsDriver::check_state()
{
   int ret = 0;

   if (!strcmp(_DeviceVendor, "APC"))
      ret = powernet_snmp_ups_check_state();

   if (!strcmp(_DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_check_state();

   return ret;
}

bool SnmpUpsDriver::read_volatile_data()
{
   int ret = 0;

   write_lock(_ups);

   _ups->poll_time = time(NULL);    /* save time stamp */
   if (!strcmp(_DeviceVendor, "APC"))
      ret = powernet_snmp_ups_read_volatile_data();

   if (!strcmp(_DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_read_volatile_data();

   write_unlock(_ups);

   return ret;
}

bool SnmpUpsDriver::read_static_data()
{
   int ret = 0;

   write_lock(_ups);

   if (!strcmp(_DeviceVendor, "APC"))
      ret = powernet_snmp_ups_read_static_data();

   if (!strcmp(_DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_read_static_data();

   write_unlock(_ups);

   return ret;
}
