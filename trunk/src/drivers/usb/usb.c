/*
 * usb.c
 *
 * Public driver interface for all platform USB drivers.
 *
 * Based on linux-usb.c by Kern Sibbald
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

#include "apc.h"
#include "usb.h"
#include "usb_common.h"
#include <math.h>

/* Implemented in platform-specific code */
int pusb_ups_get_capabilities(UPSINFO *ups, const struct s_known_info *known_info);
int pusb_ups_open(UPSINFO *ups);
int pusb_ups_close(UPSINFO *ups);
int pusb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval);
int pusb_ups_check_state(UPSINFO *ups, unsigned int timeout_msec, int *ci, USB_VALUE *uval);
int pusb_ups_setup(UPSINFO *ups);
int pusb_write_int_to_ups(UPSINFO *ups, int ci, int value, char *name);
int pusb_read_int_from_ups(UPSINFO *ups, int ci, int *value);

/* Private helpers */
static void usb_process_value(UPSINFO* ups, int ci, USB_VALUE* uval);
static int usb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval);

/*
 * This table is used when walking through the USB reports to see
 * what information found in the UPS that we want. If the usage_code 
 * and the physical code match, then we make an entry in the command
 * index table containing the usage information provided by the UPS
 * as well as the data type from this table. Entries in the table 
 * with ci == CI_NONE are not used, for the moment, they are
 * retained just so they are not forgotten.
 */
