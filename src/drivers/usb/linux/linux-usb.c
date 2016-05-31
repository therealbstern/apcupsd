/*
 * linux-usb.c
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

/*
 * The following is a work around for a problem in 2.6 kernel
 *  linux/hiddev.h file that is fixed in 2.6.9
 */
#define HID_MAX_USAGES 1024

#include "apc.h"
#include "linux-usb.h"
#include <glob.h>
#include <linux/usbdevice_fs.h>

/* RHEL has an out-of-date hiddev.h */
#ifndef HIDIOCGFLAG
# define HIDIOCSFLAG       _IOW('H', 0x0F, int)
#endif
#ifndef HIDDEV_FLAG_UREF
# define HIDDEV_FLAG_UREF  0x1
#endif

/* Enable this to force Linux 2.4 compatability mode */
#define FORCE_COMPAT24  false

UpsDriver *UsbUpsDriver::Factory(UPSINFO *ups)
{
   return new LinuxUsbUpsDriver(ups);
}

LinuxUsbUpsDriver::LinuxUsbUpsDriver(UPSINFO *ups) :
   UsbUpsDriver(ups),
   _fd(-1),
   _compat24(false),
   _linkcheck(false)
{
   memset(_orig_device, 0, sizeof(_orig_device));
   memset(_info, 0, sizeof(_info));
}

void LinuxUsbUpsDriver::reinitialize_private_structure()
{
   int k;

   Dmsg(200, "Reinitializing private structure.\n");
   /*
    * We are being reinitialized, so clear the Cap
    * array, and release previously allocated memory.
    */
   for (k = 0; k <= CI_MAXCI; k++) {
      _ups->UPS_Cap[k] = false;
      if (_info[k] != NULL) {
         free(_info[k]);
         _info[k] = NULL;
      }
   }
}

/*
 * Internal routine to attempt device open.
 */
int LinuxUsbUpsDriver::open_device(const char *dev)
{
   int flaguref = HIDDEV_FLAG_UREF;
   int fd, ret, i;

   Dmsg(200, "Attempting to open \"%s\"\n", dev);

   /* Open the device port */
   fd = open(dev, O_RDWR | O_NOCTTY | O_CLOEXEC);
   if (fd >= 0) {
      /* Check for the UPS application HID usage */
      for (i = 0; (ret = ioctl(fd, HIDIOCAPPLICATION, i)) > 0; i++) {
         if ((ret & 0xffff000) == (UPS_USAGE & 0xffff0000)) {
            /* Request full uref reporting from read() */
            if (FORCE_COMPAT24 || ioctl(fd, HIDIOCSFLAG, &flaguref)) {
               Dmsg(100, "HIDIOCSFLAG failed; enabling linux-2.4 "
                      "compatibility mode\n");
               _compat24 = true;
            }
            /* Successfully opened the device */
            Dmsg(200, "Successfully opened \"%s\"\n", dev);
            return fd;
         }
      }
      close(fd);
   }

   /* Failed to open the device */
   return -1;
}

/*
 * Routine to request that kernel driver attach to any APC USB UPSes
 */
