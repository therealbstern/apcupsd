/*
 * generic-usb.c
 *
 * Generic USB module for libusb.
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

#include "apc.h"
#include "generic-usb.h"
#include "hidutils.h"

// Statics used to coordinate first time libusb init
bool GenericUsbDriver::_libusbinit = false;

/*
 * When we are traversing the USB reports given by the UPS and we find
 * an entry corresponding to an entry in the known_info table,
 * we make the following usb_info entry in the info table of our
 * private data.
 */
struct GenericUsbDriver::usb_info {
   unsigned usage_code;            /* usage code wanted */
   unsigned unit_exponent;         /* exponent */
   unsigned unit;                  /* units */
   int data_type;                  /* data type */
   hid_item_t item;                /* HID item */
   int report_len;                 /* Length of containing report */
   int ci;                         /* which CI does this usage represent? */
   int value;                      /* Previous value of this item */
};

GenericUsbDriver::GenericUsbDriver(UPSINFO *ups)
   : UsbDriver(ups)
{
   memset(_info, 0, sizeof(_info));

   /* Initialize libusb */
   if (!_libusbinit) {
      Dmsg0(200, "Initializing libusb\n");
      usb_set_debug(debug_level/100);
      usb_init();
      _libusbinit = true;
   }
}

bool GenericUsbDriver::populate_uval(
   usb_info *info, unsigned char *data, usb_value *uval)
{
   const char *str;
   int exponent;
   usb_value val;

   /* data+1 skips the report tag byte */
   info->value = hid_get_data(data+1, &info->item);

   exponent = info->unit_exponent;
   if (exponent > 7)
      exponent = exponent - 16;

   if (info->data_type == T_INDEX) {    /* get string */
      if (info->value == 0)
         return false;

      str = hidu_get_string(_fd, info->value);
      if (!str)
         return false;

      astrncpy(val.sValue, str, sizeof(val.sValue));
      val.value_type = V_STRING;

      Dmsg4(200, "Def val=%d exp=%d sVal=\"%s\" ci=%d\n", info->value,
         exponent, val.sValue, info->ci);
   } else if (info->data_type == T_UNITS) {
      val.value_type = V_DOUBLE;

      switch (info->unit) {
      case 0x00F0D121:
         val.UnitName = "Volts";
         exponent -= 7;            /* remove bias */
         break;
      case 0x00100001:
         exponent += 2;            /* remove bias */
         val.UnitName = "Amps";
         break;
      case 0xF001:
         val.UnitName = "Hertz";
         break;
      case 0x1001:
         val.UnitName = "Seconds";
         break;
      case 0xD121:
         exponent -= 7;            /* remove bias */
         val.UnitName = "Watts";
         break;
      case 0x010001:
         val.UnitName = "Degrees K";
         break;
      case 0x0101001:
         val.UnitName = "AmpSecs";
         if (exponent == 0)
            val.dValue = info->value;
         else
            val.dValue = ((double)info->value) * pow_ten(exponent);
         break;
      default:
         val.UnitName = "";
         val.value_type = V_INTEGER;
         val.iValue = info->value;
         break;
      }

      if (exponent == 0)
         val.dValue = info->value;
      else
         val.dValue = ((double)info->value) * pow_ten(exponent);

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   } else {                        /* should be T_NONE */

      val.UnitName = "";
      val.value_type = V_INTEGER;
      val.iValue = info->value;

      if (exponent == 0)
         val.dValue = info->value;
      else
         val.dValue = ((double)info->value) * pow_ten(exponent);

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   }

   memcpy(uval, &val, sizeof(*uval));
   return true;   
}

void GenericUsbDriver::reinitialize()
{
   Dmsg0(200, "Reinitializing private structure.\n");

   /*
    * We are being reinitialized, so clear the Cap
    *   array, and release previously allocated memory.
    */
   for (int k = 0; k <= CI_MAXCI; k++) {
      _ups->UPS_Cap[k] = false;
      if (_info[k] != NULL) {
         free(_info[k]);
         _info[k] = NULL;
      }
   }

   _fd = NULL;
   _linkcheck = false;
}