const struct s_known_info known_info[] = {
   /*  Page 0x84 is the Power Device Page */
   /* CI                        USAGE       PHYS       TYPE        VOLATILE? */
   {CI_NONE,                    0x00840001, P_ANY,     T_INDEX,    false},  /* iName */
   {CI_VLINE,                   0x00840030, P_INPUT,   T_UNITS,    true },  /* Line Voltage */
   {CI_VOUT,                    0x00840030, P_OUTPUT,  T_UNITS,    true },  /* Output Voltage */
   {CI_VBATT,                   0x00840030, P_BATTERY, T_UNITS,    true },  /* Battery Voltage */
   {CI_NONE,                    0x00840031, P_ANY,     T_UNITS,    false},  /* Current */
   {CI_FREQ,                    0x00840032, P_OUTPUT,  T_UNITS,    true },  /* Frequency */
   {CI_NONE,                    0x00840033, P_ANY,     T_UNITS,    false},  /* ApparentPower */
   {CI_NONE,                    0x00840034, P_ANY,     T_UNITS,    false},  /* ActivePower */
   {CI_LOAD,                    0x00840035, P_ANY,     T_UNITS,    true },  /* PercentLoad */
   {CI_ITEMP,                   0x00840036, P_BATTERY, T_UNITS,    true },  /* Temperature */
   {CI_HUMID,                   0x00840037, P_ANY,     T_UNITS,    true },  /* Humidity */
   {CI_NOMBATTV,                0x00840040, P_ANY,     T_UNITS,    false},  /* ConfigVoltage */
   {CI_NONE,                    0x00840042, P_ANY,     T_UNITS,    false},  /* ConfigFrequency */
   {CI_NONE,                    0x00840043, P_ANY,     T_UNITS,    false},  /* ConfigApparentPower */
   {CI_LTRANS,                  0x00840053, P_ANY,     T_UNITS,    false},  /* LowVoltageTransfer */
   {CI_HTRANS,                  0x00840054, P_ANY,     T_UNITS,    false},  /* HighVoltageTransfer */
   {CI_DelayBeforeReboot,       0x00840055, P_ANY,     T_UNITS,    false},  /* DelayBeforeReboot */
   {CI_DWAKE,                   0x00840056, P_ANY,     T_UNITS,    false},  /* DelayBeforeStartup */
   {CI_DelayBeforeShutdown,     0x00840057, P_ANY,     T_UNITS,    false},  /* DelayBeforeShutdown */
   {CI_ST_STAT,                 0x00840058, P_ANY,     T_NONE,     false},  /* Test */
   {CI_DALARM,                  0x0084005a, P_ANY,     T_NONE,     true },  /* AudibleAlarmControl */
   {CI_NONE,                    0x00840061, P_ANY,     T_NONE,     false},  /* Good */
   {CI_IFailure,                0x00840062, P_ANY,     T_NONE,     false},  /* InternalFailure */
   {CI_PWVoltageOOR,            0x00840063, P_ANY,     T_NONE,     false},  /* Volt out-of-range */
   {CI_PWFrequencyOOR,          0x00840064, P_ANY,     T_NONE,     false},  /* Freq out-of-range */
   {CI_Overload,                0x00840065, P_ANY,     T_NONE,     true },  /* Overload */
   {CI_OverCharged,             0x00840066, P_ANY,     T_NONE,     false},  /* Overcharged */
   {CI_OverTemp,                0x00840067, P_ANY,     T_NONE,     false},  /* Overtemp */
   {CI_ShutdownRequested,       0x00840068, P_ANY,     T_NONE,     false},  /* ShutdownRequested */
   {CI_ShutdownImminent,        0x00840069, P_ANY,     T_NONE,     true },  /* ShutdownImminent */
   {CI_NONE,                    0x0084006b, P_ANY,     T_NONE,     false},  /* Switch On/Off */
   {CI_NONE,                    0x0084006c, P_ANY,     T_NONE,     false},  /* Switchable */
   {CI_Boost,                   0x0084006e, P_ANY,     T_NONE,     true },  /* Boost */
   {CI_Trim,                    0x0084006f, P_ANY,     T_NONE,     true },  /* Buck */
   {CI_CommunicationLost,       0x00840073, P_ANY,     T_NONE,     false},  /* CommunicationLost */
   {CI_Manufacturer,            0x008400fd, P_ANY,     T_INDEX,    false},  /* iManufacturer */
   {CI_UPSMODEL,                0x008400fe, P_ANY,     T_INDEX,    false},  /* iProduct */
   {CI_SERNO,                   0x008400ff, P_ANY,     T_INDEX,    false},  /* iSerialNumber */
   {CI_MANDAT,                  0x00850085, P_PWSUM,   T_DATE,     false},  /* ManufactureDate */

   /*  Page 0x85 is the Battery System Page */
   /* CI                        USAGE       PHYS       TYPE        VOLATILE? */
   {CI_RemCapLimit,             0x00850029, P_ANY,     T_CAPACITY, false},  /* RemCapLimit */
   {CI_RemTimeLimit,            0x0085002a, P_ANY,     T_UNITS,    false},  /* RemTimeLimit */
   {CI_NONE,                    0x0085002c, P_ANY,     T_CAPACITY, false},  /* CapacityMode */
   {CI_BelowRemCapLimit,        0x00850042, P_ANY,     T_NONE,     true },  /* BelowRemCapLimit */
   {CI_RemTimeLimitExpired,     0x00850043, P_ANY,     T_NONE,     true },  /* RemTimeLimitExpired */
   {CI_Charging,                0x00850044, P_ANY,     T_NONE,     false},  /* Charging */
   {CI_Discharging,             0x00850045, P_ANY,     T_NONE ,    true },  /* Discharging */
   {CI_NeedReplacement,         0x0085004b, P_ANY,     T_NONE ,    true },  /* NeedReplacement */
   {CI_BATTLEV,                 0x00850066, P_ANY,     T_CAPACITY, true },  /* RemainingCapacity */
   {CI_NONE,                    0x00850067, P_ANY,     T_CAPACITY, false},  /* FullChargeCapacity */
   {CI_RUNTIM,                  0x00850068, P_ANY,     T_UNITS,    true },  /* RunTimeToEmpty */
   {CI_CycleCount,              0x0085006b, P_ANY,     T_NONE,     false},
   {CI_BattPackLevel,           0x00850080, P_ANY,     T_NONE,     false},  /* BattPackLevel */
   {CI_NONE,                    0x00850083, P_ANY,     T_CAPACITY, false},  /* DesignCapacity */
   {CI_BATTDAT,                 0x00850085, P_BATTERY, T_DATE,     false},  /* ManufactureDate */
   {CI_IDEN,                    0x00850088, P_ANY,     T_INDEX,    false},  /* iDeviceName */
   {CI_NONE,                    0x00850089, P_ANY,     T_INDEX,    false},  /* iDeviceChemistry */
   {CI_NONE,                    0x0085008b, P_ANY,     T_NONE,     false},  /* Rechargeable */
   {CI_WarningCapacityLimit,    0x0085008c, P_ANY,     T_CAPACITY, false},  /* WarningCapacityLimit */
   {CI_NONE,                    0x0085008d, P_ANY,     T_CAPACITY, false},  /* CapacityGranularity1 */
   {CI_NONE,                    0x0085008e, P_ANY,     T_CAPACITY, false},  /* CapacityGranularity2 */
   {CI_NONE,                    0x0085008f, P_ANY,     T_INDEX,    false},  /* iOEMInformation */
   {CI_ACPresent,               0x008500d0, P_ANY,     T_NONE,     true },  /* ACPresent */
   {CI_BatteryPresent,          0x008500d1, P_ANY,     T_NONE,     false},  /* BatteryPresent */
   {CI_ChargerVoltageOOR,       0x008500d8, P_ANY,     T_NONE,     false},  /* Volt out-of-range */
   {CI_ChargerCurrentOOR,       0x008500d9, P_ANY,     T_NONE,     false},  /* Current out-of-range */
   {CI_CurrentNotRegulated,     0x008500da, P_ANY,     T_NONE,     false},  /* Current not regulated */
   {CI_VoltageNotRegulated,     0x008500db, P_ANY,     T_NONE,     false},  /* VoltageNotRegulated */

   /*  Pages 0xFF00 to 0xFFFF are vendor specific */
   /* CI                        USAGE       PHYS       TYPE        VOLATILE? */
   {CI_STATUS,                  0xFF860060, P_ANY,     T_BITS,     true },  /* APCStatusFlag */
   {CI_DSHUTD,                  0xFF860076, P_ANY,     T_UNITS,    false},  /* APCShutdownAfterDelay */
   {CI_NONE,                    0xFF860005, P_ANY,     T_NONE,     false},  /* APCGeneralCollection */
   {CI_APCForceShutdown,        0xFF86007C, P_ANY,     T_NONE,     false},  /* APCForceShutdown */
   {CI_NONE,                    0xFF860072, P_ANY,     T_NONE,     false},  /* APCPanelTest */
   {CI_BattReplaceDate,         0xFF860016, P_ANY,     T_APCDATE,  false},  /* APCBattReplaceDate */
   {CI_NONE,                    0xFF860042, P_ANY,     T_NONE,     false},  /* APC_UPS_FirmwareRevision */
   {CI_NONE,                    0xFF860079, P_ANY,     T_NONE,     false},  /* APC_USB_FirmwareRevision */
   {CI_APCBattCapBeforeStartup, 0xFF860019, P_ANY,     T_CAPACITY, false},  /* APCBattCapBeforeStartup */
   {CI_APCDelayBeforeStartup,   0xFF86007E, P_ANY,     T_UNITS,    false},  /* APCDelayBeforeStartup */
   {CI_APCDelayBeforeShutdown,  0xFF86007D, P_ANY,     T_UNITS,    false},  /* APCDelayBeforeShutdown */
   {CI_BUPBattCapBeforeStartup, 0x00860012, P_ANY,     T_NONE,     false},  /* BUPBattCapBeforeStartup */
   {CI_BUPDelayBeforeStartup,   0x00860076, P_ANY,     T_NONE,     false},  /* BUPDelayBeforeStartup */
   {CI_BUPSelfTest,             0x00860010, P_ANY,     T_NONE,     false},  /* BUPSelfTest */
   {CI_BUPHibernate,            0x00850058, P_ANY,     T_NONE,     false},  /* BUPHibernate */
   
   /* END OF TABLE */
   {CI_NONE,                    0x00000000, P_ANY,     T_NONE,     false}   /* END OF TABLE */
};

