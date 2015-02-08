/*
 * HidUps.cpp
 *
 * Utility functions for interfacing with the libusbhid userspace
 * HID parsing library.
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

#include "apc.h"
#include "HidUps.h"
#include "usb_common.h"
#include "astring.h"

#define MAX_SANE_DESCRIPTOR_LEN 4096

HidUps::HidUps() :
   _fd(NULL),
   _rdesc(NULL)
{
}

HidUps::~HidUps()
{
   Close();
}

bool HidUps::Open(const char *serno)
{
   /* Set libusb debug level */
   usb_set_debug(debug_level/100);

   /* Initialize libusb */
   Dmsg(200, "Initializing libusb\n");
   usb_init();

   /* Enumerate usb busses and devices */
   int i = usb_find_busses();
   Dmsg(200, "Found %d USB busses\n", i);
   i = usb_find_devices();
   Dmsg(200, "Found %d USB devices\n", i);

   /* Iterate over all devices */
   struct usb_bus *bus = usb_get_busses();
   while (bus)
   {
      struct usb_device *dev = bus->devices;
      while (dev)
      {
         Dmsg(200, "%s:%s - %04x:%04x\n",
            bus->dirname, dev->filename, 
            dev->descriptor.idVendor, dev->descriptor.idProduct);

         if (init_device(dev, serno)) {
            /* Successfully found and initialized an UPS */
            return true;
         }

         dev = dev->next;
      }

      bus = bus->next;
   }

   /* Failed to find an UPS */
   return false;
}

void HidUps::Close()
{
   if (_rdesc)
   {
      hid_dispose_report_desc(_rdesc);
      _rdesc = NULL;
   }

   if (_fd)
   {
      usb_release_interface(_fd, 0);
      usb_reset(_fd);
      usb_close(_fd);
      _fd = NULL;
   }
}

bool HidUps::init_device(
   struct usb_device *dev,
   const char *serno)
{
   int rc;
   unsigned char* rdesc;
   int rdesclen;

   /* Check if this is a supported device before we mess with it */
   if (!MatchVidPid(dev->descriptor.idVendor, dev->descriptor.idProduct)) {
      Dmsg(100, "Not an APC device.\n");
      return false;
   }

   /* Open the device with libusb */
   _fd = usb_open(dev);
   if (!_fd) {
      Dmsg(100, "Unable to open device.\n");
      return false;
   }

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
   /*
    * Attempt to detach the kernel driver so we can drive
    * this device from userspace. Don't worry if this fails;
    * that just means the driver was already detached.
    */
   usb_detach_kernel_driver_np(_fd, 0);
#endif

   /* Check device serial number, if user specified one */
   if (serno && serno[0] != '\0')
   {
      /* Fetch serial number from device */
      const char *tmpser;
      if (dev->descriptor.iSerialNumber == 0 ||
          (tmpser = GetString(dev->descriptor.iSerialNumber)) == NULL)
      {
         usb_close(_fd);
         _fd = NULL;
         Dmsg(100, "Device does not report serial number.\n");
         return false;
      }

      /* Remove leading/trailing whitespace */
      astring serial(tmpser);
      serial.trim();

      /* Check against user specification, ignoring case */
      Dmsg(100, "device='%s', user='%s'\n", serial.str(), serno);
      if (strcasecmp(serial, serno))
      {
         usb_close(_fd);
         _fd = NULL;
         return false;
      }
   }

   /* Choose config #1 */
   rc = usb_set_configuration(_fd, 1);
   if (rc) {
      usb_close(_fd);
      _fd = NULL;
      Dmsg(100, "Unable to set configuration (%d) %s.\n", rc, usb_strerror());
      return false;
   }

   /* Claim the interface */
   rc = usb_claim_interface(_fd, 0);
   if (rc) {
      usb_close(_fd);
      _fd = NULL;
      Dmsg(100, "Unable to claim interface (%d) %s.\n", rc, usb_strerror());
      return false;
   }

   /* Fetch the report descritor */
   rdesc = FetchReportDescr(&rdesclen);
   if (!rdesc) {
      Dmsg(100, "Unable to fetch report descriptor.\n");
      goto error_out;
   }

   /* Initialize hid parser with this descriptor */
   _rdesc = hid_use_report_desc(rdesc, rdesclen);
   free(rdesc);
   if (!_rdesc) {
      Dmsg(100, "Unable to init parser with report descriptor.\n");
      goto error_out;
   }

   /* Does this device have an UPS application collection? */
   if (!LocateItem(
         UPS_USAGE,             /* Match usage code */
         -1,                    /* Don't care about application */
         -1,                    /* Don't care about physical usage */
         -1,                    /* Don't care about logical */
         HID_KIND_COLLECTION,   /* Match collection type */
         NULL)) {
      hid_dispose_report_desc(_rdesc);
      Dmsg(100, "Device does not have an UPS application collection.\n");
      goto error_out;
   }

   return true;

error_out:
   usb_release_interface(_fd, 0);
   usb_close(_fd);
   _fd = NULL;
   return false;
}

