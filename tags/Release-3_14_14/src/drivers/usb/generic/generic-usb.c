/*
 * generic-usb.c
 *
 * Generic USB module for libusb.
 */

/*
 * Copyright (C) 2005 Adam Kropelin
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
#include "astring.h"
#include "generic-usb.h"

UpsDriver *UsbUpsDriver::Factory(UPSINFO *ups)
{
   return new GenericUsbUpsDriver(ups);
}

GenericUsbUpsDriver::GenericUsbUpsDriver(UPSINFO *ups) :
   UsbUpsDriver(ups),
   _linkcheck(false)
{
   memset(_info, 0, sizeof(_info));
}

bool GenericUsbUpsDriver::pusb_ups_get_capabilities()
{
   int i, input, feature, ci, phys, logi;
   hid_item_t input_item, feature_item, item;
   USB_INFO *info;

   write_lock(_ups);

   for (i = 0; _known_info[i].usage_code; i++) {
      ci = _known_info[i].ci;
      phys = _known_info[i].physical;
      logi = _known_info[i].logical;

      if (ci != CI_NONE && !_info[ci]) {

         /* Try to find an INPUT report containing this usage */
         input = _hidups.LocateItem(
            _known_info[i].usage_code,     /* Match usage code */
            -1,                           /* Don't care about application */
            (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
            (logi == P_ANY) ? -1 : logi,  /* Match logical usage */
            HID_KIND_INPUT,               /* Match feature type */
            &input_item);

         /* Try to find a FEATURE report containing this usage */
         feature = _hidups.LocateItem(
            _known_info[i].usage_code,     /* Match usage code */
            -1,                           /* Don't care about application */
            (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
            (logi == P_ANY) ? -1 : logi,  /* Match logical usage */
            HID_KIND_FEATURE,             /* Match feature type */
            &feature_item);

         /*
          * Choose which report to use. We prefer FEATURE since some UPSes
          * have broken INPUT reports, but we will fall back on INPUT if
          * FEATURE is not available.
          */
         if (feature)
            item = feature_item;
         else if (input)
            item = input_item;
         else
            continue; // No valid report, bail

         _ups->UPS_Cap[ci] = true;
         _ups->UPS_Cmd[ci] = _known_info[i].usage_code;

         info = (USB_INFO *)malloc(sizeof(USB_INFO));
         if (!info) {
            write_unlock(_ups);
            Error_abort("Out of memory.\n");
         }

         // Populate READ report data
         _info[ci] = info;
         memset(info, 0, sizeof(*info));
         info->ci = ci;
         info->usage_code = item.usage;
         info->unit_exponent = item.unit_exponent;
         info->unit = item.unit;
         info->data_type = _known_info[i].data_type;
         info->item = item;
         info->report_len = _hidups.GetReportSize( /* +1 for report id */
            item.kind, item.report_ID) + 1;
         Dmsg(200, "Got READ ci=%d, rpt=%d (len=%d), usage=0x%x (len=%d), kind=0x%02x\n",
            ci, item.report_ID, info->report_len,
            _known_info[i].usage_code, item.report_size, item.kind);

         // If we have a FEATURE report, use that as the writable report
         if (feature) {
            info->witem = item;
            Dmsg(200, "Got WRITE ci=%d, rpt=%d (len=%d), usage=0x%x (len=%d), kind=0x%02x\n",
               ci, item.report_ID, info->report_len,
               _known_info[i].usage_code, item.report_size, item.kind);               
         }
      }
   }

   _ups->UPS_Cap[CI_STATUS] = true; /* we always have status flag */
   write_unlock(_ups);
   return 1;
}