bool GenericUsbDriver::init_device(struct usb_device *dev)
{
   usb_dev_handle *fd;
   int rc;
   unsigned char* rdesc;
   int rdesclen;

   /* Open the device with libusb */
   fd = usb_open(dev);
   if (!fd) {
      Dmsg0(100, "Unable to open device.\n");
      return false;
   }

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
   /*
    * Attempt to detach the kernel driver so we can drive
    * this device from userspace.
    */
   rc = usb_detach_kernel_driver_np(fd, 0);
   Dmsg1(200, "Kernel detach returned %d\n", rc);
#endif

   /* Choose config #1 */
   rc = usb_set_configuration(fd, 1);
   if (rc) {
      usb_close(fd);
      Dmsg2(100, "Unable to set configuration (%d) %s.\n", rc, usb_strerror());
      return false;
   }

   /* Claim the interface */
   rc = usb_claim_interface(fd, 0);
   if (rc) {
      usb_close(fd);
      Dmsg2(100, "Unable to claim interface (%d) %s.\n", rc, usb_strerror());
      return false;
   }

   /* Fetch the report descritor */
   rdesc = hidu_fetch_report_descriptor(fd, &rdesclen);
   if (!rdesc) {
      usb_close(fd);
      Dmsg0(100, "Unable to fetch report descriptor.\n");
      return false;
   }

   /* Initialize hid parser with this descriptor */
   _rdesc = hid_use_report_desc(rdesc, rdesclen);
   free(rdesc);
   if (!_rdesc) {
      usb_close(fd);
      Dmsg0(100, "Unable to init parser with report descriptor.\n");
      return false;
   }

   /* Does this device have an UPS application collection? */
   if (!hidu_locate_item(
         _rdesc,
         UPS_USAGE,             /* Match usage code */
         -1,                    /* Don't care about application */
         -1,                    /* Don't care about physical usage */
         -1,                    /* Don't care about logical */
         HID_KIND_COLLECTION,   /* Match collection type */
         NULL)) {
      hid_dispose_report_desc(_rdesc);
      usb_close(fd);
      Dmsg0(100, "Device does not have an UPS application collection.\n");
      return false;
   }

   _fd = fd;
   return true;
}

bool GenericUsbDriver::open_usb_device()
{
   int i;
   struct usb_bus* bus;
   struct usb_device* dev;

   /* Enumerate usb busses and devices */
   i = usb_find_busses();
   Dmsg1(200, "Found %d USB busses\n", i);
   i = usb_find_devices();
   Dmsg1(200, "Found %d USB devices\n", i);

   /* Iterate over all devices, checking for idVendor=APC */
   bus = usb_get_busses();
   while (bus)
   {
      dev = bus->devices;
      while (dev)
      {
         Dmsg4(200, "%s:%s - %04x:%04x\n",
            bus->dirname, dev->filename, 
            dev->descriptor.idVendor, dev->descriptor.idProduct);

         if (dev->descriptor.idVendor == VENDOR_APC) {
            Dmsg2(200, "Trying device %s:%s\n", bus->dirname, dev->filename);
            if (init_device(dev)) {
               /* Successfully found and initialized an UPS */
               astrncpy(_ups->device, bus->dirname, sizeof(_ups->device));
               astrncat(_ups->device, ":", sizeof(_ups->device));
               astrncat(_ups->device, dev->filename, sizeof(_ups->device));
               return true;
            }
         }

         dev = dev->next;
      }
      
      bus = bus->next;
   }

   /* Failed to find an UPS */
   _ups->device[0] = 0;
   return false;
}

/* 
 * Called if there is an ioctl() or read() error, we close() and
 * re open() the port since the device was probably unplugged.
 */
bool GenericUsbDriver::usb_link_check()
{
   bool comm_err = true;
   int tlog;
   bool once = true;

   if (_linkcheck)
      return false;

   _linkcheck = true;               /* prevent recursion */

   _ups->set_commlost();
   Dmsg0(200, "link_check comm lost\n");

   /* Don't warn until we try to get it at least 2 times and fail */
   for (tlog = LINK_RETRY_INTERVAL * 2; comm_err; tlog -= (LINK_RETRY_INTERVAL)) {

      if (tlog <= 0) {
         tlog = 10 * 60;           /* notify every 10 minutes */
         log_event(_ups, event_msg[CMDCOMMFAILURE].level,
                   event_msg[CMDCOMMFAILURE].msg);
         if (once) {               /* execute script once */
            execute_command(_ups, ups_event[CMDCOMMFAILURE]);
            once = false;
         }
      }

      /* Retry every LINK_RETRY_INTERVAL seconds */
      sleep(LINK_RETRY_INTERVAL);

      if (_fd) {
         usb_reset(_fd);
         usb_close(_fd);
         _fd = NULL;
         hid_dispose_report_desc(_rdesc);
         reinitialize();
      }

      if (open_usb_device() && SubclassGetCapabilities() && ReadStaticData()) {
         comm_err = false;
      } else {
         continue;
      }
   }

   if (!comm_err) {
      generate_event(_ups, CMDCOMMOK);
      _ups->clear_commlost();
      Dmsg0(200, "link check comm OK.\n");
   }

   _linkcheck = false;
   return true;
}