/*
 * USB USAGE NOTES
 *
 * From the NUT project   
 *    
 *  0x860060 == "441HMLL" - looks like a 'capability' string     
 *           == locale 4, 4 choices, 1 byte each                 
 *           == line sensitivity (high, medium, low, low)        
 *  NOTE! the above does not seem to correspond to my info 
 *
 *  0x860013 == 44200155090 - capability again                   
 *           == locale 4, 4 choices, 2 bytes, 00, 15, 50, 90     
 *           == minimum charge to return online                  
 *
 *  0x860062 == D43133136127130                                  
 *           == locale D, 4 choices, 3 bytes, 133, 136, 127, 130 
 *           == high transfer voltage                            
 *
 *  0x860064 == D43103100097106                                  
 *           == locale D, 4 choices, 3 bytes, 103, 100, 097, 106 
 *           == low transfer voltage                             
 *
 *  0x860066 == 441HMLL (see 860060)                                   
 *
 *  0x860074 == 4410TLN                                          
 *           == locale 4, 4 choices, 1 byte, 0, T, L, N          
 *           == alarm setting (5s, 30s, low battery, none)       
 *
 *  0x860077 == 443060180300600                                  
 *           == locale 4, 4 choices, 3 bytes, 060,180,300,600    
 *           == wake-up delay (after power returns)              
 *
 *
 * From MGE -- MGE specific items
 *
 * TestPeriod                      0xffff0045
 * RemainingCapacityLimitSetting   0xffff004d
 * LowVoltageBoostTransfer         0xffff0050
 * HighVoltageBoostTransfer        0xffff0051
 * LowVoltageBuckTransfer          0xffff0052
 * HighVoltageBuckTransfer         0xffff0053
 * iModel                          0xffff00f0
 */


/*
 * Operations that must be handled by platform-specific code
 */

int usb_ups_get_capabilities(UPSINFO *ups)
{
   return pusb_ups_get_capabilities(ups, known_info);
}

