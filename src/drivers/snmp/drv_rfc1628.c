/*
 *  drv_rfc1628.c -- rfc1628 aka UPS-MIB driver
 *  Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c   -- Simple Daemon to catch power failure signals from a
 *                 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *              -- Now SmartMode support for SmartUPS and BackUPS Pro.
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

#include "apc.h"
#include "snmp.h"
#include "snmp_private.h"

static int rfc_1628_check_alarms(UPSINFO *ups) {
    struct snmp_ups_internal_data *Sid = (struct snmp_ups_internal_data *)ups->driver_internal_data;
    struct snmp_session *s = &Sid->session;
    ups_mib_t *data = (ups_mib_t *)Sid->MIB;

    /*
     * Check the Ethernet COMMLOST first, then check the
     * Agent/SNMP->UPS serial COMMLOST together with all the other
     * alarms.
     */
    if (ups_mib_mgr_get_upsAlarmEntry(s, &(data->upsAlarmEntry)) == -1) {
        set_ups(UPS_COMMLOST);
        free(data->upsAlarmEntry);
        return 0;
    } else {
        clear_ups(UPS_COMMLOST);
    }
    free(data->upsAlarmEntry);
    return 1;
}

int rfc1628_snmp_kill_ups_power(UPSINFO *ups) {
/*
 *  struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
 *  struct snmp_session *s = &Sid->session;
 *  ups_mib_t *data = Sid->MIB;
 */

    return 0;
}

int rfc1628_snmp_ups_get_capabilities(UPSINFO *ups) {
/*
 *  struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
 *  struct snmp_session *s = &Sid->session;
 *  ups_mib_t *data = Sid->MIB;
 */
    int i = 0;

    /*
     * Assume that an UPS with SNMP control has all the capabilities.
     * We know that the RFC1628 doesn't even implement some of the
     * capabilities. We do this way for sake of simplicity.
     */
    for (i = 0; i <= CI_MAX_CAPS; i++) {
        ups->UPS_Cap[i] = TRUE;
    }

    return 1;
}

int rfc1628_snmp_ups_read_static_data(UPSINFO *ups) {
/*
 *  struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
 *  struct snmp_session *s = &Sid->session;
 *  ups_mib_t *data = Sid->MIB;
 */

    rfc_1628_check_alarms(ups);
    return 1;
}

int rfc1628_snmp_ups_read_volatile_data(UPSINFO *ups) {
/*
 *  struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
 *  struct snmp_session *s = &Sid->session;
 *  ups_mib_t *data = Sid->MIB;
 */

    rfc_1628_check_alarms(ups);
    return 1;
}

int rfc1628_snmp_ups_check_state(UPSINFO *ups) {
/*
 *  struct snmp_ups_internal_data *Sid = ups->driver_internal_data;
 *  struct snmp_session *s = &Sid->session;
 *  ups_mib_t *data = Sid->MIB;
 */

    rfc_1628_check_alarms(ups);
    return 1;
}