/*
 * Get a field value
 */
bool GenericUsbDriver::SubclassGetValue(int ci, usb_value *uval)
{
   usb_info *info = _info[ci];
   unsigned char data[20];
   int len;

   /*
    * Note we need to check info since CI_STATUS is always true
    * even when the UPS doesn't directly support that CI.
    */
   if (!UPS_HAS_CAP(ci) || !info)
      return false;                /* UPS does not have capability */

   /*
    * Clear the destination buffer. In the case of a short transfer (see
    * below) this will increase the likelihood of extracting the correct
    * value in spite of the missing data.
    */
   memset(data, 0, sizeof(data));

   /* Fetch the proper report */
   len = hidu_get_report(_fd, &info->item, data, info->report_len);
   if (len == -1)
      return false;

   /*
    * Some UPSes seem to have broken firmware that sends a different number
    * of bytes (usually fewer) than the report descriptor specifies. On
    * UHCI controllers under *BSD, this can lead to random lockups. To
    * reduce the likelihood of a lockup, we adjust our expected length to
    * match the actual as soon as a mismatch is detected, so future
    * transfers will have the proper lengths from the outset. NOTE that
    * the data returned may not be parsed properly (since the parsing is
    * necessarily based on the report descriptor) but given that HID
    * reports are in little endian byte order and we cleared the buffer
    * above, chances are good that we will actually extract the right
    * value in spite of the UPS's brokenness.
    */
   if (info->report_len != len) {
      Dmsg4(100, "Report length mismatch, fixing "
         "(id=%d, ci=%d, expected=%d, actual=%d)\n",
         info->item.report_ID, ci, info->report_len, len);
      info->report_len = len;
   }

   /* Populate a uval struct using the raw report data */
   return populate_uval(info, data, uval);
}