bool GenericUsbUpsDriver::populate_uval(USB_INFO *info, unsigned char *data, USB_VALUE *uval)
{
   const char *str;
   int exponent;
   USB_VALUE val;

   /* data+1 skips the report tag byte */
   info->value = hid_get_data(data+1, &info->item);

   exponent = info->unit_exponent;
   if (exponent > 7)
      exponent = exponent - 16;

   if (info->data_type == T_INDEX) {    /* get string */
      if (info->value == 0)
         return false;

      str = _hidups.GetString(info->value);
      if (!str)
         return false;

      strlcpy(val.sValue, str, sizeof(val.sValue));
      val.value_type = V_STRING;

      Dmsg(200, "Def val=%d exp=%d sVal=\"%s\" ci=%d\n", info->value,
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

      // Store a (possibly truncated) copy of the floating point value in the
      // integer field as well.
      val.iValue = (int)val.dValue;

      Dmsg(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   } else {                        /* should be T_NONE */

      val.UnitName = "";
      val.value_type = V_INTEGER;
      val.iValue = info->value;

      if (exponent == 0)
         val.dValue = info->value;
      else
         val.dValue = ((double)info->value) * pow_ten(exponent);

      Dmsg(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   }

   memcpy(uval, &val, sizeof(*uval));
   return true;   
}

/*
 * Get a field value
 */
bool GenericUsbUpsDriver::pusb_get_value(int ci, USB_VALUE *uval)
{
   USB_INFO *info = _info[ci];
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
   len = _hidups.GetReport(&info->item, data, info->report_len);
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
      Dmsg(100, "Report length mismatch, fixing "
         "(id=%d, ci=%d, expected=%d, actual=%d)\n",
         info->item.report_ID, ci, info->report_len, len);
      info->report_len = len;
   }

   /* Populate a uval struct using the raw report data */
   return populate_uval(info, data, uval);
}

void GenericUsbUpsDriver::reinitialize_private_structure()
{
   int k;

   Dmsg(200, "Reinitializing private structure.\n");
   /*
    * We are being reinitialized, so clear the Cap
    *   array, and release previously allocated memory.
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
 * Called if there is an ioctl() or read() error, we close() and
 * re open() the port since the device was probably unplugged.
 */
bool GenericUsbUpsDriver::usb_link_check()
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

      _hidups.Close();
      reinitialize_private_structure();

      if (_hidups.Open(_ups->device) && get_capabilities() && read_static_data()) {
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

bool GenericUsbUpsDriver::check_state()
{
   int i, ci;
   int retval, value;
   unsigned char buf[20];
   struct timeval now, exit;
   int timeout;
   USB_VALUE uval;
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
         return 0;
      }

      Dmsg(200, "Timeout=%d\n", timeout);
      retval = _hidups.InterruptRead(USB_ENDPOINT_IN|1, (char*)buf, sizeof(buf), timeout);

      if (retval == 0 || retval == -ETIMEDOUT) {
         /* No events available in _ups->wait_time seconds. */
         return 0;
      } else if (retval == -EINTR || retval == -EAGAIN) {
         /* assume SIGCHLD */
         continue;
      } else if (retval < 0) {
         /* Hard error */
         Dmsg(200, "usb_interrupt_read error: (%d) %s\n", retval, strerror(-retval));
         usb_link_check();      /* link is down, wait */
         return 0;
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

            /*
             * Check if we received fewer bytes of data from the UPS than we
             * should have. If so, ignore the report since we can't process it
             * reliably. If we go ahead and try to process it we may get 
             * sporradic bad readings. UPSes we've seen this issue on so far 
             * include:
             *
             *    "Back-UPS CS 650 FW:817.v7 .I USB FW:v7"
             *    "Back-UPS CS 500 FW:808.q8.I USB FW:q8"
             */
            if (_info[ci]->report_len != retval) {
               Dmsg(100, "Report length mismatch, ignoring "
                  "(id=%d, ci=%d, expected=%d, actual=%d)\n",
                  _info[ci]->item.report_ID, ci, 
                  _info[ci]->report_len, retval);
               break; /* don't continue since other CIs will be just as wrong */
            }

            /* Ignore this event if the value has not changed */
            value = hid_get_data(buf+1, &_info[ci]->item);
            if (_info[ci]->value == value) {
               Dmsg(200, "Ignoring unchanged value (ci=%d, rpt=%d, val=%d)\n",
                  ci, buf[0], value);
               continue;
            }

            Dmsg(200, "Processing changed value (ci=%d, rpt=%d, val=%d)\n",
               ci, buf[0], value);

            /* Populate a uval and report it to the upper layer */
            populate_uval(_info[ci], buf, &uval);
            if (usb_report_event(ci, &uval)) {
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
bool GenericUsbUpsDriver::Open()
{
   write_lock(_ups);

   bool rc = _hidups.Open(_ups->device);

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
   return rc;
}

bool GenericUsbUpsDriver::Close()
{
   _hidups.Close();
   return 1;
}

int GenericUsbUpsDriver::read_int_from_ups(int ci, int *value)
{
   USB_VALUE val;

   if (!pusb_get_value(ci, &val))
      return false;

   *value = val.iValue;
   return true;
}

int GenericUsbUpsDriver::write_int_to_ups(int ci, int value, const char *name)
{
   USB_INFO *info;
   int old_value, new_value;
   unsigned char rpt[20];

   if (_ups->UPS_Cap[ci] && _info[ci] && _info[ci]->witem.report_ID) {
      info = _info[ci];    /* point to our info structure */

      if (_hidups.GetReport(&info->item, rpt, info->report_len) < 1) {
         Dmsg(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      old_value = hid_get_data(rpt + 1, &info->item);

      hid_set_data(rpt + 1, &info->witem, value);

      if (!_hidups.SetReport(&info->witem, rpt, info->report_len)) {
         Dmsg(000, "set_report for kill power function %s failed.\n", name);
         return false;
      }

      if (_hidups.GetReport(&info->item, rpt, info->report_len) < 1) {
         Dmsg(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      new_value = hid_get_data(rpt + 1, &info->item);

      Dmsg(100, "function %s ci=%d value=%d OK.\n", name, ci, value);
      Dmsg(100, "%s before=%d set=%d after=%d\n", name, old_value, value, new_value);
      return true;
   }

   Dmsg(000, "function %s ci=%d not available in this UPS.\n", name, ci);
   return false;
}