/*
 * Fetch a string descriptor from the device given an _fd for the
 * device's control endpoint and the string index. Returns a pointer
 * to a static buffer containing the NUL-terminated string or NULL
 * on failure.
 */
const char *HidUps::GetString(int index)
{
   int rc;
   static char string[128];

   rc = usb_get_string_simple(_fd, index, string, sizeof(string));
   if (rc <= 0) {
      Dmsg(100, "Error fetching string descriptor: (%d) %s\n", rc, strerror(-rc));
      return NULL;
   }

   Dmsg(200, "Got string of length=%d\n", rc);
   return string;
}

/* Fetch a descriptor from an interface (as opposed to from the device) */
int HidUps::GetIntfDescr(
   unsigned char type, unsigned char index, void *buf, int size)
{
   memset(buf, 0, size);

   return usb_control_msg(_fd, USB_ENDPOINT_IN | USB_RECIP_INTERFACE,
                         USB_REQ_GET_DESCRIPTOR,
                         (type << 8) + index, 0, (char*)buf, size, 1000);
}

/*
 * Fetch the report descriptor from the device given an _fd for the
 * device's control endpoint. Descriptor length is written to the
 * rlen out paramter and a pointer to a malloc'ed buffer containing
 * the descriptor is returned. Returns NULL on failure.
 */
unsigned char *HidUps::FetchReportDescr(int *rlen)
{
   unsigned char *ptr;
   int rdesclen;

   ptr = (unsigned char*)malloc(MAX_SANE_DESCRIPTOR_LEN);
   rdesclen = GetIntfDescr(USB_DT_REPORT, 0, ptr, MAX_SANE_DESCRIPTOR_LEN);
   if (rdesclen <= 0) {
      Dmsg(100, "Unable to get REPORT descriptor (%d).\n", rdesclen);
      free(ptr);
      return NULL;
   }

   Dmsg(300, "Report descriptor:\n");
   hex_dump(300, ptr, rdesclen);

   *rlen = rdesclen;
   return ptr;
}

