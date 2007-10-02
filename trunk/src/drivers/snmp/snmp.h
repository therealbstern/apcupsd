/*
 * snmp.h
 *
 * Public header for the SNMP UPS driver
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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

#ifndef _SNMP_H
#define _SNMP_H

#ifdef HAVE_UCD_SNMP

# undef UCD_COMPATIBLE
# define UCD_COMPATIBLE 1

# include <ucd-snmp/asn1.h>
# include <ucd-snmp/snmp.h>
# include <ucd-snmp/snmp_api.h>
# include <ucd-snmp/snmp_client.h>

#else

# ifdef HAVE_NET_SNMP
#  include <net-snmp/net-snmp-config.h>
#  include <net-snmp/library/asn1.h>
#  include <net-snmp/library/snmp.h>
#  include <net-snmp/library/snmp_api.h>
#  include <net-snmp/library/snmp_client.h>
# endif

#endif

#include "powernet-mib.h"
#include "rfc1628-mib.h"
#include "drivers.h"
#include <sys/param.h>   /* for MIN() */

class SnmpDriver: public UpsDriver
{
public:

   SnmpDriver(UPSINFO *ups) : UpsDriver(ups, "snmp") {}
   virtual ~SnmpDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState();
   virtual bool Close();

   // We provide default do-nothing implementations
   // for these methods since not all drivers need them.
   virtual bool KillPower();

private:

   bool initialize_device_data();
   bool powernet_check_comm_lost();

   /* APC */
   bool powernet_snmp_ups_get_capabilities();
   bool powernet_snmp_ups_read_static_data();
   bool powernet_snmp_ups_read_volatile_data();
   bool powernet_snmp_ups_check_state();
   bool powernet_snmp_kill_ups_power();
   bool powernet_snmp_ups_open();
   bool rfc_1628_check_alarms();

   /* IETF */
   bool rfc1628_snmp_ups_get_capabilities();
   bool rfc1628_snmp_ups_read_static_data();
   bool rfc1628_snmp_ups_read_volatile_data();
   bool rfc1628_snmp_ups_check_state();
   bool rfc1628_snmp_kill_ups_power();

   static int powernet_snmp_callback(
      int operation, snmp_session *session, 
      int reqid, snmp_pdu *pdu, void *magic);

   struct snmp_session _session;        /* snmp session struct */
   char _device[MAXSTRING];             /* Copy of ups->device */
   char *_peername;                     /* hostname|IP of peer */
   unsigned short _remote_port;         /* Remote socket, usually 161 */
   char *_DeviceVendor;                 /* Vendor (ex. APC|RFC) */
   char *_community;                    /* Community name */
   void *_MIB;                          /* Pointer to MIB data */
   struct snmp_session *_trap_session;  /* snmp session for traps */
   bool _trap_received;                 /* Have we seen a trap? */
};


/*
 * Copy a string from the SNMP library structure into the UPSINFO structure.
 * Structure member names are formed by simple patterns, so allow the caller
 * to specify nice readable names and build the ugly ones ourself. Source
 * strings are NOT nul-terminated, so let astrncpy terminate them for us.
 */
#define SNMP_STRING(oid, field, dest) \
   do \
   {  \
      astrncpy(_ups->dest, \
         (const char *)data->oid->oid##field, \
         MIN(sizeof(_ups->dest), data->oid->_##oid##field##Length+1)); \
   }  \
   while(0)

#endif   /* _SNMP_H */