void LinuxUsbUpsDriver::bind_upses()
{
#ifdef USBDEVFS_CONNECT
   // Find all USB devices in usbfs
   glob_t g;
   if (glob("/proc/bus/usb/[0-9][0-9][0-9]/[0-9][0-9][0-9]", 0, NULL, &g) &&
      glob("/dev/bus/usb/[0-9][0-9][0-9]/[0-9][0-9][0-9]", 0, NULL, &g))
   {
      return;
   }

   // Iterate over all USB devices...
   for (size_t i = 0; i < g.gl_pathc; ++i)
   {
      // Open the device in usbfs
      int fd = open(g.gl_pathv[i], O_RDWR|O_CLOEXEC);
      if (fd == -1)
         continue;

      struct usb_device_descriptor
      {
         uint8_t  bLength;
         uint8_t  bDescriptorType;
         uint16_t bcdUSB;
         uint8_t  bDeviceClass;
         uint8_t  bDeviceSubClass;
         uint8_t  bDeviceProtocol;
         uint8_t  bMaxPacketSize0;
         uint16_t idVendor;
         uint16_t idProduct;
         uint16_t bcdDevice;
         uint8_t  iManufacturer;
         uint8_t  iProduct;
         uint8_t  iSerialNumber;
         uint8_t  bNumConfigurations;
      } __attribute__ ((packed)) dd;

      // Fetch device descriptor, then verify device VID/PID are supported and
      // no kernel driver is currently attached
      struct usbdevfs_getdriver getdrv = {0};
      if (read(fd, &dd, sizeof(dd)) == sizeof(dd) && 
          MatchVidPid(dd.idVendor, dd.idProduct) &&
          ioctl(fd, USBDEVFS_GETDRIVER, &getdrv))
      {
         // APC device with no kernel driver attached
         // Issue command to attach kernel driver
         struct usbdevfs_ioctl command = {0};
         command.ioctl_code = USBDEVFS_CONNECT;
         int rc = ioctl(fd, USBDEVFS_IOCTL, &command);
         // Hard to tell if this succeeded because the return value of the
         // ioctl was changed. In linux 2.4.37, 0 means success but in 3.4 it
         // means failure. So...we're screwed. The only thing we know for sure
         // is <0 is definitely failure.
         if (rc >= 0)
         {
            Dmsg(100, "Reattached kernel driver to %s\n", g.gl_pathv[i]);
         }
         else
         {
            Dmsg(0, "Failed to attach kernel driver to %s (%d)\n", 
               g.gl_pathv[i], rc);
         }
      }

      close(fd);
   }

   globfree(&g);
#endif // USBDEVFS_CONNECT
}

/*
 * Internal routine to open the device and ensure that there is
 * a UPS application on the line.  This routine may be called
 * many times because the device may be unplugged and plugged
 * back in -- the joys of USB devices.
 */
bool LinuxUsbUpsDriver::open_usb_device()
{
   char devname[MAXSTRING];
   const char *hiddev[] =
      { "/dev/usb/hiddev", "/dev/usb/hid/hiddev", "/dev/hiddev", NULL };
   int i, j, k;

   /*
    * Ensure any APC UPSes are utilizing the usbhid/hiddev kernel driver.
    * This is necessary if they were detached from the kernel driver in order
    * to use libusb (the apcupsd 'generic' USB driver).
    */
   bind_upses();

   /*
    * Note, we set _ups->fd here so the "core" of apcupsd doesn't
    * think we are a slave, which is what happens when it is -1.
    * (ADK: Actually this only appears to be true for apctest as
    * apcupsd proper uses the UPS_slave flag.)
    * Internally, we use the fd in our own private space   
    */
   _ups->fd = 1;

   /*
    * If no device locating specified, we go autodetect it
    * by searching known places.
    */
   if (_ups->device[0] == 0)
      goto auto_detect;

   /*
    * Also if specified device includes deprecated '[]' notation,
    * just use the automatic search.
    */
   if (strchr(_ups->device, '[') &&
       strchr(_ups->device, ']'))
   {
      _orig_device[0] = 0;
      goto auto_detect;
   }

   /*
    * If we get here we know the user specified a device or we are
    * trying to re-open a device that previously was open.
    */
 
   /* Retry 10 times */
   for (i = 0; i < 10; i++) {
      _fd = open_device(_ups->device);
      if (_fd != -1)
         return true;
      sleep(1);
   }

   /*
    * If user-specified device could not be opened, fail.
    */
   if (_orig_device[0] != 0)
      return false;

   /*
    * If we get here we failed to re-open a previously auto-detected
    * device. We will fall thru and restart autodetection...
    */

auto_detect:
 
   for (i = 0; i < 10; i++) {           /* try 10 times */
      for (j = 0; hiddev[j]; j++) {     /* loop over known device names */
         for (k = 0; k < 16; k++) {     /* loop over devices */
            asnprintf(devname, sizeof(devname), "%s%d", hiddev[j], k);
            _fd = open_device(devname);
            if (_fd != -1) {
               /* Successful open, save device name and return */
               strlcpy(_ups->device, devname, sizeof(_ups->device));
               return true;
            }
         }
      }
      sleep(1);
   }

   _ups->device[0] = '\0';
   return false;
}

