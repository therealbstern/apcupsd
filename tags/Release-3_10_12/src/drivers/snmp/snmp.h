/*
 *  snmp.h  -- public header file for this driver
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  All rights reserved.
 *
 */

/*
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *                           675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS "APCUPSD-Team" BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE "APCUPSD-Team" HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE "APCUPSD-Team" SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN "AS IS" BASIS, AND THE "APCUPSD-Team" HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE "APCUPSD-Team" HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, "APCC".  THE "APCUPSD-Team" DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH "APCC".  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */

#ifndef _SNMP_H
#define _SNMP_H

#ifndef UCD_COMPATIBLE
#define UCD_COMPATIBLE 1
#endif
#include <ucd-snmp/asn1.h>
#include <ucd-snmp/snmp.h>
#include <ucd-snmp/snmp_api.h>
#include <ucd-snmp/snmp_client.h>
#include "powernet-mib.h"
#include "rfc1628-mib.h"

/*********************************************************************/
/* Internal structures                                               */
/*********************************************************************/

struct snmp_ups_internal_data {
    struct snmp_session session;            /* snmp session struct */
    char device[MAXSTRING];                 /* Copy of ups->device */
    char *peername;                         /* hostname|IP of peer */
    unsigned short remote_port;             /* Remote socket, usually 161 */
    char *DeviceVendor;                     /* Vendor (ex. APC|RFC) */
    char *community;                        /* Community name */
    void *MIB;                              /* Pointer to MIB data */
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

#endif /* _SNMP_H */
