/*
 * usb.h
 *
 * Public USB driver interface exposed to the driver management layer.
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 2004-2005 Adam Kropelin
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

class UsbUpsDriver: public UpsDriver
{
public:
   UsbUpsDriver(UPSINFO *ups);
   virtual ~UsbUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups);

   // UpsDriver functions impemented in UsbUpsDriver base class
   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool shutdown();
   virtual bool entry_point(int command, void *data);

   // Extra functions exported for use by apctest
   // Implemented by derived XXXUsbUpsDriver class
   virtual int write_int_to_ups(int ci, int value, char const* name) = 0;
   virtual int read_int_from_ups(int ci, int *value) = 0;

protected:

   typedef struct s_usb_value {
      int value_type;               /* Type of returned value */
      double dValue;                /* Value if double */
      int iValue;                   /* Integer value */
      const char *UnitName;         /* Name of units */
      char sValue[MAXSTRING];       /* Value if string */
   } USB_VALUE;

   // Helper functions implemented in UsbUpsDriver
   bool usb_get_value(int ci, USB_VALUE *uval);
   bool usb_process_value_bup(int ci, USB_VALUE* uval);
   void usb_process_value(int ci, USB_VALUE* uval);
   bool usb_update_value(int ci);
   bool usb_report_event(int ci, USB_VALUE *uval);
   double pow_ten(int exponent);

   struct s_known_info {
      int ci;                       /* Command index */
      unsigned usage_code;          /* Usage code */
      unsigned physical;            /* Physical usage */
      unsigned logical;             /* Logical usage */
      int data_type;                /* Data type expected */
      bool isvolatile;              /* Volatile data item */
   };

   static const s_known_info _known_info[];

   // Functions implemented in derived XXXUsbUpsDriver class
   virtual bool pusb_ups_get_capabilities() = 0;
   virtual bool pusb_get_value(int ci, USB_VALUE *uval) = 0;

   bool _quirk_old_backups_pro;
   struct timeval _prev_time;
   int _bpcnt;
};

#endif  /* _USB_H */