int usb_ups_check_state(UPSINFO *ups)
{
   USB_VALUE uval;
   struct timeval exit, now;
   int diff, ci;

   /* Calculate at what time we need to exit */
   gettimeofday(&exit, NULL);
   exit.tv_sec += ups->wait_time;

   while(1) {
      /* Calculate how long before we need to exit */
      gettimeofday(&now, NULL);
      diff = TV_DIFF_MS(now, exit);
      if (diff <= 0)
         break;

      Dmsg1(300, "usb_ups_check_state: Waiting %d msec\n", diff);

      /* Call the lower layer to wait for an event */
      if (pusb_ups_check_state(ups, diff, &ci, &uval)) {
         
         /* Got an event: go process it */
         usb_process_value(ups, ci, &uval);

         switch (ci) {
         /*
          * Some important usages cause us to return so immediate
          * action can be taken.
          */
         case CI_Discharging:
         case CI_ACPresent:
         case CI_BelowRemCapLimit:
         case CI_RemainingCapacity:
         case CI_RunTimeToEmpty:
         case CI_NeedReplacement:
         case CI_ShutdownImminent:
            return true;

         /*
          * We don't handle these directly, but rather use them as a
          * signal to go poll the full set of volatile data.
          */
         case CI_IFailure:
         case CI_Overload:
         case CI_PWVoltageOOR:
         case CI_PWFrequencyOOR:
         case CI_OverCharged:
         case CI_OverTemp:
         case CI_CommunicationLost:
         case CI_ChargerVoltageOOR:
         case CI_ChargerCurrentOOR:
         case CI_CurrentNotRegulated:
         case CI_VoltageNotRegulated:
         case CI_BatteryPresent:
            return true;

         /*
          * Anything else is relatively unimportant, so we can
          * keep gathering data until the timeout.
          */
         default:
            break;
         }
      }
   }

   return true;
}

int usb_ups_open(UPSINFO *ups)
{
   return pusb_ups_open(ups);
}

int usb_ups_close(UPSINFO *ups)
{
   return pusb_ups_close(ups);
}

int usb_ups_setup(UPSINFO *ups)
{
   return pusb_ups_setup(ups);
}

int usb_write_int_to_ups(UPSINFO *ups, int ci, int value, char *name)
{
   return pusb_write_int_to_ups(ups, ci, value, name);
}

int usb_read_int_from_ups(UPSINFO *ups, int ci, int *value)
{
   return pusb_read_int_from_ups(ups, ci, value);
}

#define URB_DELAY_MS 5

static int usb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval)
{
   static struct timeval prev = {0};
   struct timeval now;
   int diff;

   /*
    * Some UPSes (650 CS and 800 RS, possibly others) lock up if
    * control transfers are issued too quickly, so we throttle a
    * bit here.
    */
   if (prev.tv_sec) {
      gettimeofday(&now, NULL);
      diff = TV_DIFF_MS(prev, now);
      if (diff >= 0 && diff < URB_DELAY_MS)
          usleep((URB_DELAY_MS-diff)*1000);
   }
   gettimeofday(&prev, NULL);

   return pusb_get_value(ups, ci, uval);
}


/*
 * Operations that are not supported
 */

int usb_ups_program_eeprom(UPSINFO *ups, int command, char *data)
{
   /* We don't support this for USB */
   return 0;
}

int usb_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   /* What should this do? */
   return 0;
}


/*
 * Operations which are platform agnostic and therefore can be
 * implemented here
 */

