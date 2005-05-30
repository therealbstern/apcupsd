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
#  include <net-snmp/library/asn1.h>
#  include <net-snmp/library/snmp.h>
#  include <net-snmp/library/snmp_api.h>
#  include <net-snmp/library/snmp_client.h>
# endif

#endif

#include "powernet-mib.h"
#include "rfc1628-mib.h"

/*********************************************************************/
/* Internal structures                                               */
/*********************************************************************/

struct snmp_ups_internal_data {
   struct snmp_session session;    /* snmp session struct */
   char device[MAXSTRING];         /* Copy of ups->device */
   char *peername;                 /* hostname|IP of peer */
   unsigned short remote_port;     /* Remote socket, usually 161 */
   char *DeviceVendor;             /* Vendor (ex. APC|RFC) */
   char *community;                /* Community name */
   void *MIB;                      /* Pointer to MIB data */
};

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern int snmp_ups_get_capabilities(UPSINFO *ups);
extern int snmp_ups_read_volatile_data(UPSINFO *ups);
extern int snmp_ups_read_static_data(UPSINFO *ups);
extern int snmp_ups_kill_power(UPSINFO *ups);
extern int snmp_ups_check_state(UPSINFO *ups);
extern int snmp_ups_open(UPSINFO *ups);
extern int snmp_ups_close(UPSINFO *ups);
extern int snmp_ups_setup(UPSINFO *ups);
extern int snmp_ups_program_eeprom(UPSINFO *ups, int command, char *data);
extern int snmp_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _SNMP_H */
