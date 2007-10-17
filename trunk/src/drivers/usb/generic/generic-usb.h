/*
 * generic-usb.h
 *
 * Generic libusb based USB UPS driver.
 */

/*
 * Copyright (C) 2005-2007 Adam Kropelin
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

#ifndef _GENERIC_USB_H
#define _GENERIC_USB_H

#include "../usb.h"
#include "libusb.h"

typedef struct report_desc *report_desc_t;

class GenericUsbDriver: public UsbDriver
{
public:

   GenericUsbDriver(UPSINFO *ups);
   virtual ~GenericUsbDriver() {}

   virtual bool SubclassGetCapabilities();
   virtual bool SubclassOpen();
   virtual bool SubclassClose();
   virtual bool SubclassCheckState();
   virtual bool SubclassGetValue(int ci, usb_value *uval);
   virtual bool SubclassWriteIntToUps(int ci, int value, const char *name);
   virtual bool SubclassReadIntFromUps(int ci, int *value);

private:

   struct usb_info;

   bool populate_uval(usb_info *info, unsigned char *data, usb_value *uval);
   void reinitialize();
   bool init_device(struct usb_device *dev);
   bool open_usb_device();
   bool usb_link_check();

   usb_dev_handle *_fd;             /* Our UPS control pipe fd when open */
   char _orig_device[MAXSTRING];    /* Original port specification */
   short _vendor;                   /* UPS vendor id */
   report_desc_t _rdesc;            /* Device's report descrptor */
   usb_info *_info[CI_MAXCI + 1];   /* Info pointers for each command */
   bool _linkcheck;                 /* Are we in the linkcheck state? */
   static bool _libusbinit;         /* Has libusb been initialized yet? */
};

#endif  /* _GENERIC_USB_H */
