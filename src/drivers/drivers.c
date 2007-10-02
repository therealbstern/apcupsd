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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
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

#ifdef HAVE_SNMP_DRIVER
# include "snmp/snmp.h"
#endif

#ifdef HAVE_TEST_DRIVER
# include "test/testdriver.h"
#endif

#ifdef HAVE_PCNET_DRIVER
# include "pcnet/pcnet.h"
#endif

void attach_driver(UPSINFO *ups)
{
   char *driver_name = NULL;

   write_lock(ups);

   /* Attach the correct driver. */
   switch (ups->mode.type) {

   case BK:
   case SHAREBASIC:
   case DUMB_UPS:
      driver_name = "dumb";
#ifdef HAVE_DUMB_DRIVER
      ups->driver = new DumbDriver(ups);
#endif
      break;

   case BKPRO:
   case VS:
   case NBKPRO:
   case SMART:
   case MATRIX:
   case SHARESMART:
   case APCSMART_UPS:
      driver_name = "apcsmart";
#ifdef HAVE_APCSMART_DRIVER
      ups->driver = new ApcSmartDriver(ups);
#endif
      break;

   case USB_UPS:
      driver_name = "usb";
#if defined(HAVE_LINUX_USB)
      ups->driver = new LinuxUsbDriver(ups);
#elif defined(HAVE_GENERIC_USB)
      ups->driver = new GenericUsbDriver(ups);
#elif defined(HAVE_BSD_USB)
      ups->driver = new BsdUsbDriver(ups);
#endif

      break;

   case SNMP_UPS:
      driver_name = "snmp";
#ifdef HAVE_SNMP_DRIVER
      ups->driver = new SnmpDriver(ups);
#endif
      break;

   case TEST_UPS:
      driver_name = "test";
#ifdef HAVE_TEST_DRIVER
      ups->driver = new TestDriver(ups);
#endif
      break;

   case NETWORK_UPS:
      driver_name = "net";
#ifdef HAVE_NET_DRIVER
      ups->driver = new NetDriver(ups);
#endif
      break;

   case PCNET_UPS:
      driver_name = "pcnet";
#ifdef HAVE_PCNET_DRIVER
      ups->driver = new PcnetDriver(ups);
#endif
      break;

   default:
   case NO_UPS:
      driver_name = "[unknown]";
      Dmsg1(000, "Warning: no UPS driver found (ups->mode.type=%d).\n",
         ups->mode.type);
      break;
   }

   if (!ups->driver) {
      printf("\nApcupsd driver %s not found.\n"
             "The available apcupsd drivers are:\n", driver_name);

#ifdef HAVE_DUMB_DRIVER
      printf("dumb\n");
#endif
#ifdef HAVE_APCSMART_DRIVER
      printf("apcsmart\n");
#endif
#ifdef HAVE_USB_DRIVER
      printf("usb\n");
#endif
#ifdef HAVE_SNMP_DRIVER
      printf("snmp\n");
#endif
#ifdef HAVE_TEST_DRIVER
      printf("test\n");
#endif
#ifdef HAVE_NET_DRIVER
      printf("net\n");
#endif
#ifdef HAVE_PCNET_DRIVER
      printf("pcnet\n");
#endif

      printf("\n");
      printf("Most likely, you need to add --enable-%s "
             "to your ./configure options.\n\n", driver_name);
   }

   write_unlock(ups);
}