/* Push a value onto the collection stack */
#define PUSH_COLLECTION(c, v)             \
do                                        \
{                                         \
    if (c##_idx<MAX_COLLECTION_NESTING-1) \
    {                                     \
        c##_idx++;                        \
        c##_stack[c##_idx] = v;           \
    }                                     \
} while(0)

/* Remove a value from the collection stack */
#define POP_COLLECTION(c) \
do                        \
{                         \
    if (c##_idx >= 0)     \
        c##_idx--;        \
} while(0)

/* Get the topmost item on the stack */
#define TOP_COLLECTION(c) \
    ((c##_idx == -1) ? -1 : c##_stack[c##_idx])

/* Collection types */
#define HIDCOL_PHYSICAL     0
#define HIDCOL_APPLICATION  1
#define HIDCOL_LOGICAL      2
#define MAX_COLLECTION_NESTING 10

/* For pretty printing... */
#define KIND_TO_CHAR(x)                         \
        ((x) == hid_input) ? 'I' :              \
        ((x) == hid_output) ? 'O' :             \
        ((x) == hid_feature) ? 'F' :            \
        ((x) == hid_collection) ? 'C' :         \
        ((x) == hid_endcollection) ? 'E' : '?'

#define COLLECTION_TO_CHAR(x)   \
        ((x) == 0) ? 'P' :      /* Physical */       \
        ((x) == 1) ? 'A' :      /* Application */    \
        ((x) == 2) ? 'L' :      /* Logical */        \
        ((x) == 3) ? 'R' :      /* Report */         \
        ((x) == 4) ? 'N' :      /* Named Array */    \
        ((x) == 5) ? 'S' :      /* Usage Switch */   \
        ((x) == 6) ? 'M' : '?'  /* Usage Modifier */


/*
 * Locate an item matching the given parameters. If found, the
 * item is copied to the supplied buffer. Returns true on success,
 * false on failure. Any of usage, app, phys, logical, and kind
 * may be set to -1 for "don't care".
 */
int HidUps::LocateItem(int usage, int app, int phys,
   int logical, int kind, hid_item_t *outitem)
{
   int rc;
   hid_data_t cookie;
   hid_item_t item;

   int phys_stack[MAX_COLLECTION_NESTING];
   int app_stack[MAX_COLLECTION_NESTING];
   int logical_stack[MAX_COLLECTION_NESTING];
   int phys_idx = -1, app_idx = -1, logical_idx = -1;

   cookie = hid_start_parse(_rdesc, HID_KIND_ALL, -1);
   if (!cookie) {
      Dmsg(100, "Unable to start hid parser\n");
      return 0;
   }

   while ((rc = hid_get_item(cookie, &item)) > 0) {
      if (item.kind == hid_collection) {
         if (item.collection == HIDCOL_PHYSICAL)
            PUSH_COLLECTION(phys, item.usage);
         else if (item.collection == HIDCOL_LOGICAL)
            PUSH_COLLECTION(logical, item.usage);
         else if (item.collection == HIDCOL_APPLICATION)
            PUSH_COLLECTION(app, item.usage);
      }

      if (usage != -1 && (unsigned int)usage != item.usage)
         goto next;
      if (app != -1 && app != TOP_COLLECTION(app))
         goto next;
      if (phys != -1 && phys != TOP_COLLECTION(phys))
         goto next;
      if (logical != -1 && logical != TOP_COLLECTION(logical))
         goto next;
      if (kind != -1 && ((1 << item.kind) & kind) == 0)
         goto next;

      if (outitem)
         memcpy(outitem, &item, sizeof(item));

      hid_end_parse(cookie);
      return 1;

    next:
      if (item.kind == hid_endcollection) {
         if (item.collection == HIDCOL_PHYSICAL)
            POP_COLLECTION(phys);
         else if (item.collection == HIDCOL_LOGICAL)
            POP_COLLECTION(logical);
         else if (item.collection == HIDCOL_APPLICATION)
            POP_COLLECTION(app);
      }
   }

   hid_end_parse(cookie);
   return 0;
}

#define USB_REQ_GET_REPORT 0x01
#define USB_REQ_SET_REPORT 0x09

/*
 * Fetch a report from a device given an _fd for the device's control
 * endpoint, the populated item structure describing the report, a
 * data buffer in which to store the result, and the report length.
 * Returns actual report length (in bytes) on success and -1 on failure.
 */
int HidUps::GetReport(hid_item_t *item, unsigned char *data, int len)
{
   int actlen;

   Dmsg(200, "get_report: id=0x%02x, kind=%d, length=%d pos=%d\n",
      item->report_ID, item->kind, len, item->pos);

   actlen = usb_control_msg(_fd,
      USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
      USB_REQ_GET_REPORT, ((item->kind + 1) << 8) | item->report_ID,
      0, (char*)data, len, 1000);

   if (actlen <= 0) {
      Dmsg(100, "Error getting report: (%d) %s\n", actlen, strerror(-actlen));
      return -1;
   }

   hex_dump(300, data, actlen);

   return actlen;
}

/*
 * Send a report to the device given an _fd for the device's control
 * endpoint, the populated item structure, the data to send, and the
 * report length. Returns true on success, false on failure.
 */
int HidUps::SetReport(hid_item_t *item, unsigned char *data, int len)
{
   int actlen;

   Dmsg(200, "set_report: id=0x%02x, kind=%d, length=%d pos=%d\n",
      item->report_ID, item->kind, len, item->pos);
   hex_dump(300, data, len);

   actlen = usb_control_msg(_fd,
      USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
      USB_REQ_SET_REPORT, ((item->kind + 1) << 8) | item->report_ID,
      0, (char*)data, len, 1000);
   if (actlen != len) {
      Dmsg(100, "Error setting report: (%d) %s\n", actlen, strerror(-actlen));
      return 0;
   }

   return 1;
}
