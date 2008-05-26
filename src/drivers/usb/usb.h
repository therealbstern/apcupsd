/*
 * usb.h
 *
 * Public USB driver interface exposed to the driver management layer.
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

#ifndef _USB_H
#define _USB_H

#include "drivers.h"
#include "config.h"

class UsbDriver: public UpsDriver
{
public:

   UsbDriver(UPSINFO *ups) : UpsDriver(ups, "usb") {}
   virtual ~UsbDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState() { return SubclassCheckState(); }
   virtual bool Close() { return SubclassClose(); }

   // Optional methods
   virtual bool KillPower();
   virtual bool EntryPoint(int command, void *data);

protected:

   struct usb_value {
      int value_type;               /* Type of returned value */
      double dValue;                /* Value if double */
      int iValue;                   /* Integer value */
      const char *UnitName;         /* Name of units */
      char sValue[MAXSTRING];       /* Value if string */
   };

   // Platform-specific derived class must implement
   virtual bool SubclassGetCapabilities() = 0;
   virtual bool SubclassOpen() = 0;
   virtual bool SubclassClose() = 0;
   virtual bool SubclassCheckState() = 0;
   virtual bool SubclassGetValue(int ci, usb_value *uval) = 0;
   virtual bool SubclassWriteIntToUps(int ci, int value, const char *name) = 0;
   virtual bool SubclassReadIntFromUps(int ci, int *value) = 0;

   // Upcalls from derived class to UsbDriver base class
   bool report_event(int ci, usb_value *uval);
   static double pow_ten(int exponent);

   // USB HID spec mappings to apcupsd CIs
   struct known_info {
      int ci;                       /* Command index */
      unsigned usage_code;          /* Usage code */
      unsigned physical;            /* Physical usage */
      int data_type;                /* Data type expected */
      bool isvolatile;              /* Volatile data item */
   };
   static const known_info _known_info[];

private:

   // Data handling callback functions
   void configure_callbacks();
   void process_bool(int ci, usb_value *uval);
   void process_string(int ci, usb_value *uval);
   void process_volts(int ci, usb_value *uval);
   void process_percent(int ci, usb_value *uval);
   void process_freq(int ci, usb_value *uval);
   void process_power(int ci, usb_value *uval);
   void process_date(int ci, usb_value *uval);
   void process_date_bcd(int ci, usb_value *uval);
   void process_model(int ci, usb_value *uval);
   void process_selftest(int ci, usb_value *uval);
   void process_sensitivity(int ci, usb_value *uval);
   void process_battpresent(int ci, usb_value *uval);
   void process_whybatt(int ci, usb_value *uval);
   void process_timesecs(int ci, usb_value *uval);
   void process_temp(int ci, usb_value *uval);
   void process_status(int ci, usb_value *uval);
   void process_timemins(int ci, usb_value *uval);
   void process_alarm(int ci, usb_value *uval);
   void process_asciivolts(int ci, usb_value *uval);
   void process_asciipct(int ci, usb_value *uval);
   void process_asciifreq(int ci, usb_value *uval);

   // Fixup functions for UPS bug workarounds
   void fixup_nomv(int ci, usb_value *uval);

   // Misc helpers
   bool get_value(int ci, usb_value *uval);
   bool update_value(int ci);
   void process_value(int ci, usb_value* uval);
   bool write_int_to_ups(int ci, int value, const char *name)
      { return SubclassWriteIntToUps(ci, value, name); }
   bool read_int_from_ups(int ci, int *value)
      { return SubclassReadIntFromUps(ci, value); }

   // Thread body function
   virtual void body();

   int _batt_present_count;         // Battery present count
   struct timeval _last_urb_time;   // Last time we talked to the UPS

   // Data-handling callback array
   typedef void (UsbDriver::*Callback)(int, usb_value*);
   Callback _callbacks[CI_MAXCI + 1];
   Callback _fixups[CI_MAXCI + 1];
};

/* Max rate to update volatile data */
#define MAX_VOLATILE_POLL_RATE 5

/* How often to retry the link (seconds) */
#define LINK_RETRY_INTERVAL    5 

/* USB Vendor ID's */ 
#define VENDOR_APC 0x51D
#define VENDOR_MGE 0x463

/* Various known USB codes */ 
#define UPS_USAGE   0x840004
#define UPS_VOLTAGE 0x840030
#define UPS_OUTPUT  0x84001c
#define UPS_BATTERY 0x840012

/* These are the data_type expected for our know_info */ 
#define T_NONE     0          /* No units */
#define T_INDEX    1          /* String index */
#define T_CAPACITY 2          /* Capacity (usually %) */
#define T_BITS     3          /* Bit field */
#define T_UNITS    4          /* Use units/exponent field */
#define T_DATE     5          /* Date */
#define T_APCDATE  6          /* APC date */

/* These are the resulting value types returned */ 
#define V_DOUBLE   1          /* Double */ 
#define V_STRING   2          /* String pointer */
#define V_INTEGER  3          /* Integer */

/* These are the desired Physical usage values we want */ 
#define P_ANY     0           /* Any value */
#define P_OUTPUT  0x84001c    /* Output values */
#define P_BATTERY 0x840012    /* Battery values */
#define P_INPUT   0x84001a    /* Input values */
#define P_PWSUM   0x840024    /* Power summary */
#define P_APC1    0xff860007  /* From AP9612 environmental monitor */

/* No Command Index, don't save this value */ 
#define CI_NONE -1

/* Check if the UPS has the given capability */ 
#define UPS_HAS_CAP(ci) (_ups->UPS_Cap[ci])

/* Include headers for platform-specific drivers */
#ifdef HAVE_LINUX_USB
#include "linux/linux-usb.h"
#endif
#ifdef HAVE_GENERIC_USB
#include "generic/generic-usb.h"
#endif
#ifdef HAVE_BSD_USB
#include "linux/bsd-usb.h"
#endif

#endif  /* _USB_H */
