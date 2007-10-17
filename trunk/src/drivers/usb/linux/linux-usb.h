/*
 * linux-usb.h
 *
 * Linux hiddev based USB UPS driver
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 2004-2007 Adam Kropelin
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

#ifndef _LINUX_USB_H
#define _LINUX_USB_H

#include "../usb.h"
#include <asm/types.h>
#include <linux/hiddev.h>

class LinuxUsbDriver: public UsbDriver
{
public:

   LinuxUsbDriver(UPSINFO *ups);
   virtual ~LinuxUsbDriver() {}

   virtual bool SubclassGetCapabilities();
   virtual bool SubclassOpen();
   virtual bool SubclassClose();
   virtual bool SubclassCheckState();
   virtual bool SubclassGetValue(int ci, usb_value *uval);
   virtual bool SubclassWriteIntToUps(int ci, int value, const char *name);
   virtual bool SubclassReadIntFromUps(int ci, int *value);

private:

   struct usb_info;

   void reinitialize();
   int  open_device(const char *dev);
   bool open_usb_device();
   bool link_check();
   bool populate_uval(usb_info *info, usb_value *uval);
   usb_info *find_info_by_uref(struct hiddev_usage_ref *uref);
   usb_info *find_info_by_ucode(unsigned int ucode);

   int _fd;                         /* Our UPS fd when open */
   bool _compat24;                  /* Linux 2.4 compatibility mode */
   char _orig_device[MAXSTRING];    /* Original port specification */
   usb_info *_info[CI_MAXCI + 1];   /* Info pointers for each command */
   bool _linkcheck;                 /* Are we in the linkcheck state? */
};

#endif  /* _LINUX_USB_H */
