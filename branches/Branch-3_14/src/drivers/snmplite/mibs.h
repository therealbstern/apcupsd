/*
 * apc-mib.h
 *
 * Public header for the APC MIB strategy
 */

/*
 * Copyright (C) 2010 Adam Kropelin
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

#ifndef __MIBS_H_
#define __MIBS_H_

#include "snmp.h"
#include "asn.h"

// Mapping from CI to SNMP OID and type
struct CiOidMap
{
   int ci;                 // CI
   int *oid;               // SNMP OID
   Asn::Identifier type;   // ASN type for this OID
   bool dynamic;           // True if dynamic parameter, false if static
};

// Associates a MIB with processing functions for that MIB
struct MibStrategy
{
   const char *name;
   CiOidMap *mib;
   void (*update_ci_func)(UPSINFO*, int, Snmp::Variable &);
   int (*killpower_func)(Snmp::SnmpEngine *snmp);
   int (*shutdown_func)(Snmp::SnmpEngine *snmp);
};

extern struct MibStrategy *MibStrategies[];

#endif