/* 
 * Called if there is an ioctl() or read() error, we close() and
 * re open() the port since the device was probably unplugged.
 */
bool LinuxUsbUpsDriver::usb_link_check()
{
   bool comm_err = true;
   int tlog;
   bool once = true;

   if (_linkcheck)
      return false;

   _linkcheck = true;               /* prevent recursion */

   _ups->set_commlost();
   Dmsg(200, "link_check comm lost\n");

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
      if (_fd >= 0) {
         close(_fd);
         _fd = -1;
         reinitialize_private_structure();
      }

      if (open_usb_device() && get_capabilities() &&
         read_static_data()) {
         comm_err = false;
      } else {
         continue;
      }
   }

   if (!comm_err) {
      generate_event(_ups, CMDCOMMOK);
      _ups->clear_commlost();
      Dmsg(200, "link check comm OK.\n");
   }

   _linkcheck = false;
   return true;
}

bool LinuxUsbUpsDriver::populate_uval(USB_INFO *info, USB_VALUE *uval)
{
   struct hiddev_string_descriptor sdesc;
   USB_VALUE val;
   int exponent;

   exponent = info->unit_exponent;
   if (exponent > 7)
      exponent = exponent - 16;

   if (info->data_type == T_INDEX) {    /* get string */
      if (info->uref.value == 0)
         return false;

      sdesc.index = info->uref.value;
      if (ioctl(_fd, HIDIOCGSTRING, &sdesc) < 0)
         return false;

      strlcpy(val.sValue, sdesc.value, sizeof(val.sValue));
      val.value_type = V_STRING;

      Dmsg(200, "Def val=%d exp=%d sVal=\"%s\" ci=%d\n", info->uref.value,
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
         break;
      default:
         val.UnitName = "";
         val.value_type = V_INTEGER;
         val.iValue = info->uref.value;
         break;
      }

      if (exponent == 0)
         val.dValue = info->uref.value;
      else
         val.dValue = ((double)info->uref.value) * pow_ten(exponent);

      // Store a (possibly truncated) copy of the floating point value in the
      // integer field as well.
      val.iValue = (int)val.dValue;

      Dmsg(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->uref.value,
         exponent, val.dValue, info->ci);
   } else {                        /* should be T_NONE */
      val.UnitName = "";
      val.value_type = V_INTEGER;
      val.iValue = info->uref.value;

      if (exponent == 0)
         val.dValue = info->uref.value;
      else
         val.dValue = ((double)info->uref.value) * pow_ten(exponent);

      Dmsg(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->uref.value,
         exponent, val.dValue, info->ci);
   }

   memcpy(uval, &val, sizeof(*uval));
   return true;
}

/*
 * Get a field value
 */
bool LinuxUsbUpsDriver::pusb_get_value(int ci, USB_VALUE *uval)
{
   struct hiddev_report_info rinfo;
   USB_INFO *info;

   if (!_ups->UPS_Cap[ci] || !_info[ci])
      return false;                /* UPS does not have capability */

   /* Fetch the new value from the UPS */

   info = _info[ci];       /* point to our info structure */
   rinfo.report_type = info->uref.report_type;
   rinfo.report_id = info->uref.report_id;
   if (ioctl(_fd, HIDIOCGREPORT, &rinfo) < 0)   /* update Report */
      return false;

   if (ioctl(_fd, HIDIOCGUSAGE, &info->uref) < 0)       /* update UPS value */
      return false;

   /* Process the updated value */
   return populate_uval(info, uval);
}

/*
 * Find the USB_INFO structure used for tracking a given usage. Searching
 * by usage_code alone is insufficient since the same usage may appear in
 * multiple reports or even multiple times in the same report.
 */
LinuxUsbUpsDriver::USB_INFO *LinuxUsbUpsDriver::find_info_by_uref(
   struct hiddev_usage_ref *uref)
{
   int i;

