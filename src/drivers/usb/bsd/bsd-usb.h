/*
 * bsd-usb.h
 *
 * Platform-specific USB module for *BSD ugen USB driver.
 *
 * Based on linux-usb.c by Kerb Sibbald.
 */

/*
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

#ifndef _BSDUSB_H
#define _BSDUSB_H

#include "../usb.h"

class BsdUsbUpsDriver: public UsbUpsDriver
{
public:
   BsdUsbUpsDriver(UPSINFO *ups);
   virtual ~BsdUsbUpsDriver() {}

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
    * When we are traversing the USB reports given by the UPS and we find
    * an entry corresponding to an entry in the known_info table above,
    * we make the following USB_INFO entry in the info table of our
    * private data.
    */
   typedef struct s_usb_info {
      unsigned usage_code;          /* usage code wanted */
      unsigned unit_exponent;       /* exponent */
      unsigned unit;                /* units */
      int data_type;                /* data type */
      hid_item_t item;              /* HID item (read) */
      hid_item_t witem;             /* HID item (write) */
      int report_len;               /* Length of containing report */
      int ci;                       /* which CI does this usage represent? */
      int value;                    /* Previous value of this item */
   } USB_INFO;

   void reinitialize_private_structure();
   bool init_device(const char *devname);
   bool open_usb_device();
   bool usb_link_check();
   bool populate_uval(USB_INFO *info, unsigned char *data, USB_VALUE *uval);

   int _fd;                         /* Our UPS control pipe fd when open */
   int _intfd;                      /* Interrupt pipe fd */
   char _orig_device[MAXSTRING];    /* Original port specification */
   report_desc_t _rdesc;            /* Device's report descrptor */
   USB_INFO *_info[CI_MAXCI + 1];   /* Info pointers for each command */
   bool _linkcheck;
};

#endif
