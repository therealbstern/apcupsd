/*
 * drivers.c
 *
 * UPS drivers middle (link) layer.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#include "apc.h"

#ifdef HAVE_DUMB_DRIVER
# include "dumb/dumb.h"
#endif

#ifdef HAVE_APCSMART_DRIVER
# include "apcsmart/apcsmart.h"
#endif

#ifdef HAVE_NET_DRIVER
# include "net/net.h"
#endif

#ifdef HAVE_USB_DRIVER
# include "usb/usb.h"
#endif

#ifdef HAVE_SNMPLITE_DRIVER
# include "snmplite/snmplite.h"
#endif

#ifdef HAVE_TEST_DRIVER
# include "test/testdriver.h"
#endif

#ifdef HAVE_PCNET_DRIVER
# include "pcnet/pcnet.h"
#endif

#ifdef HAVE_MODBUS_DRIVER
# include "modbus/modbus.h"
#endif

static const UPSDRIVER drivers[] = {
#ifdef HAVE_DUMB_DRIVER
   { "dumb",      DumbUpsDriver::Factory },
#endif   /* HAVE_DUMB_DRIVER */

#ifdef HAVE_APCSMART_DRIVER
   { "apcsmart",  ApcSmartUpsDriver::Factory },
#endif   /* HAVE_APCSMART_DRIVER */

#ifdef HAVE_NET_DRIVER
   { "net",       NetUpsDriver::Factory },
#endif   /* HAVE_NET_DRIVER */

#ifdef HAVE_USB_DRIVER
   { "usb",       UsbUpsDriver::Factory },
#endif   /* HAVE_USB_DRIVER */

#ifdef HAVE_SNMPLITE_DRIVER
   { "snmplite",  SnmpLiteUpsDriver::Factory },
#endif   /* HAVE_SNMPLITE_DRIVER */

#ifdef HAVE_TEST_DRIVER
   { "test",      TestUpsDriver::Factory },
#endif   /* HAVE_TEST_DRIVER */

#ifdef HAVE_PCNET_DRIVER
   { "pcnet",     PcnetUpsDriver::Factory },
#endif   /* HAVE_PCNET_DRIVER */

#ifdef HAVE_MODBUS_DRIVER
   { "modbus",    ModbusUpsDriver::Factory },
#endif   /* HAVE_MODBUS_DRIVER */

   /*
    * The NULL driver: closes the drivers list.
    */
   { NULL,        NULL }
};

/*
 * This is the glue between UPSDRIVER and UPSINFO.
 * It returns an UPSDRIVER pointer that may be null if something
 * went wrong.
 */
static UpsDriver *helper_attach_driver(UPSINFO *ups, const char *drvname)
{
   int i;

   write_lock(ups);

   Dmsg(99, "Looking for driver: %s\n", drvname);
   ups->driver = NULL;

   for (i = 0; drivers[i].driver_name; i++) {
      Dmsg(99, "Driver %s is configured.\n", drivers[i].driver_name);
      if (strcasecmp(drivers[i].driver_name, drvname) == 0) {
         ups->driver = drivers[i].factory(ups);
         Dmsg(20, "Driver %s found and attached.\n", drivers[i].driver_name);
         break;
      }
   }

   if (!ups->driver) {
      printf("\nApcupsd driver %s not found.\n"
             "The available apcupsd drivers are:\n", drvname);

      for (i = 0; drivers[i].driver_name; i++)
         printf("%s\n", drivers[i].driver_name);

      printf("\n");
      printf("Most likely, you need to add --enable-%s "
             "to your ./configure options.\n\n", drvname);
   }

   write_unlock(ups);

   Dmsg(99, "Driver ptr=0x%x\n", ups->driver);
   Dmsg(10, "Attached to driver: %s\n", drivers[i].driver_name);
   return ups->driver;
}

UpsDriver *attach_driver(UPSINFO *ups)
{
   const char *driver_name = NULL;

   /* Attach the correct driver. */
   switch (ups->mode.type) {
   case DUMB_UPS:
      driver_name = "dumb";
      break;

   case APCSMART_UPS:
      driver_name = "apcsmart";
      break;

   case USB_UPS:
      driver_name = "usb";
      break;

   case SNMPLITE_UPS:
      driver_name = "snmplite";
      break;

   case TEST_UPS:
      driver_name = "test";
      break;

   case NETWORK_UPS:
      driver_name = "net";
      break;

   case PCNET_UPS:
      driver_name = "pcnet";
      break;

   case MODBUS_UPS:
      driver_name = "modbus";
      break;

   default:
   case NO_UPS:
      Dmsg(000, "Warning: no UPS driver found (ups->mode.type=%d).\n",
         ups->mode.type);
      break;
   }

   return driver_name ? helper_attach_driver(ups, driver_name) : NULL;
}