   for (i=0; i<CI_MAXCI; i++) {
      if (_ups->UPS_Cap[i] && _info[i] &&
          _info[i]->uref.report_id == uref->report_id &&
          _info[i]->uref.field_index == uref->field_index &&
          _info[i]->uref.usage_index == uref->usage_index &&
          _info[i]->uref.usage_code == uref->usage_code) {
            return _info[i];
      }
   }

   return NULL;
}

/*
 * Same as find_info_by_uref() but only checks the usage code. This is
 * not entirely reliable, but it's the best we have on linux-2.4.
 */
LinuxUsbUpsDriver::USB_INFO *LinuxUsbUpsDriver::find_info_by_ucode(
   unsigned int ucode)
{
   int i;

   for (i=0; i<CI_MAXCI; i++) {
      if (_ups->UPS_Cap[i] && _info[i] &&
          _info[i]->uref.usage_code == ucode) {
            return _info[i];
      }
   }

   return NULL;
}

/*
 * Read UPS events. I.e. state changes.
 */
bool LinuxUsbUpsDriver::check_state()
{
   int retval;
   bool done = false;
   struct hiddev_usage_ref uref;
   struct hiddev_event hev;
   USB_INFO* info;
   USB_VALUE uval;

   struct timeval tv;
   tv.tv_sec = _ups->wait_time;
   tv.tv_usec = 0;

   while (!done) {
      fd_set rfds;

      FD_ZERO(&rfds);
      FD_SET(_fd, &rfds);

      retval = select(_fd + 1, &rfds, NULL, NULL, &tv);

      /*
       * Note: We are relying on select() adjusting tv to the amount
       * of time NOT waited. This is a Linux-specific feature but
       * shouldn't be a problem since the driver is only intended for
       * for Linux.
       */

      switch (retval) {
      case 0:                     /* No chars available in TIMER seconds. */
         return false;
      case -1:
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;

         Dmsg(200, "select error: ERR=%s\n", strerror(errno));
         usb_link_check();      /* link is down, wait */
         return false;
      default:
         break;
      }

      if (!_compat24) {
         /* This is >= linux-2.6, so we can read a uref directly */
         do {
            retval = read(_fd, &uref, sizeof(uref));
         } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

         if (retval < 0) {            /* error */
            Dmsg(200, "read error: ERR=%s\n", strerror(errno));
            usb_link_check();      /* notify that link is down, wait */
            return false;
         }

         if (retval == 0 || retval < (int)sizeof(uref))
            return false;

         /* Ignore events we don't recognize */
         if ((info = find_info_by_uref(&uref)) == NULL) {
            Dmsg(200, "Unrecognized usage (rpt=%d, usg=0x%08x, val=%d)\n",
               uref.report_id, uref.usage_code, uref.value);
            continue;
         }
      } else {
         /*
          * We're in linux-2.4 compatibility mode, so we read a
          * hiddev_event and use it to construct a uref.
          */
         do {
            retval = read(_fd, &hev, sizeof(hev));
         } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

         if (retval < 0) {            /* error */
            Dmsg(200, "read error: ERR=%s\n", strerror(errno));
            usb_link_check();      /* notify that link is down, wait */
            return false;
         }

         if (retval == 0 || retval < (int)sizeof(hev))
            return false;

         /* Ignore events we don't recognize */
         if ((info = find_info_by_ucode(hev.hid)) == NULL) {
            Dmsg(200, "Unrecognized usage (usg=0x%08x, val=%d)\n",
               hev.hid, hev.value);
            continue;
         }

         /*
          * ADK FIXME: The info-> struct we have now is not guaranteed to
          * actually be the right one, because linux-2.4 does not give us
          * enough data in the event to make a positive match. We may need
          * to filter out ambiguous usages here or manually fetch each CI
          * that matches the given usage.
          */

         /* Copy the stored uref, replacing its value */
         uref = info->uref;
         uref.value = hev.value;
      }

      write_lock(_ups);

      /* Ignore events whose value is unchanged */
      if (info->uref.value == uref.value) {
         Dmsg(200, "Ignoring unchanged value (rpt=%d, usg=0x%08x, val=%d)\n",
            uref.report_id, uref.usage_code, uref.value);
         write_unlock(_ups);
         continue;
      }

      /* Update tracked value */
      Dmsg(200, "Processing changed value (rpt=%d, usg=0x%08x, val=%d)\n",
         uref.report_id, uref.usage_code, uref.value);
      info->uref.value = uref.value;

      /* Populate a uval and report it to the upper layer */
      populate_uval(info, &uval);
      if (usb_report_event(info->ci, &uval)) {
         /*
          * The upper layer considers this an important event,
          * so we will return immediately.
          */
         done = true;
      }

      write_unlock(_ups);
   }

   return true;
}