bool GenericUsbDriver::SubclassGetCapabilities()
{
   int i, rc, ci, phys;
   hid_item_t item;
   usb_info *info;

   write_lock(_ups);

   for (i = 0; _known_info[i].usage_code; i++) {
      ci = _known_info[i].ci;
      phys = _known_info[i].physical;

      if (ci != CI_NONE && !_ups->UPS_Cap[ci]) {
         /* Prefer input items, but try feature if input fails */
         rc = hidu_locate_item(
               _rdesc,
               _known_info[i].usage_code,    /* Match usage code */
               -1,                           /* Don't care about application */
               (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
               -1,                           /* Don't care about logical */
               HID_KIND_INPUT,               /* Match feature type */
               &item);

         if (!rc) {
            rc = hidu_locate_item(
                  _rdesc,
                  _known_info[i].usage_code,    /* Match usage code */
                  -1,                           /* Don't care about application */
                  (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
                  -1,                           /* Don't care about logical */
                  HID_KIND_FEATURE,             /* Match feature type */
                  &item);
         }

         if (rc) {
            _ups->UPS_Cap[ci] = true;

            info = (usb_info *)malloc(sizeof(usb_info));
            if (!info) {
               write_unlock(_ups);
               Error_abort0(_("Out of memory.\n"));
            }

            _info[ci] = info;
            info->ci = ci;
            info->usage_code = item.usage;
            info->unit_exponent = item.unit_exponent;
            info->unit = item.unit;
            info->data_type = _known_info[i].data_type;
            memcpy(&info->item, &item, sizeof(item));
            info->report_len = hid_report_size( /* +1 for report id */
               _rdesc, item.kind, item.report_ID) + 1;
            Dmsg5(200, "Got ci=%d, rpt=%d (len=%d), usage=0x%x (len=%d)\n",
               ci, item.report_ID, info->report_len,
               _known_info[i].usage_code, item.report_size);
         }
      }
   }

   _ups->UPS_Cap[CI_STATUS] = true; /* we always have status flag */
   write_unlock(_ups);
   return true;
}

/*
 * libusb-win32, pthreads, and compat.h all have different ideas
 * of what ETIMEDOUT is on mingw. We need to make sure we match
 * libusb-win32's error.h in that case, so override ETIMEDOUT.
 */
#ifdef HAVE_MINGW
# define LIBUSB_ETIMEDOUT   116
#else
# define LIBUSB_ETIMEDOUT   ETIMEDOUT
#endif

bool GenericUsbDriver::SubclassCheckState()
{
   int i, ci;
   int retval, value;
   unsigned char buf[20];
   struct timeval now, exit;
   int timeout;
   usb_value uval;
   bool done = false;

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += _ups->wait_time;

   while (!done) {

      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);
      timeout = TV_DIFF_MS(now, exit);
      if (timeout <= 0) {
         /* Done already? How time flies... */
         return false;
      }

      Dmsg1(200, "Timeout=%d\n", timeout);
      retval = usb_interrupt_read(_fd, USB_ENDPOINT_IN|1, (char*)buf, sizeof(buf), timeout);

      if (retval == 0 || retval == -LIBUSB_ETIMEDOUT) {
         /* No events available in _ups->wait_time seconds. */
         return false;
      } else if (retval == -EINTR || retval == -EAGAIN) {
         /* assume SIGCHLD */
         continue;
      } else if (retval < 0) {
         /* Hard error */
         Dmsg2(200, "usb_interrupt_read error: (%d) %s\n", retval, strerror(-retval));
         usb_link_check();      /* link is down, wait */
         return false;
      }

      if (debug_level >= 300) {
         logf("Interrupt data: ");
         for (i = 0; i < retval; i++)
            logf("%02x, ", buf[i]);
         logf("\n");
      }

      write_lock(_ups);

      /*
       * Iterate over all CIs, firing off events for any that are
       * affected by this report.
       */
      for (ci=0; ci<CI_MAXCI; ci++) {
         if (_ups->UPS_Cap[ci] && _info[ci] &&
             _info[ci]->item.report_ID == buf[0]) {

            /* Ignore this event if the value has not changed */
            value = hid_get_data(buf+1, &_info[ci]->item);
            if (_info[ci]->value == value) {
               Dmsg3(200, "Ignoring unchanged value (ci=%d, rpt=%d, val=%d)\n",
                  ci, buf[0], value);
               continue;
            }

            Dmsg3(200, "Processing changed value (ci=%d, rpt=%d, val=%d)\n",
               ci, buf[0], value);

            /* Populate a uval and report it to the upper layer */
            populate_uval(_info[ci], buf, &uval);
            if (report_event(ci, &uval)) {
               /*
                * The upper layer considers this an important event,
                * so we will return after processing any remaining
                * CIs for this report.
                */
               done = true;
            }
         }
      }

      write_unlock(_ups);
   }
   
   return true;
}

/*
 * Open usb port
 *
 * This is called once by the core code and is the first 
 * routine called.
 */
bool GenericUsbDriver::SubclassOpen()
{
   write_lock(_ups);
   reinitialize();

   if (!open_usb_device()) {
      write_unlock(_ups);
      Error_abort0(_("Cannot find UPS device --\n"
            "For a link to detailed USB trouble shooting information,\n"
            "please see <http://www.apcupsd.com/support.html>.\n"));
   }

   /*
    * Note, we set _ups->fd here so the "core" of apcupsd doesn't
    * think we are a slave, which is what happens when it is -1.
    * (ADK: Actually this only appears to be true for apctest as
    * apcupsd proper uses the UPS_slave flag.)
    * Internally, we use the fd in our own private space   
    */
   _ups->fd = 1;

   _ups->clear_slave();
   write_unlock(_ups);
   return true;
}

bool GenericUsbDriver::SubclassClose()
{
   /* Should we be politely closing fds here or anything? */
   return true;
}

bool GenericUsbDriver::SubclassReadIntFromUps(int ci, int *value)
{
   usb_value val;

   if (!SubclassGetValue(ci, &val))
      return false;

   *value = val.iValue;
   return true;
}

bool GenericUsbDriver::SubclassWriteIntToUps(int ci, int value, const char *name)
{
   usb_info *info;
   int old_value, new_value;
   unsigned char rpt[20];

   if (_ups->UPS_Cap[ci] && _info[ci]) {
      info = _info[ci];    /* point to our info structure */

      if (hidu_get_report(_fd, &info->item, rpt, info->report_len) < 1) {
         Dmsg1(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      old_value = hid_get_data(rpt + 1, &info->item);

      hid_set_data(rpt + 1, &info->item, value);

      if (!hidu_set_report(_fd, &info->item, rpt, info->report_len)) {
         Dmsg1(000, "set_report for kill power function %s failed.\n", name);
         return false;
      }

      if (hidu_get_report(_fd, &info->item, rpt, info->report_len) < 1) {
         Dmsg1(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      new_value = hid_get_data(rpt + 1, &info->item);

      Dmsg3(100, "function %s ci=%d value=%d OK.\n", name, ci, value);
      Dmsg4(100, "%s before=%d set=%d after=%d\n", name, old_value, value, new_value);
      return true;
   }

   Dmsg2(000, "function %s ci=%d not available in this UPS.\n", name, ci);
   return false;
}
