/*
 *  apcdrivers.c -- UPS drivers middle (link) layer.
 *
 *  apcupsd.c	     -- Simple Daemon to catch power failure signals from a
 *		     BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		  -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *  All rights reserved.
 *
 */

/*
 *			 GNU GENERAL PUBLIC LICENSE
 *			    Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			       675 Mass Ave, Cambridge, MA 02139, USA
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
#ifdef HAVE_DUMB_DRIVER
#include "dumb/dumb.h"
#endif
#ifdef HAVE_APCSMART_DRIVER
#include "apcsmart/apcsmart.h"
#endif
#ifdef HAVE_NET_DRIVER
#include "net/net.h"
#endif
#ifdef HAVE_USB_DRIVER
#include "usb/usb.h"
#endif
#ifdef HAVE_SNMP_DRIVER
#include "snmp/snmp.h"
#endif
#ifdef HAVE_TEST_DRIVER
#include "test/testdriver.h"
#endif

UPSDRIVER drivers[] = {
#ifdef HAVE_DUMB_DRIVER
    { "dumb",
      dumb_ups_open,
      dumb_ups_setup,
      dumb_ups_close,
      dumb_ups_kill_power,
      dumb_ups_read_static_data,
      dumb_ups_read_volatile_data,
      dumb_ups_get_capabilities,
      dumb_ups_read_volatile_data,
      dumb_ups_program_eeprom,
      dumb_ups_entry_point },
#endif /* HAVE_DUMB_DRIVER */

#ifdef HAVE_APCSMART_DRIVER
    { "apcsmart",
      apcsmart_ups_open,
      apcsmart_ups_setup,
      apcsmart_ups_close,
      apcsmart_ups_kill_power,
      apcsmart_ups_read_static_data,
      apcsmart_ups_read_volatile_data,
      apcsmart_ups_get_capabilities,
      apcsmart_ups_check_state,
      apcsmart_ups_program_eeprom,
      apcsmart_ups_entry_point },
#endif /* HAVE_APCSMART_DRIVER */

#ifdef HAVE_NET_DRIVER
    { "net",
      net_ups_open,
      net_ups_setup,
      net_ups_close,
      net_ups_kill_power,
      net_ups_read_static_data,
      net_ups_read_volatile_data,
      net_ups_get_capabilities,
      net_ups_check_state,
      net_ups_program_eeprom,
      net_ups_entry_point },
#endif /* HAVE_NET_DRIVER */

#ifdef HAVE_USB_DRIVER
    { "usb",
      usb_ups_open,
      usb_ups_setup,
      usb_ups_close,
      usb_ups_kill_power,
      usb_ups_read_static_data,
      usb_ups_read_volatile_data,
      usb_ups_get_capabilities,
      usb_ups_check_state,
      usb_ups_program_eeprom,
      usb_ups_entry_point },
#endif /* HAVE_USB_DRIVER */

#ifdef HAVE_SNMP_DRIVER
    { "snmp",
      snmp_ups_open,
      snmp_ups_setup,
      snmp_ups_close,
      snmp_ups_kill_power,
      snmp_ups_read_static_data,
      snmp_ups_read_volatile_data,
      snmp_ups_get_capabilities,
      snmp_ups_check_state,
      snmp_ups_program_eeprom,
      snmp_ups_entry_point },
#endif /* HAVE_SNMP_DRIVER */

#ifdef HAVE_TEST_DRIVER
    { "test",
      test_ups_open,
      test_ups_setup,
      test_ups_close,
      test_ups_kill_power,
      test_ups_read_static_data,
      test_ups_read_volatile_data,
      test_ups_get_capabilities,
      test_ups_check_state,
      test_ups_program_eeprom,
      test_ups_entry_point },
#endif /* HAVE_TEST_DRIVER */

    /*
     * The NULL driver: closes the drivers list.
     */
    { NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL }
};

/*
 * This is the glue between UPSDRIVER and UPSINFO.
 * It returns an UPSDRIVER pointer that may be null if something
 * went wrong.
 */
static UPSDRIVER *helper_attach_driver(UPSINFO *ups, char *drvname)
{
    int i;

    write_lock(ups);

    Dmsg1(99, "Looking for driver: %s\n", drvname);
    ups->driver = NULL;
    for (i=0; drivers[i].driver_name; i++) {
        Dmsg1(99, "Driver %s is configured.\n", drivers[i].driver_name);
	if (strcasecmp(drivers[i].driver_name, drvname) == 0) {
	    ups->driver = &drivers[i];
            Dmsg1(20, "Driver %s found and attached.\n", drivers[i].driver_name);
	    break;
	}
    }
    if (!ups->driver) {
        printf("\nApcupsd driver %s not found.\nThe available apcupsd drivers are:\n", drvname);
	for (i=0; drivers[i].driver_name; i++) {
            printf("%s\n", drivers[i].driver_name);
	}
        printf("\n");
        printf("Most likely, you need to add --enable-%s to your ./configure options.\n\n",
	   drvname);
    }

    write_unlock(ups);

    Dmsg1(99, "Driver ptr=0x%x\n", ups->driver);
    return ups->driver;
}

UPSDRIVER *attach_driver(UPSINFO *ups) {
    char *driver_name = NULL;
    /*
     * Attach the correct driver.
     */
    switch(ups->mode.type) {
    case BK:
    case SHAREBASIC:
    case NETUPS:
    case DUMB_UPS:
        driver_name = "dumb";
	break;

    case BKPRO:
    case VS:
    case NBKPRO:
    case SMART:
    case MATRIX:
    case SHARESMART:
    case APCSMART_UPS:
        driver_name = "apcsmart";
	break;

    case USB_UPS:
        driver_name = "usb";
	break;

    case SNMP_UPS:
        driver_name = "snmp";
	break;

    case TEST_UPS:
        driver_name = "test";
	break;

    case NETWORK_UPS:
        driver_name = "net";
	break;

    default:
    case NO_UPS:
        Dmsg1(000, "Warning: no UPS driver found (ups->mode.type=%d).\n",
		    ups->mode.type);
	break;
    }
    return driver_name ? helper_attach_driver(ups, driver_name) : NULL;
}