/*
 * Open usb port
 * This is called once by the core code and is the first routine
 * called.
 */
bool LinuxUsbUpsDriver::Open()
{
   write_lock(_ups);

   if (_orig_device[0] == 0)
      strlcpy(_orig_device, _ups->device, sizeof(_orig_device));

   bool rc = open_usb_device();

   _ups->clear_slave();
   write_unlock(_ups);
   return rc;
}

/*
 * This is the last routine called from apcupsd core code 
 */
bool LinuxUsbUpsDriver::Close()
{
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
bool LinuxUsbUpsDriver::pusb_ups_get_capabilities()
{
   int rtype[2] = { HID_REPORT_TYPE_FEATURE, HID_REPORT_TYPE_INPUT };
   struct hiddev_report_info rinfo;
   struct hiddev_field_info finfo;
   struct hiddev_usage_ref uref;
   unsigned int i, j, k, n;

   if (ioctl(_fd, HIDIOCINITREPORT, 0) < 0)
      Error_abort("Cannot init USB HID report. ERR=%s\n", strerror(errno));

   write_lock(_ups);

   /*
    * Walk through all available reports and determine
    * what information we can use.
    */
   for (n = 0; n < sizeof(rtype)/sizeof(*rtype); n++) {
      rinfo.report_type = rtype[n];
      rinfo.report_id = HID_REPORT_ID_FIRST;

      while (ioctl(_fd, HIDIOCGREPORTINFO, &rinfo) >= 0) {
         for (i = 0; i < rinfo.num_fields; i++) {
            memset(&finfo, 0, sizeof(finfo));
            finfo.report_type = rinfo.report_type;
            finfo.report_id = rinfo.report_id;
            finfo.field_index = i;

            if (ioctl(_fd, HIDIOCGFIELDINFO, &finfo) < 0)
               continue;

            memset(&uref, 0, sizeof(uref));
            for (j = 0; j < finfo.maxusage; j++) {
               uref.report_type = finfo.report_type;
               uref.report_id = finfo.report_id;
               uref.field_index = i;
               uref.usage_index = j;

               if (ioctl(_fd, HIDIOCGUCODE, &uref) < 0)
                  continue;

               if (ioctl(_fd, HIDIOCGUSAGE, &uref) < 0)
                  continue;

               /*
                * We've got a UPS usage entry, now walk down our
                * know_info table and see if we have a match. If so,
                * allocate a new entry for it.
                */
               for (k = 0; _known_info[k].usage_code; k++) {
                  USB_INFO *info;
                  int ci = _known_info[k].ci;

                  if (ci != CI_NONE &&
                      uref.usage_code == _known_info[k].usage_code &&
                      (_known_info[k].physical == P_ANY ||
                         _known_info[k].physical == finfo.physical) &&
                      (_known_info[k].logical == P_ANY ||
                         _known_info[k].logical == finfo.logical)) {

                     // If we do not have any data saved for this report yet,
                     // allocate an USB_INFO and populate the read uref.
                     info = _info[ci];
                     if (!info) {
                        _ups->UPS_Cap[ci] = true;
                        info = (USB_INFO *)malloc(sizeof(USB_INFO));

                        if (!info) {
                           write_unlock(_ups);
                           Error_abort("Out of memory.\n");
                        }

                        _info[ci] = info;
                        memset(info, 0, sizeof(*info));
                        info->ci = ci;
                        info->physical = finfo.physical;
                        info->unit_exponent = finfo.unit_exponent;
                        info->unit = finfo.unit;
                        info->data_type = _known_info[k].data_type;
                        memcpy(&info->uref, &uref, sizeof(uref));

                        Dmsg(200, "Got READ ci=%d, usage=0x%x, rpt=%d\n",
                           ci, _known_info[k].usage_code, uref.report_id);
                     }

                     // If this is a FEATURE report and we haven't set the
                     // write uref yet, place it in the write uref (possibly in
                     // addition to placing it in the read uref above).
                     if (rinfo.report_type == HID_REPORT_TYPE_FEATURE &&
                         info->wuref.report_id == 0) {
                        memcpy(&info->wuref, &uref, sizeof(uref));

                        Dmsg(200, "Got WRITE ci=%d, usage=0x%x, rpt=%d\n",
                           ci, _known_info[k].usage_code, uref.report_id);
                     }

                     break;
                  }
               }
            }
         }
         rinfo.report_id |= HID_REPORT_ID_NEXT;
      }
   }

   _ups->UPS_Cap[CI_STATUS] = true; /* we have status flag */
   write_unlock(_ups);
   return 1;
}


int LinuxUsbUpsDriver::read_int_from_ups(int ci, int *value)
{
   USB_VALUE val;

   if (!pusb_get_value(ci, &val))
      return false;

   *value = val.iValue;
   return true;
}

int LinuxUsbUpsDriver::write_int_to_ups(int ci, int value, const char *name)
{
   struct hiddev_report_info rinfo;
   USB_INFO *info;
   int old_value, new_value;

   // Make sure we have a writable uref for this CI
   if (_ups->UPS_Cap[ci] && _info[ci] &&
      _info[ci]->wuref.report_id) {
      info = _info[ci];    /* point to our info structure */
      rinfo.report_type = info->uref.report_type;
      rinfo.report_id = info->uref.report_id;

      /* Get report */
      if (ioctl(_fd, HIDIOCGREPORT, &rinfo) < 0) {
         Dmsg(000, "HIDIOCGREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Get UPS value */
      if (ioctl(_fd, HIDIOCGUSAGE, &info->wuref) < 0) {
         Dmsg(000, "HIDIOCGUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }
      old_value = info->uref.value;

      info->wuref.value = value;
      rinfo.report_type = info->wuref.report_type;
      rinfo.report_id = info->wuref.report_id;
      Dmsg(100, "SUSAGE type=%d id=%d index=%d\n", info->wuref.report_type,
         info->wuref.report_id, info->wuref.field_index);

      /* Update UPS value */
      if (ioctl(_fd, HIDIOCSUSAGE, &info->wuref) < 0) {
         Dmsg(000, "HIDIOCSUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Update Report */
      if (ioctl(_fd, HIDIOCSREPORT, &rinfo) < 0) {
         Dmsg(000, "HIDIOCSREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /*
       * This readback of the report is NOT just for debugging. It
       * has the effect of flushing the above SET_REPORT to the 
       * device, which is important since we need to make sure it
       * happens before subsequent reports are sent.
       */

      rinfo.report_type = info->uref.report_type;
      rinfo.report_id = info->uref.report_id;

      /* Get report */
      if (ioctl(_fd, HIDIOCGREPORT, &rinfo) < 0) {
         Dmsg(000, "HIDIOCGREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Get UPS value */
      if (ioctl(_fd, HIDIOCGUSAGE, &info->uref) < 0) {
         Dmsg(000, "HIDIOCGUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      new_value = info->uref.value;
      Dmsg(100, "function %s ci=%d value=%d OK.\n", name, ci, value);
      Dmsg(100, "%s before=%d set=%d after=%d\n", name, old_value, value,
         new_value);

      return true;
   }

   Dmsg(000, "function %s ci=%d not available in this UPS.\n", name, ci);
   return false;
}
