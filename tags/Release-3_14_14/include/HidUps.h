/*
 * HidUps.h
 *
 * Utility class for interfacing with a HID based UPS using libusb and the 
 * libusbhid userspace HID parsing library.
 */

/*
 * Copyright (C) 2004-2014 Adam Kropelin
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

#ifndef _HIDUPS_H
#define _HIDUPS_H

#include "usbhid.h"
#include "libusb.h"

/* Forward-declaration */
struct usb_dev_handle;

#define HID_KIND_INPUT          (1 << hid_input)
#define HID_KIND_OUTPUT         (1 << hid_output)
#define HID_KIND_FEATURE        (1 << hid_feature)
#define HID_KIND_COLLECTION     (1 << hid_collection)
#define HID_KIND_ENDCOLLECTION  (1 << hid_endcollection)
#define HID_KIND_ALL            (HID_KIND_INPUT         | \
                                 HID_KIND_OUTPUT        | \
                                 HID_KIND_FEATURE       | \
                                 HID_KIND_COLLECTION    | \
                                 HID_KIND_ENDCOLLECTION)

class HidUps
{
public:

   HidUps();
   ~HidUps();

   bool Open(const char *serno = NULL);
   void Close();

   /*
    * Locate an item matching the given parameters. If found, the
    * item is copied to the supplied buffer. Returns true on success,
    * false on failure. Any of usage, app, phys, logical, and kind
    * may be set to -1 for "don't care".
    */
   int LocateItem(int usage, int app, int phys,
      int logical, int kind, hid_item_t *outitem);

   /*
    * Fetch a report from a device given an fd for the device's control
    * endpoint, the populated item structure describing the report, a
    * data buffer in which to store the result, and the report length.
    * Returns actual report length (in bytes) on success and -1 on failure.
    */
   int GetReport(hid_item_t *item, unsigned char *data, int len);

   /*
    * Send a report to the device given an fd for the device's control
    * endpoint, the populated item structure, the data to send, and the
    * report length. Returns true on success, false on failure.
    */
   int SetReport(hid_item_t *item, unsigned char *data, int len);

   /*
    * Fetch a string descriptor from the device given an fd for the
    * device's control endpoint and the string index. Returns a pointer
    * to a static buffer containing the NUL-terminated string or NULL
    * on failure.
    */
   const char *GetString(int index);

   int GetReportSize(enum hid_kind k, int id)
      { return hid_report_size(_rdesc, k, id); }

   int InterruptWrite(int ep, const void *bytes, int size, int timeout)
      { return usb_interrupt_write(_fd, ep, (char*)bytes, size, timeout); }

   int InterruptRead(int ep, const void *bytes, int size, int timeout)
      { return usb_interrupt_read(_fd, ep, (char*)bytes, size, timeout); }

private:

   /*
    * Fetch the report descriptor from the device given an fd for the
    * device's control endpoint. Descriptor length is written to the
    * rlen out paramter and a pointer to a malloc'ed buffer containing
    * the descriptor is returned. Returns NULL on failure.
    */
   unsigned char *FetchReportDescr(int *rlen);

   /* Fetch a descriptor from an interface (as opposed to from the device) */
   int GetIntfDescr(
      unsigned char type, unsigned char index, void *buf, int size);

   bool init_device(struct usb_device *dev, const char *serno);

   usb_dev_handle *_fd;
   report_desc_t _rdesc;
};

#endif
