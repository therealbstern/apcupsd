/*
 *  snmp.c -- SNMP driver
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
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

#include "apc.h"
#include "snmp.h"

static int initialize_device_data(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    char *cp;

    strcpy(Sid->device, ups->device);

    /*
     * Split the DEVICE statement and assign pointers to the various parts.
     *
     * The DEVICE statement syntax in apcupsd.conf is:
     *
     * DEVICE address:vendor:community
     */

    Sid->peername = Sid->device;

    cp = strchr(Sid->device, ':');
    if (cp == NULL) {
        log_event(ups, LOG_ERR, "Wrong device for SNMP driver.");
	exit(1);
    }
    *cp = '\0';

    Sid->DeviceVendor = cp + 1;

    cp = strchr(Sid->DeviceVendor, ':');
    if (cp == NULL) {
        log_event(ups, LOG_ERR, "Wrong device for SNMP driver.");
	exit(1);
    }
    *cp = '\0';

    Sid->community = cp + 1;

    return 1;
}

int snmp_ups_open(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid;

    /*
     * Allocate the internal data structure and link to UPSINFO.
     */
    Sid = malloc(sizeof(struct snmp_ups_internal_data));
    if (Sid == NULL) {
        log_event(ups, LOG_ERR, "Out of memory.");
	exit(1);
    }
    memset(Sid, 0, sizeof(struct snmp_ups_internal_data));

    ups->driver_internal_data = Sid;

    initialize_device_data(ups);

    /*
     * Initialize the requested MIB file, that will be read by init_mib()
     * in the MIBFILE env variable (libsnmp reads it there).
     */
    if (!strcmp(Sid->DeviceVendor, "APC"))
        setenv("MIBFILE", SYSCONFDIR "/mibs/APC-MIB", 1);

    if (!strcmp(Sid->DeviceVendor, "RFC"))
        setenv("MIBFILE", SYSCONFDIR "/mibs/UPS-MIB", 1);

    /*
     * Read the MIB file.
     */
    init_mib();

    memset(&Sid->session, 0, sizeof(struct snmp_session));

    Sid->session.peername = Sid->peername;

    /*
     * We will use Version 1 of SNMP protocol as it is the most widely
     * used.
     */
    Sid->session.version = SNMP_VERSION_1;
    Sid->session.community = Sid->community;
    Sid->session.community_len = strlen(Sid->session.community);
    /*
     * Set a maximum of 5 retries before giving up.
     */
    Sid->session.retries = 5;
    Sid->session.timeout = SNMP_DEFAULT_TIMEOUT;
    Sid->session.authenticator = NULL;
    /*
     * Setup synchronous conversation.
     */
    snmp_synch_setup(&Sid->session);
    /*
     * Open the snmp connection.
     */
    Sid->sessptr = snmp_open(&Sid->session);
    if (Sid->sessptr == NULL) {
        log_event(ups, LOG_ERR, "Can not connect to remote SNMP device.");
	exit(1);
    }
    return 1;
}

int snmp_ups_close(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;

    snmp_close(Sid->sessptr);
    free(ups->driver_internal_data);
    return 1;
}

int snmp_ups_setup(UPSINFO *ups) {
    /*
     * No need to setup anything.
     */
    return 1;
}

int snmp_ups_get_capabilities(UPSINFO *ups)
{
    return 1;
}

int snmp_ups_program_eeprom(UPSINFO *ups)
{
    return 0;
}

int snmp_ups_kill_power(UPSINFO *ups) {
    return 0;
}

int snmp_ups_check_state(UPSINFO *ups) {
    return 1;
}

int snmp_ups_read_volatile_data(UPSINFO *ups)
{
    return 1;
}

int snmp_ups_read_static_data(UPSINFO *ups)
{
    struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
    char names[128];
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    oid thisOID[MAX_OID_LEN];
    size_t thisOID_len = MAX_OID_LEN;
    int status;

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    read_objid("system.sysDescr.0", thisOID, &thisOID_len);
    snmp_add_null_var(pdu, thisOID, thisOID_len);
    status = snmp_synch_response(Sid->sessptr, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
	if (response->variables->type == ASN_OCTET_STR)
	    log_event(ups, LOG_INFO,
                    "[%d] SNMP sysdescr: %s",
                    time(NULL),
                    response->variables->val.string);
    }
    /*
     * If we had responses from SNMP device, we must free the PDU.
     * Then we must close the snmp session and free the device_data.
     */
    if (response)
	snmp_free_pdu(response);
    return 1;
}

int snmp_ups_entry_point(UPSINFO *ups, int command, void *data)
{
    return 0;
}