static void usb_process_value(UPSINFO* ups, int ci, USB_VALUE* uval)
{
   int v, yy, mm, dd;
   char *p;

   /*
    * ADK FIXME: This switch statement is really excessive. Consider
    * breaking it into volatile vs. non-volatile or perhaps an array
    * of function pointers with handler functions for each CI.
    */

   switch(ci)
   {
   /* UPS_STATUS -- this is the most important status for apcupsd */
   case CI_STATUS:
      ups->Status |= (uval->iValue & 0xff);
      break;

   case CI_ACPresent:
      if (uval->iValue) {
         ups->set_online();
         Dmsg0(200, "ACPRESENT\n");
      }
      break;

   case CI_Discharging:
      if (uval->iValue) {
         ups->clear_online();
         Dmsg0(200, "DISCHARGING\n");
      }
      break;

   case CI_BelowRemCapLimit:
      if (uval->iValue) {
         ups->set_battlow();
         Dmsg1(200, "BelowRemCapLimit=%d\n", uval->iValue);
      }
      break;

   case CI_RemTimeLimitExpired:
      if (uval->iValue) {
         ups->set_battlow();
         Dmsg0(200, "RemTimeLimitExpired\n");
      }
      break;

   case CI_ShutdownImminent:
      if (uval->iValue) {
         ups->set_battlow();
         Dmsg0(200, "ShutdownImminent\n");
      }
      break;

   case CI_Boost:
      if (uval->iValue) {
         ups->set_boost();
         Dmsg0(200, "Boost\n");
      }
      break;

   case CI_Trim:
      if (uval->iValue) {
         ups->set_trim();
         Dmsg0(200, "Trim\n");
      }
      break;

   case CI_Overload:
      if (uval->iValue) {
         ups->set_overload();
         Dmsg0(200, "Overload\n");
      }
      break;

   case CI_NeedReplacement:
      if (uval->iValue) {
         ups->set_replacebatt();
         Dmsg0(200, "ReplaceBatt\n");
      }
      break;

   /* LINE_VOLTAGE */
   case CI_VLINE:
      ups->LineVoltage = uval->dValue;
      Dmsg1(200, "LineVoltage = %d\n", (int)ups->LineVoltage);
      break;

   /* OUTPUT_VOLTAGE */
   case CI_VOUT:
      ups->OutputVoltage = uval->dValue;
      Dmsg1(200, "OutputVoltage = %d\n", (int)ups->OutputVoltage);
      break;

   /* BATT_FULL Battery level percentage */
   case CI_BATTLEV:
      ups->BattChg = uval->dValue;
      Dmsg1(200, "BattCharge = %d\n", (int)ups->BattChg);
      break;

   /* BATT_VOLTAGE */
   case CI_VBATT:
      ups->BattVoltage = uval->dValue;
      Dmsg1(200, "BattVoltage = %d\n", (int)ups->BattVoltage);
      break;

   /* UPS_LOAD */
   case CI_LOAD:
      ups->UPSLoad = uval->dValue;
      Dmsg1(200, "UPSLoad = %d\n", (int)ups->UPSLoad);
      break;

   /* LINE_FREQ */
   case CI_FREQ:
      ups->LineFreq = uval->dValue;
      Dmsg1(200, "LineFreq = %d\n", (int)ups->LineFreq);
      break;

   /* UPS_RUNTIME_LEFT */
   case CI_RUNTIM:
      ups->TimeLeft = uval->dValue / 60; /* convert to minutes */
      Dmsg1(200, "TimeLeft = %d\n", (int)ups->TimeLeft);
      break;

   /* UPS_TEMP */
   case CI_ITEMP:
      ups->UPSTemp = uval->dValue - 273.15;      /* convert to deg C. */
      Dmsg1(200, "ITemp = %d\n", (int)ups->UPSTemp);
      break;

   /*  Humidity percentage */ 
   case CI_HUMID:
      ups->humidity = uval->dValue;
      Dmsg1(200, "Humidity = %d\n", (int)ups->humidity);
      break;

   /*  Ambient temperature */ 
   case CI_ATEMP:
      ups->ambtemp = uval->dValue;
      Dmsg1(200, "ATemp = %d\n", (int)ups->ambtemp);
      break;

   /* Self test results */
   case CI_ST_STAT:
      switch (uval->iValue) {
      case 1:  /* Passed */
         astrncpy(ups->X, "OK", sizeof(ups->X));
         break;
      case 2:  /* Warning */
         astrncpy(ups->X, "WN", sizeof(ups->X));
         break;
      case 3:  /* Error */
      case 4:  /* Aborted */
         astrncpy(ups->X, "NG", sizeof(ups->X));
         break;
      case 5:  /* In progress */
         astrncpy(ups->X, "IP", sizeof(ups->X));
         break;
      case 6:  /* None */
         astrncpy(ups->X, "NO", sizeof(ups->X));
         break;
      default:
         break;
      }
      break;

   /* UPS_NAME */
   case CI_IDEN:
      if (ups->upsname[0] == 0 && ups->buf[0] != 0) {
         strncpy(ups->upsname, ups->buf, sizeof(ups->upsname) - 1);
         ups->upsname[sizeof(ups->upsname) - 1] = 0;
      }
      break;

   /* model, firmware */
   case CI_UPSMODEL:
      /* Truncate Firmware info on APC Product string */
      if ((p = strchr(ups->buf, 'F')) && *(p + 1) == 'W' && *(p + 2) == ':') {
         *(p - 1) = 0;
         strncpy(ups->firmrev, p + 4, sizeof(ups->firmrev) - 1);
         ups->firmrev[sizeof(ups->firmrev) - 1] = 0;
         ups->UPS_Cap[CI_REVNO] = true;
      }

      strncpy(ups->upsmodel, ups->buf, sizeof(ups->upsmodel) - 1);
      ups->upsmodel[sizeof(ups->upsmodel) - 1] = 0;
      strncpy(ups->mode.long_name, ups->buf, sizeof(ups->mode.long_name) - 1);
      ups->mode.long_name[sizeof(ups->mode.long_name) - 1] = 0;
      break;

   /* WAKEUP_DELAY */
   case CI_DWAKE:
      ups->dwake = (int)uval->dValue;
      break;

   /* SLEEP_DELAY */
   case CI_DSHUTD:
      ups->dshutd = (int)uval->dValue;
      break;

   /* LOW_TRANSFER_LEVEL */
   case CI_LTRANS:
      ups->lotrans = (int)uval->dValue;
      break;

   /* HIGH_TRANSFER_LEVEL */
   case CI_HTRANS:
      ups->hitrans = (int)uval->dValue;
      break;

   /* UPS_BATT_CAP_RETURN */
   case CI_RETPCT:
      ups->rtnpct = (int)uval->dValue;
      break;

   /* LOWBATT_SHUTDOWN_LEVEL */
   case CI_DLBATT:
      ups->dlowbatt = (int)uval->dValue;
      break;

   /* UPS_MANUFACTURE_DATE */
   case CI_MANDAT:
      asnprintf(ups->birth, sizeof(ups->birth), "%4d-%02d-%02d",
         (uval->iValue >> 9) + 1980, (uval->iValue >> 5) & 0xF,
         uval->iValue & 0x1F);
      break;

   /* Last UPS_BATTERY_REPLACE */
   case CI_BATTDAT:
      asnprintf(ups->battdat, sizeof(ups->battdat), "%4d-%02d-%02d",
         (uval->iValue >> 9) + 1980, (uval->iValue >> 5) & 0xF,
         uval->iValue & 0x1F);
      break;

   /* APC_BATTERY_DATE */
   case CI_BattReplaceDate:
      v = uval->iValue;
      yy = ((v >> 4) & 0xF) * 10 + (v & 0xF) + 2000;
      v >>= 8;
      dd = ((v >> 4) & 0xF) * 10 + (v & 0xF);
      v >>= 8;
      mm = ((v >> 4) & 0xF) * 10 + (v & 0xF);
      asnprintf(ups->battdat, sizeof(ups->battdat), "%4d-%02d-%02d", yy, mm, dd);
      break;

   /* UPS_SERIAL_NUMBER */
   case CI_SERNO:
      astrncpy(ups->serial, ups->buf, sizeof(ups->serial));

      /*
       * If serial number has garbage, trash it.
       */
      for (p = ups->serial; *p; p++) {
         if (*p < ' ' || *p > 'z') {
            *ups->serial = 0;
            ups->UPS_Cap[CI_SERNO] = false;
         }
      }
      break;

   /* Nominal output voltage when on batteries */
   case CI_NOMOUTV:
      ups->NomOutputVoltage = (int)uval->dValue;
      break;

   /* Nominal battery voltage */
   case CI_NOMBATTV:
      ups->nombattv = uval->dValue;
      break;

   default:
      break;
   }
}

