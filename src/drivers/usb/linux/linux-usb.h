/*
 * linux-usb.h
 *
 * Platform-specific interface to Linux hiddev USB HID driver.
 *
 * Parts of this code (especially the initialization and walking
 * the reports) were derived from a test program hid-ups.c by:    
 *    Vojtech Pavlik <vojtech@ucw.cz>
 *    Paul Stewart <hiddev@wetlogic.net>
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 2004-2012 Adam Kropelin
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

#ifndef _LINUXUSB_H
#define _LINUXUSB_H

#include "../usb.h"
#include <asm/types.h>
#include <linux/hiddev.h>

class LinuxUsbUpsDriver: public UsbUpsDriver
{
public:
   LinuxUsbUpsDriver(UPSINFO *ups);
   virtual ~LinuxUsbUpsDriver() {}

   // Inherited from UpsDriver
   virtual bool Open();
   virtual bool Close();
   virtual bool check_state();

   // Inherited from UsbUpsDriver
   virtual int write_int_to_ups(int ci, int value, char const* name);
   virtual int read_int_from_ups(int ci, int *value);

protected:

   // Inherited from UsbUpsDriver
   virtual bool pusb_ups_get_capabilities();
   virtual bool pusb_get_value(int ci, USB_VALUE *uval);

private:

   /*
    * When we are traversing the USB reports given by the UPS and we
    * find an entry corresponding to an entry in the known_info table
    * above, we make the following USB_INFO entry in the info table
    * of our private data.
    */
   typedef struct s_usb_info {
      unsigned physical;              /* physical value wanted */
      unsigned unit_exponent;         /* exponent */
      unsigned unit;                  /* units */
      int data_type;                  /* data type */
      int ci;                         /* which CI does this usage represent? */
      struct hiddev_usage_ref uref;   /* usage reference (read) */
      struct hiddev_usage_ref wuref;  /* usage reference (write) */
   } USB_INFO;

   void reinitialize_private_structure();
   int open_device(const char *dev);
   void bind_upses();
   bool open_usb_device();
   bool usb_link_check();
   bool populate_uval(USB_INFO *info, USB_VALUE *uval);
   USB_INFO *find_info_by_uref(struct hiddev_usage_ref *uref);
   USB_INFO *find_info_by_ucode(unsigned int ucode);

   int _fd;                         /* Our UPS fd when open */
   bool _compat24;                  /* Linux 2.4 compatibility mode */
   char _orig_device[MAXSTRING];    /* Original port specification */
   USB_INFO *_info[CI_MAXCI + 1];   /* Info pointers for each command */
   bool _linkcheck;
};

#endif
