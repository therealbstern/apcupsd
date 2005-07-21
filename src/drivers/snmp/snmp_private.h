/*
 *  snmp_private.h  -- private header file for this driver
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  All rights reserved.
 *
 */

/*
   Copyright (C) 2000-2004 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#ifndef _SNMP_PRIVATE_H
#define _SNMP_PRIVATE_H

/* APC */
extern int powernet_snmp_ups_get_capabilities(UPSINFO *ups);
extern int powernet_snmp_ups_read_static_data(UPSINFO *ups);
extern int powernet_snmp_ups_read_volatile_data(UPSINFO *ups);
extern int powernet_snmp_ups_check_state(UPSINFO *ups);
extern int powernet_snmp_kill_ups_power(UPSINFO *ups);

/* IETF */
extern int rfc1628_snmp_ups_get_capabilities(UPSINFO *ups);
extern int rfc1628_snmp_ups_read_static_data(UPSINFO *ups);
extern int rfc1628_snmp_ups_read_volatile_data(UPSINFO *ups);
extern int rfc1628_snmp_ups_check_state(UPSINFO *ups);
extern int rfc1628_snmp_kill_ups_power(UPSINFO *ups);

#endif /* _SNMP_PRIVATE_H */