static void usb_update_value(UPSINFO* ups, int ci)
{
   USB_VALUE uval;

   if (usb_get_value(ups, ci, &uval)) {
      usb_process_value(ups, ci, &uval);
   }
}


/*
 * Read UPS info that changes -- e.g. voltage, temperature, etc.
 *
 * This routine is called once every N seconds to get a current
 * idea of what the UPS is doing.
 */
int usb_ups_read_volatile_data(UPSINFO *ups)
{
   time_t last_poll = ups->poll_time;
   time_t now = time(NULL);

   Dmsg0(200, "Enter usb_ups_read_volatile_data\n");

   /* 
    * If we are not on batteries, update this maximum once every
    * MAX_VOLATILE_POLL_RATE seconds. This prevents flailing around
    * too much if the UPS state is rapidly changing while on mains.
    */
   if (ups->is_onbatt() && last_poll &&
       (now - last_poll < MAX_VOLATILE_POLL_RATE)) {
      return 1;
   }

   write_lock(ups);
   ups->poll_time = now;           /* save time stamp */

   /* Loop through all known data, polling the ones marked volatile */
   for (int i=0; known_info[i].usage_code; i++) {
      if (known_info[i].isvolatile && known_info[i].ci != CI_NONE)
         usb_update_value(ups, known_info[i].ci);
   }

   write_unlock(ups);
   return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer voltages, 
 * shutdown delay, etc.
 *
 * This routine is called once when apcupsd is starting.
 */
int usb_ups_read_static_data(UPSINFO *ups)
{
   write_lock(ups);

   /* Loop through all known data, polling the ones marked non-volatile */
   for (int i=0; known_info[i].usage_code; i++) {
      if (!known_info[i].isvolatile && known_info[i].ci != CI_NONE)
         usb_update_value(ups, known_info[i].ci);
   }

   write_unlock(ups);
   return 1;
}

/* How long to wait before killing output power */
#define SHUTDOWN_DELAY  60

/* How many seconds of good utility power before turning output back on */
#define STARTUP_DELAY   10

int usb_ups_kill_power(UPSINFO *ups)
{
   char *func;
   int shutdown = 0;
   int val;

   Dmsg0(200, "Enter usb_ups_kill_power\n");

   /*
    * We try various different ways to shutdown the UPS (i.e.
    * killpower). Some of these commands are not supported on all 
    * UPSes, but that should cause no harm.
    */

   /*
    * First, set required battery capacity before startup to 0 so UPS
    * will not wait for the battery to charge before turning back on.
    * Not all UPSes have this capability, so this setting is allowed
    * to fail. The value we program here should be made configurable
    * some day.
    */
   if (UPS_HAS_CAP(CI_APCBattCapBeforeStartup)) {
      func = "CI_APCBattCapBeforeStartup";
      if (!usb_write_int_to_ups(ups, CI_APCBattCapBeforeStartup, 0, func))
         Dmsg1(100, "Unable to set %s (not an error)\n", func);
   }

   /*
    * BackUPS Pro uses an enumerated setting (reads percent in 
    * ASCII). The value advances to the next higher setting by
    * writing a '1' and to the next lower setting when writing a 
    * '2'. The value wraps around when advanced past the max or min
    * setting.
    *
    * We walk the setting down to the minimum of 0.
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring
    * this out.
    */
   if (UPS_HAS_CAP(CI_BUPBattCapBeforeStartup)) {
      if (pusb_read_int_from_ups(ups, CI_BUPBattCapBeforeStartup, &val)) {
         func = "CI_BUPBattCapBeforeStartup";
         switch (val) {
         case 0x3930:             /* 90% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3630:             /* 60% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3135:             /* 15% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3030:             /* 00% */
            break;

         default:
            Dmsg1(100, "Unknown BUPBattCapBeforeStartup value (%04x)\n", val);
            break;
         }
      }
   }

   /*
    * Second, set the length of time to wait after power returns
    * before starting up. We set it to something pretty low, but it
    * seems the UPS rounds this value up to the nearest multiple of
    * 60 seconds. Not all UPSes have this capability, so this setting
    * is allowed to fail.  The value we program here should be made
    * configurable some day.
    */
   if (UPS_HAS_CAP(CI_APCDelayBeforeStartup)) {
      func = "CI_APCDelayBeforeStartup";
      if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeStartup, 10, func)) {
         Dmsg1(100, "Unable to set %s (not an error)\n", func);
      }
   }

   /*
    * BackUPS Pro uses an enumerated setting (reads seconds in ASCII).
    * The value advances to the next higher setting by writing a '1' 
    * and to the next lower setting when writing a '2'. The value 
    * wraps around when advanced past the max or min setting.
    *
    * We walk the setting down to the minimum of 60.
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring
    * this out.
    */
   if (UPS_HAS_CAP(CI_BUPDelayBeforeStartup)) {
      if (pusb_read_int_from_ups(ups, CI_BUPDelayBeforeStartup, &val)) {
         func = "CI_BUPDelayBeforeStartup";
         switch (val) {
         case 0x363030:           /* 600 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x333030:           /* 300 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x313830:           /* 180 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3630:             /* 60 sec */
            break;

         default:
            Dmsg1(100, "Unknown CI_BUPDelayBeforeStartup value (%04x)\n", val);
            break;
         }
      }
   }

   /*
    * BackUPS shutdown
    *
    * Alternately, if APCDelayBeforeShutdown is available, setting 
    * it will start a countdown after which the UPS will hibernate.
    */
   if (!shutdown && UPS_HAS_CAP(CI_APCDelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support BackUPS style shutdown.\n");
      func = "CI_APCDelayBeforeShutdown";
      if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeShutdown,
            SHUTDOWN_DELAY, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         shutdown = 1;
      }
   }

   /*
    * SmartUPS shutdown
    *
    * If both DWAKE and DelayBeforeShutdown are available, trigger
    * a hibernate by writing DWAKE a few seconds longer than 
    * DelayBeforeShutdown. ORDER IS IMPORTANT. The write to 
    * DelayBeforeShutdown starts both timers ticking down and the
    * UPS will hibernate when DelayBeforeShutdown hits zero.
    */
   if (!shutdown && UPS_HAS_CAP(CI_DWAKE) && UPS_HAS_CAP(CI_DelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support SmartUPS style shutdown.\n");
      func = "CI_DWAKE";
      if (!usb_write_int_to_ups(ups, CI_DWAKE, SHUTDOWN_DELAY + 4, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         func = "CI_DelayBeforeShutdown";
         if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown,
               SHUTDOWN_DELAY, func)) {
            Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
            /* reset prev timer */
            usb_write_int_to_ups(ups, CI_DWAKE, -1, "CI_DWAKE");  
         } else {
            shutdown = 1;
         }
      }
   }

   /*
    * BackUPS Pro shutdown
    *
    * Here we see the BackUPS Pro further distinguish itself as 
    * having the most broken firmware of any APC product yet. We have
    * to trigger two magic boolean flags using APC custom usages.
    * First we hit BUPHibernate and follow that with a write to 
    * BUPSelfTest (!).
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring 
    * this out.
    */
   if (!shutdown && UPS_HAS_CAP(CI_BUPHibernate) && UPS_HAS_CAP(CI_BUPSelfTest)) {
      Dmsg0(000, "UPS appears to support BackUPS Pro style shutdown.\n");
      func = "CI_BUPHibernate";
      if (!pusb_write_int_to_ups(ups, CI_BUPHibernate, 1, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         func = "CI_BUPSelfTest";
         if (!pusb_write_int_to_ups(ups, CI_BUPSelfTest, 1, func)) {
            Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
            pusb_write_int_to_ups(ups, CI_BUPHibernate, 0, "CI_BUPHibernate");
         } else {
            shutdown = 1;
         }
      }
   }

   /*
    * All UPSes tested so far are covered by one of the above cases. 
    * However, there are a couple other ways to shutdown.
    */

   /*
    * Misc method A
    *
    * Writing CI_DelayBeforeReboot starts a countdown timer, after 
    * which the UPS will hibernate. If utility power is out, the UPS
    * will stay hibernating until power is restored. SmartUPSes seem
    * to support this method, but PowerChute uses the dual countdown
    * method above, so we prefer that one. UPSes seem to round the
    * value up to 90 seconds if it is any lower. Note that the
    * behavior described here DOES NOT comply with the standard set
    * out in the HID Usage Tables for Power Devices spec. 
    */
   if (!shutdown && UPS_HAS_CAP(CI_DelayBeforeReboot)) {
      Dmsg0(000, "UPS appears to support DelayBeforeReboot style shutdown.\n");

      func = "CI_DelayBeforeReboot";
      if (!usb_write_int_to_ups(ups, CI_DelayBeforeReboot, SHUTDOWN_DELAY, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         shutdown = 1;
   }

   /*
    * Misc method B
    *
    * We can set CI_APCForceShutdown to true (it's a boolean flag).
    * We have no control over how long the UPS waits before turning
    * off. Experimentally it seems to be about 60 seconds. Some 
    * BackUPS models support this in addition to the preferred 
    * BackUPS method above. It's included here "just in case".
    */
   if (!shutdown && UPS_HAS_CAP(CI_APCForceShutdown)) {
      Dmsg0(000, "UPS appears to support ForceShutdown style shutdown.\n");

      func = "CI_APCForceShutdown";
      if (!usb_write_int_to_ups(ups, CI_APCForceShutdown, 1, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         shutdown = 1;
   }

   /*
    * Misc method C
    *
    * This one seems to turn the UPS off completely after a given 
    * delay. The only way to power the UPS back on is to manually hit
    * the power button.
    */
   if (!shutdown && UPS_HAS_CAP(CI_DelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support DelayBeforeShutdown style shutdown.\n");

      func = "CI_DelayBeforeShutdown";
      if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown, SHUTDOWN_DELAY, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         shutdown = 1;
   }

   /*
    * I give up.
    */
   if (!shutdown) {
      Dmsg0(000, "I don't know how to turn off this UPS...sorry.\n"
         "Please report this, along with the output from\n"
         "running examples/hid-ups, to the apcupsd-users\n"
         "mailing list (apcupsd-users@lists.sourceforge.net).\n");
   }

   /*
    * Note that there are a couple other CIs that look interesting
    * for shutdown, but they're not what we want.
    *
    * APCShutdownAfterDelay: Tells the UPS how many seconds to wait
    *     after power goes out before asserting ShutdownRequested
    *    (see next item).
    *
    * CI_ShutdownRequested: This is an indicator from the UPS to the
    *     server that it would like the server to begin shutting
    *     down. In conjunction with APCShutdownAfterDelay this can be
    *     used to offload the decision of when to shut down the
    *     server to the UPS.
    */

   Dmsg0(200, "Leave usb_ups_kill_power\n");
   return shutdown;
}


/*
 * Helper functions for use by platform specific code
 */

double pow_ten(int exponent)
{
   int i;
   double val = 1;

   if (exponent < 0) {
      exponent = -exponent;
      for (i = 0; i < exponent; i++) {
         val = val / 10;
      }
      return val;
   } else {
      for (i = 0; i < exponent; i++) {
         val = val * 10;
      }
   }
   return val;
}
