/*
 * mibs.c
 *
 * MIBs for SNMP Lite driver
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "mibs.h"

extern struct MibStrategy ApcMibStrategy;
extern struct MibStrategy ApcNoTrapMibStrategy;
extern struct MibStrategy Rfc1628MibStrategy;
extern struct MibStrategy Rfc1628NoTrapMibStrategy;

struct MibStrategy *MibStrategies[] =
{
   &ApcMibStrategy,
   &ApcNoTrapMibStrategy,
   &Rfc1628MibStrategy,
   &Rfc1628NoTrapMibStrategy,
   NULL
};
