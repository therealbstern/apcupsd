/*
 *
 *  Public driver interface for all platform USB drivers.
 *
 *   Adam Kropelin, November 2004 
 *
 *   Based on linux-usb.c by Kern Sibbald
 */

/*
   Copyright (C) 2004 Adam Kropelin

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "apc.h"
#include "usb.h"
#include "usb_common.h"
#include <math.h>

/* Implemented in platform-specific code */
int pusb_ups_get_capabilities(UPSINFO *ups, const struct s_known_info* known_info);
int pusb_ups_open(UPSINFO *ups);
int pusb_ups_close(UPSINFO *ups);
int pusb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval);
int pusb_ups_check_state(UPSINFO *ups);
int pusb_ups_setup(UPSINFO *ups);
int pusb_write_int_to_ups(UPSINFO *ups, int ci, int value, char *name);
int pusb_read_int_from_ups(UPSINFO *ups, int ci, int* value);

/*
 * This table is used when walking through the USB reports to see
 *   what information found in the UPS that we want. If the
 *   usage_code and the physical code match, then we make an
 *   entry in the command index table containing the usage information
 *   provided by the UPS as well as the data type from this table.
 *   Entries in the table with ci == CI_NONE are not used, for the
 *   moment, they are retained just so they are not forgotten.
 */
const struct s_known_info known_info[] = {
    /*	Page 0x84 is the Power Device Page */
    { CI_NONE,		      0x840001, P_ANY,	  T_INDEX   },	/* iName */
    { CI_VLINE, 	      0x840030, P_INPUT,  T_UNITS   },	/* Line Voltage */
    { CI_VOUT,		      0x840030, P_OUTPUT, T_UNITS   },	/* Output Voltage */
    { CI_VBATT, 	      0x840030, P_BATTERY,T_UNITS   },	/* Battery Voltage */
    { CI_NONE,		      0x840031, P_ANY,	  T_UNITS   },	/* Current */
    { CI_FREQ,		      0x840032, P_OUTPUT, T_UNITS   },	/* Frequency */
    { CI_NONE,		      0x840033, P_ANY,	  T_UNITS   },	/* ApparentPower */
    { CI_NONE,		      0x840034, P_ANY,	  T_UNITS   },	/* ActivePower */
    { CI_LOAD,		      0x840035, P_ANY,	  T_UNITS   },	/* PercentLoad */
    { CI_ITEMP, 	      0x840036, P_BATTERY,T_UNITS   },	/* Temperature */
    { CI_HUMID, 	      0x840037, P_ANY,	  T_UNITS   },	/* Humidity */
    { CI_NOMBATTV,	      0x840040, P_ANY,	  T_UNITS   },	/* ConfigVoltage */
    { CI_NONE,		      0x840042, P_ANY,	  T_UNITS   },	/* ConfigFrequency */
    { CI_NONE,		      0x840043, P_ANY,	  T_UNITS   },	/* ConfigApparentPower */
    { CI_LTRANS,	      0x840053, P_ANY,	  T_UNITS   },	/* LowVoltageTransfer */
    { CI_HTRANS,	      0x840054, P_ANY,	  T_UNITS   },	/* HighVoltageTransfer */
    { CI_DelayBeforeReboot,   0x840055, P_ANY,	  T_UNITS   },	/* DelayBeforeReboot */
    { CI_DWAKE, 	      0x840056, P_ANY,	  T_UNITS   },	/* DelayBeforeStartup */
    { CI_DelayBeforeShutdown, 0x840057, P_ANY,	  T_UNITS   },	/* DelayBeforeShutdown */
    { CI_ST_STAT,	      0x840058, P_ANY,	  T_NONE    },	/* Test */
    { CI_DALARM,	      0x84005a, P_ANY,	  T_NONE    },	/* AudibleAlarmControl */
    { CI_NONE,		      0x840061, P_ANY,	  T_NONE    },	/* Good */
    { CI_NONE,		      0x840062, P_ANY,	  T_NONE    },	/* InternalFailure */
    { CI_Overload,	      0x840065, P_ANY,	  T_NONE    },	/* Overload */
    { CI_ShutdownRequested,   0x840068, P_ANY,	  T_NONE    },	/* ShutdownRequested */
    { CI_ShutdownImminent,    0x840069, P_ANY,	  T_NONE    },	/* ShutdownImminent */
    { CI_NONE,		      0x84006b, P_ANY,	  T_NONE    },	/* Switch On/Off */
    { CI_NONE,		      0x84006c, P_ANY,	  T_NONE    },	/* Switchable */
    { CI_Boost, 	      0x84006e, P_ANY,	  T_NONE    },	/* Boost */
    { CI_Trim,		      0x84006f, P_ANY,	  T_NONE    },	/* Buck */
    { CI_NONE,		      0x840073, P_ANY,	  T_NONE    },	/* CommunicationLost */
    { CI_Manufacturer,	      0x8400fd, P_ANY,	  T_INDEX   },	/* iManufacturer */
    { CI_UPSMODEL,	      0x8400fe, P_ANY,	  T_INDEX   },	/* iProduct */
    { CI_SERNO, 	      0x8400ff, P_ANY,	  T_INDEX   },	/* iSerialNumber */
    { CI_MANDAT,	      0x850085, P_PWSUM,  T_DATE    },	/* ManufactureDate */

    /*	Page 0x85 is the Battery System Page */
    { CI_RemCapLimit,	      0x850029, P_ANY,	  T_CAPACITY},	/* RemCapLimit */
    { CI_RemTimeLimit,	      0x85002a, P_ANY,	  T_UNITS   },	/* RemTimeLimit */
    { CI_NONE,		      0x85002c, P_ANY,	  T_CAPACITY},	/* CapacityMode */
    { CI_BelowRemCapLimit,    0x850042, P_ANY,	  T_NONE    },	/* BelowRemCapLimit */
    { CI_RemTimeLimitExpired, 0x850043, P_ANY,	  T_NONE    },	/* RemTimeLimitExpired */
    { CI_Charging,	      0x850044, P_ANY,	  T_NONE    },	/* Charging */
    { CI_Discharging,	      0x850045, P_ANY,	  T_NONE    },	/* Discharging */
    { CI_NeedReplacement,     0x85004b, P_ANY,	  T_NONE    },	/* NeedReplacement */
    { CI_BATTLEV,	      0x850066, P_ANY,	  T_CAPACITY},	/* RemainingCapacity */
    { CI_NONE,		      0x850067, P_ANY,	  T_CAPACITY},	/* FullChargeCapacity */
    { CI_RUNTIM,	      0x850068, P_ANY,	  T_UNITS   },	/* RunTimeToEmpty */
    { CI_CycleCount,	      0x85006b, P_ANY,	  T_NONE    },
    { CI_BattPackLevel,       0x850080, P_ANY,	  T_NONE    },	/* BattPackLevel */
    { CI_NONE,		      0x850083, P_ANY,	  T_CAPACITY},	/* DesignCapacity */
    { CI_BATTDAT,	      0x850085, P_BATTERY,T_DATE    },	/* ManufactureDate */
    { CI_IDEN,		      0x850088, P_ANY,	  T_INDEX   },	/* iDeviceName */
    { CI_NONE,		      0x850089, P_ANY,	  T_INDEX   },	/* iDeviceChemistry */
    { CI_NONE,		      0x85008b, P_ANY,	  T_NONE    },	/* Rechargeable */
    { CI_WarningCapacityLimit,0x85008c, P_ANY,	  T_CAPACITY},	/* WarningCapacityLimit */
    { CI_NONE,		      0x85008d, P_ANY,	  T_CAPACITY},	/* CapacityGranularity1 */
    { CI_NONE,		      0x85008e, P_ANY,	  T_CAPACITY},	/* CapacityGranularity2 */
    { CI_NONE,		      0x85008f, P_ANY,	  T_INDEX   },	/* iOEMInformation */
    { CI_ACPresent,	      0x8500d0, P_ANY,	  T_NONE    },	/* ACPresent */
    { CI_NONE,		      0x8500d1, P_ANY,	  T_NONE    },	/* BatteryPresent */
    { CI_NONE,		      0x8500db, P_ANY,	  T_NONE    },	/* VoltageNotRegulated */

    /*	Pages 0xFF00 to 0xFFFF are vendor specific */
    { CI_STATUS,                  0xFF860060, P_ANY,  T_BITS    },  /* APCStatusFlag */
    { CI_DSHUTD,                  0xFF860076, P_ANY,  T_UNITS   },  /* APCShutdownAfterDelay */
    { CI_NONE,                    0xFF860005, P_ANY,  T_NONE    },  /* APCGeneralCollection */
    { CI_APCForceShutdown,        0xFF86007C, P_ANY,  T_NONE    },  /* APCForceShutdown */
    { CI_NONE,                    0xFF860072, P_ANY,  T_NONE    },  /* APCPanelTest */
    { CI_BattReplaceDate,         0xFF860016, P_ANY,  T_APCDATE },  /* APCBattReplaceDate */
    { CI_NONE,                    0xFF860042, P_ANY,  T_NONE    },  /* APC_UPS_FirmwareRevision */
    { CI_NONE,                    0xFF860079, P_ANY,  T_NONE    },  /* APC_USB_FirmwareRevision */
    { CI_APCBattCapBeforeStartup, 0xFF860019, P_ANY,  T_CAPACITY},  /* APCBattCapBeforeStartup */
    { CI_APCDelayBeforeStartup,   0xFF86007E, P_ANY,  T_UNITS   },  /* APCDelayBeforeStartup */
    { CI_APCDelayBeforeShutdown,  0xFF86007D, P_ANY,  T_UNITS   },  /* APCDelayBeforeShutdown */
    { CI_BUPBattCapBeforeStartup, 0x00860012, P_ANY,  T_NONE    },  /* BUPBattCapBeforeStartup */
    { CI_BUPDelayBeforeStartup,   0x00860076, P_ANY,  T_NONE    },  /* BUPDelayBeforeStartup */
    { CI_BUPSelfTest,             0x00860010, P_ANY,  T_NONE    },  /* BUPSelfTest */
    { CI_BUPHibernate,            0x00850058, P_ANY,  T_NONE    },  /* BUPHibernate */
    { CI_NONE,                    0x00000000, P_ANY,  T_NONE    }   /* END OF TABLE */
};

/*
   From Matthew Mastracci
   Looks like the ff86007d register is some sort of smart shutdown.  When
   you write a value (say 5) to the register, it will wait that many
   seconds and then, depending on the power, perform an action:
   1.  If power is off. 
   - UPS will wait with Online/Overload alternating for a few seconds and
   then power off completely.
   2.  If power is on. - UPS will wait with Online/Overload alternating for a few seconds and
   then power back on.
   UPS HID device name: "American Power Conversion Back-UPS RS 1000 FW:7.g5

===

    From the NUT project   
     
    0x860060 == "441HMLL" - looks like a 'capability' string     
	     == locale 4, 4 choices, 1 byte each		 
	     == line sensitivity (high, medium, low, low)	 
    NOTE! the above does not seem to correspond to my info 

    0x860013 == 44200155090 - capability again			 
	     == locale 4, 4 choices, 2 bytes, 00, 15, 50, 90	 
	     == minimum charge to return online 		 

    0x860062 == D43133136127130 				 
	     == locale D, 4 choices, 3 bytes, 133, 136, 127, 130 
	     == high transfer voltage				 

    0x860064 == D43103100097106 				 
	     == locale D, 4 choices, 3 bytes, 103, 100, 097, 106 
	     == low transfer voltage				 

    0x860066 == 441HMLL (see 860060)				       

    0x860074 == 4410TLN 					 
	     == locale 4, 4 choices, 1 byte, 0, T, L, N 	 
	     == alarm setting (5s, 30s, low battery, none)	 

    0x860077 == 443060180300600 				 
	     == locale 4, 4 choices, 3 bytes, 060,180,300,600	 
	     == wake-up delay (after power returns)		 
  
===

   
   From MGE -- MGE specific items

   TestPeriod			   0xffff0045
   RemainingCapacityLimitSetting   0xffff004d
   LowVoltageBoostTransfer	   0xffff0050
   HighVoltageBoostTransfer	   0xffff0051
   LowVoltageBuckTransfer	   0xffff0052
   HighVoltageBuckTransfer	   0xffff0053
   iModel			   0xffff00f0
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
    return pusb_ups_check_state(ups);
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

int usb_read_int_from_ups(UPSINFO *ups, int ci, int* value)
{
    return pusb_read_int_from_ups(ups, ci, value);
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
 * Operations which are platform agnostic and therefore can be implemented here
 */

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every 5 seconds to get
 *  a current idea of what the UPS is doing.
 */
int usb_ups_read_volatile_data(UPSINFO *ups)
{
    USB_VALUE uval;
    time_t last_poll = ups->poll_time;
    time_t now = time(NULL);

    Dmsg0(200, "Enter usb_ups_read_volatile_data\n");
                            
    /* 
     * If we are not on batteries, update this maximum once every
     *  5 seconds. This prevents flailing around too much if the
     *  UPS state is rapidly changing while on mains.
     */
    if (is_ups_set(UPS_ONBATT) && last_poll && 
        (now - last_poll < MAX_VOLATILE_POLL_RATE)) {
       return 1;
    }
    write_lock(ups);
    ups->poll_time = now;             /* save time stamp */

    /* UPS_STATUS -- this is the most important status for apcupsd */

    ups->Status &= ~0xff;            /* Clear APC part of Status */
    if (pusb_get_value(ups, CI_STATUS, &uval)) {
        ups->Status |= (uval.iValue & 0xff); /* set new APC part */
    } else {
        /* No APC Status value, well, fabricate one */
        if (pusb_get_value(ups, CI_ACPresent, &uval) && uval.iValue) {
            set_ups_online();
            Dmsg0(200,"ACPRESENT\n");
        }
        if (pusb_get_value(ups, CI_Discharging, &uval) && uval.iValue) {
            clear_ups_online();
            Dmsg0(200,"DISCHARGING\n");
        }
        if (pusb_get_value(ups, CI_BelowRemCapLimit, &uval) && uval.iValue) {
            set_ups(UPS_BATTLOW);
            Dmsg1(200, "BelowRemCapLimit=%d\n", uval.iValue);
        }
        if (pusb_get_value(ups, CI_RemTimeLimitExpired, &uval) && uval.iValue) {
            set_ups(UPS_BATTLOW);
            Dmsg0(200, "RemTimeLimitExpired\n");
        }
        if (pusb_get_value(ups, CI_ShutdownImminent, &uval) && uval.iValue) {
            set_ups(UPS_BATTLOW);
            Dmsg0(200, "ShutdownImminent\n");
        }
        if (pusb_get_value(ups, CI_Boost, &uval) && uval.iValue) {
            set_ups(UPS_SMARTBOOST);
        }
        if (pusb_get_value(ups, CI_Trim, &uval) && uval.iValue) {
            set_ups(UPS_SMARTTRIM);
        }
        if (pusb_get_value(ups, CI_Overload, &uval) && uval.iValue) {
            set_ups(UPS_OVERLOAD);
        }
        if (pusb_get_value(ups, CI_NeedReplacement, &uval) && uval.iValue) {
            set_ups(UPS_REPLACEBATT);
        }
    }

    /* LINE_VOLTAGE */
    if (pusb_get_value(ups, CI_VLINE, &uval)) {
        ups->LineVoltage = uval.dValue;
        Dmsg1(200, "LineVoltage = %d\n", (int)ups->LineVoltage);
    }

    /* OUTPUT_VOLTAGE */
    if (pusb_get_value(ups, CI_VOUT, &uval)) {
        ups->OutputVoltage = uval.dValue;
        Dmsg1(200, "OutputVoltage = %d\n", (int)ups->OutputVoltage);
    }

    /* BATT_FULL Battery level percentage */
    if (pusb_get_value(ups, CI_BATTLEV, &uval)) {
        ups->BattChg = uval.dValue;
        Dmsg1(200, "BattCharge = %d\n", (int)ups->BattChg);
    }

    /* BATT_VOLTAGE */
    if (pusb_get_value(ups, CI_VBATT, &uval)) {
        ups->BattVoltage = uval.dValue;
        Dmsg1(200, "BattVoltage = %d\n", (int)ups->BattVoltage);
    }

    /* UPS_LOAD */
    if (pusb_get_value(ups, CI_LOAD, &uval)) {
        ups->UPSLoad = uval.dValue;
        Dmsg1(200, "UPSLoad = %d\n", (int)ups->UPSLoad);
    }

    /* LINE_FREQ */
    if (pusb_get_value(ups, CI_FREQ, &uval)) {
        ups->LineFreq = uval.dValue;
    }

    /* UPS_RUNTIME_LEFT */
    if (pusb_get_value(ups, CI_RUNTIM, &uval)) {
        ups->TimeLeft = uval.dValue / 60;   /* convert to minutes */
        Dmsg1(200, "TimeLeft = %d\n", (int)ups->TimeLeft);
    }

    /* UPS_TEMP */
    if (pusb_get_value(ups, CI_ITEMP, &uval)) {
        ups->UPSTemp = uval.dValue - 273.15; /* convert to deg C. */
    }

    /*  Humidity percentage */ 
    if (pusb_get_value(ups, CI_HUMID, &uval)) {
        ups->humidity = uval.dValue;
    }

    /*  Ambient temperature */ 
    if (pusb_get_value(ups, CI_ATEMP, &uval)) {
        ups->ambtemp = uval.dValue;
    }

    /* Self test results */
    if (pusb_get_value(ups, CI_ST_STAT, &uval)) {
        switch (uval.iValue) {
        case 1:                       /* passed */
           astrncpy(ups->X, "OK", sizeof(ups->X));
           break;
        case 2:                       /* Warning */
           astrncpy(ups->X, "WN", sizeof(ups->X));
           break;
        case 3:                       /* Error */
        case 4:                       /* Aborted */
           astrncpy(ups->X, "NG", sizeof(ups->X));
           break;
        case 5:                       /* In progress */
           astrncpy(ups->X, "IP", sizeof(ups->X));
           break;
        case 6:                       /* None */
           astrncpy(ups->X, "NO", sizeof(ups->X));
           break;
        default:
           break;
        }
    }

    write_unlock(ups);
    return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 *   voltages, shutdown delay, ...
 *
 *  This routine is called once when apcupsd is starting
 */
int usb_ups_read_static_data(UPSINFO *ups)
{
    USB_VALUE uval;
    int v, yy, mm, dd;

    write_lock(ups);

#ifdef xxxx
    /* ALARM_STATUS */
    if (ups->UPS_Cap[CI_DALARM])
        strncpy(ups->beepstate, smart_poll(ups->UPS_Cmd[CI_DALARM], ups), 
               sizeof(ups->beepstate));
    /* UPS_SELFTEST Interval */
    if (pusb_get_value(ups, CI_STESTI, &uval)) {
        ups->selftest = uval.iValue;
    }
#endif

    /* UPS_NAME */
    if (ups->upsname[0] == 0 && pusb_get_value(ups, CI_IDEN, &uval)) {
        if (ups->buf[0] != 0) {
            strncpy(ups->upsname, ups->buf, sizeof(ups->upsname)-1);
            ups->upsname[sizeof(ups->upsname)-1] = 0;
        }
    }

    /* model, firmware */
    if (pusb_get_value(ups, CI_UPSMODEL, &uval)) {
        char *p;
        /* Truncate Firmware info on APC Product string */
        if ((p=strchr(ups->buf, 'F')) && *(p+1) == 'W' && *(p+2) == ':') {
            *(p-1) = 0;
            strncpy(ups->firmrev, p+4, sizeof(ups->firmrev)-1);
            ups->firmrev[sizeof(ups->firmrev)-1] = 0;
            ups->UPS_Cap[CI_REVNO] = true;
        }
        strncpy(ups->upsmodel, ups->buf, sizeof(ups->upsmodel)-1);
        ups->upsmodel[sizeof(ups->upsmodel)-1] = 0;
        strncpy(ups->mode.long_name, ups->buf, sizeof(ups->mode.long_name)-1);
        ups->mode.long_name[sizeof(ups->mode.long_name)-1] = 0;
    }
       
    /* WAKEUP_DELAY */
    if (pusb_get_value(ups, CI_DWAKE, &uval)) {
        ups->dwake = (int)uval.dValue;
    }

    /* SLEEP_DELAY */
    if (pusb_get_value(ups, CI_DSHUTD, &uval)) {
        ups->dshutd = (int)uval.dValue;
    }

    /* LOW_TRANSFER_LEVEL */
    if (pusb_get_value(ups, CI_LTRANS, &uval)) {
        ups->lotrans = (int)uval.dValue;
    }

    /* HIGH_TRANSFER_LEVEL */
    if (pusb_get_value(ups, CI_HTRANS, &uval)) {
        ups->hitrans = (int)uval.dValue;
    }

    /* UPS_BATT_CAP_RETURN */
    if (pusb_get_value(ups, CI_RETPCT, &uval)) {
        ups->rtnpct = (int)uval.dValue;
    }


    /* LOWBATT_SHUTDOWN_LEVEL */
    if (pusb_get_value(ups, CI_DLBATT, &uval)) {
        ups->dlowbatt = (int)uval.dValue;
    }

    /* UPS_MANUFACTURE_DATE */
    if (pusb_get_value(ups, CI_MANDAT, &uval)) {
        sprintf(ups->birth, "%4d-%02d-%02d", (uval.iValue >> 9) + 1980,
                            (uval.iValue >> 5) & 0xF, uval.iValue & 0x1F);
    }

    /* Last UPS_BATTERY_REPLACE */
    if (pusb_get_value(ups, CI_BATTDAT, &uval)) {
        sprintf(ups->battdat, "%4d-%02d-%02d", (uval.iValue >> 9) + 1980,
                            (uval.iValue >> 5) & 0xF, uval.iValue & 0x1F);
    }
 
    /* APC_BATTERY_DATE */
    if (pusb_get_value(ups, CI_BattReplaceDate, &uval)) {
        v = uval.iValue;
        yy = ((v>>4) & 0xF)*10 + (v&0xF) + 2000;
        v >>= 8;
        dd = ((v>>4) & 0xF)*10 + (v&0xF);
        v >>= 8;
        mm = ((v>>4) & 0xF)*10 + (v&0xF);       
        sprintf(ups->battdat, "%4d-%02d-%02d", yy, mm, dd);
    }

    /* UPS_SERIAL_NUMBER */
    if (pusb_get_value(ups, CI_SERNO, &uval)) {
        char *p;
        astrncpy(ups->serial, ups->buf, sizeof(ups->serial));
        /*
         * If serial number has garbage, trash it.
         */
        for (p=ups->serial; *p; p++) {
            if (*p < ' ' || *p > 'z') {
               *ups->serial = 0;
               ups->UPS_Cap[CI_SERNO] = false;
            }
        }
    }

    /* Nominal output voltage when on batteries */
    if (pusb_get_value(ups, CI_NOMOUTV, &uval)) {
        ups->NomOutputVoltage = (int)uval.dValue;
    }

    /* Nominal battery voltage */
    if (pusb_get_value(ups, CI_NOMBATTV, &uval)) {
        ups->nombattv = uval.dValue;
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
     * We try various different ways to shutdown the UPS (i.e. killpower).
     * Some of these commands are not supported on all UPSes, but that
     * should cause no harm.
     */

    /*
     * First, set required battery capacity before startup to 0 so UPS will not
     * wait for the battery to charge before turning back on. Not all UPSes have
     * this capability, so this setting is allowed to fail. The value we program
     * here should be made configurable some day.
     */
    if (UPS_HAS_CAP(CI_APCBattCapBeforeStartup)) {
        func = "CI_APCBattCapBeforeStartup";
        if (!usb_write_int_to_ups(ups, CI_APCBattCapBeforeStartup, 0, func)) {
           Dmsg1(100, "Unable to set %s (not an error)\n", func);
        }
    }

    /*
     * BackUPS Pro uses an enumerated setting (reads percent in ASCII). The value
     * advances to the next higher setting by writing a '1' and to the next lower
     * setting when writing a '2'. The value wraps around when advanced past the
     * max or min setting.
     *
     * We walk the setting down to the minimum of 0.
     *
     * Credit goes to John Zielinski <grim@undead.cc> for figuring this out.
     */
    if (UPS_HAS_CAP(CI_BUPBattCapBeforeStartup)) {
        if (pusb_read_int_from_ups(ups, CI_BUPBattCapBeforeStartup, &val)) {
            func = "CI_BUPBattCapBeforeStartup";
            switch(val) {
                case 0x3930: /* 90% */
                    pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x3630: /* 60% */
                    pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x3135: /* 15% */
                    pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x3030: /* 00% */
                    break;
                    
                default:
                    Dmsg1(100, "Unknown BUPBattCapBeforeStartup value (%04x)\n", val);
                    break;
            }
        }
    }

    /*
     * Second, set the length of time to wait after power returns before starting
     * up. We set it to something pretty low, but it seems the UPS rounds this
     * value up to the nearest multiple of 60 seconds. Not all UPSes have this
     * capability, so this setting is allowed to fail.  The value we program
     * here should be made configurable some day.
     */
    if (UPS_HAS_CAP(CI_APCDelayBeforeStartup)) {
        func = "CI_APCDelayBeforeStartup";
        if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeStartup, 10, func)) {
           Dmsg1(100, "Unable to set %s (not an error)\n", func);
        }
    }

    /*
     * BackUPS Pro uses an enumerated setting (reads seconds in ASCII). The value
     * advances to the next higher setting by writing a '1' and to the next lower
     * setting when writing a '2'. The value wraps around when advanced past the
     * max or min setting.
     *
     * We walk the setting down to the minimum of 60.
     *
     * Credit goes to John Zielinski <grim@undead.cc> for figuring this out.
     */
    if (UPS_HAS_CAP(CI_BUPDelayBeforeStartup)) {
        if (pusb_read_int_from_ups(ups, CI_BUPDelayBeforeStartup, &val)) {
            func = "CI_BUPDelayBeforeStartup";
            switch(val) {
                case 0x363030: /* 600 sec */
                    pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x333030: /* 300 sec */
                    pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x313830: /* 180 sec */
                    pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
                    /* Falls thru... */
                case 0x3630:   /* 60 sec */
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
     * Alternately, if APCDelayBeforeShutdown is available, setting it will
     * start a countdown after which the UPS will hibernate.
     */
    if(!shutdown && UPS_HAS_CAP(CI_APCDelayBeforeShutdown)) {
        Dmsg0(000, "UPS appears to support BackUPS style shutdown.\n");   
        func = "CI_APCDelayBeforeShutdown";
        if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeShutdown, SHUTDOWN_DELAY, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            shutdown = 1;
        }
    }

    /*
     * SmartUPS shutdown
     *
     * If both DWAKE and DelayBeforeShutdown are available, trigger a hibernate
     * by writing DWAKE a few seconds longer than DelayBeforeShutdown. ORDER IS
     * IMPORTANT. The write to DelayBeforeShutdown starts both timers ticking
     * down and the UPS will hibernate when DelayBeforeShutdown hits zero.
     */
    if (!shutdown && UPS_HAS_CAP(CI_DWAKE) && UPS_HAS_CAP(CI_DelayBeforeShutdown)) {
        Dmsg0(000, "UPS appears to support SmartUPS style shutdown.\n");   
        func = "CI_DWAKE";
        if (!usb_write_int_to_ups(ups, CI_DWAKE, SHUTDOWN_DELAY+4, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            func = "CI_DelayBeforeShutdown";
            if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown, SHUTDOWN_DELAY, func)) {
               Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
               usb_write_int_to_ups(ups, CI_DWAKE, -1, "CI_DWAKE"); /* reset prev timer */
            }
            else {
                shutdown = 1;
            }
        }
    }

    /*
     * BackUPS Pro shutdown
     *
     * Here we see the BackUPS Pro further distinguish itself as having the
     * most broken firmware of any APC product yet. We have to trigger two magic
     * boolean flags using APC custom usages. First we hit BUPHibernate and 
     * follow that with a write to BUPSelfTest (!).
     *
     * Credit goes to John Zielinski <grim@undead.cc> for figuring this out.
     */
    if (!shutdown && UPS_HAS_CAP(CI_BUPHibernate) && UPS_HAS_CAP(CI_BUPSelfTest)) {
        Dmsg0(000, "UPS appears to support BackUPS Pro style shutdown.\n");   
        func = "CI_BUPHibernate";
        if (!pusb_write_int_to_ups(ups, CI_BUPHibernate, 1, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            func = "CI_BUPSelfTest";
            if (!pusb_write_int_to_ups(ups, CI_BUPSelfTest, 1, func)) {
               Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
               pusb_write_int_to_ups(ups, CI_BUPHibernate, 0, "CI_BUPHibernate");
            }
            else {
                shutdown = 1;
            }
        }
    }

    /*
     * All UPSes tested so far are covered by one of the above cases. However,
     * there are a couple other ways to shutdown.
     */

    /*
     * Misc method A
     *
     * Writing CI_DelayBeforeReboot starts a countdown timer, after which the UPS
     * will hibernate. If utility power is out, the UPS will stay hibernating until
     * power is restored. SmartUPSes seem to support this method, but PowerChute
     * uses the dual countdown method above, so we prefer that one. UPSes seem to
     * round the value up to 90 seconds if it is any lower. Note that the behavior
     * described here DOES NOT comply with the standard set out in the HID Usage
     * Tables for Power Devices spec. 
     */
    if(!shutdown && UPS_HAS_CAP(CI_DelayBeforeReboot)) {
        Dmsg0(000, "UPS appears to support DelayBeforeReboot style shutdown.\n");   
        func = "CI_DelayBeforeReboot";
        if (!usb_write_int_to_ups(ups, CI_DelayBeforeReboot, SHUTDOWN_DELAY, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            shutdown = 1;
        }
    }

    /*
     * Misc method B
     *
     * We can set CI_APCForceShutdown to true (it's a boolean flag). We have no
     * control over how long the UPS waits before turning off. Experimentally
     * it seems to be about 60 seconds. Some BackUPS models support this in
     * addition to the preferred BackUPS method above. It's included here
     * "just in case".
     */
    if(!shutdown && UPS_HAS_CAP(CI_APCForceShutdown)) {
        Dmsg0(000, "UPS appears to support ForceShutdown style shutdown.\n");   
        func = "CI_APCForceShutdown";
        if (!usb_write_int_to_ups(ups, CI_APCForceShutdown, 1, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            shutdown = 1;
        }
    }

    /*
     * Misc method C
     *
     * This one seems to turn the UPS off completely after a given delay.
     * The only way to power the UPS back on is to manually hit the power button.
     */
    if(!shutdown && UPS_HAS_CAP(CI_DelayBeforeShutdown)){
        Dmsg0(000, "UPS appears to support DelayBeforeShutdown style shutdown.\n");   
        func = "CI_DelayBeforeShutdown";
        if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown, SHUTDOWN_DELAY, func)) {
           Dmsg1(000, "Kill power function \"%s\" failed.\n", func);   
        }
        else {
            shutdown = 1;
        }
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
     * Note that there are a couple other CIs that look interesting for shutdown,
     * but they're not what we want.
     *
     * APCShutdownAfterDelay: Tells the UPS how many seconds to wait after power
     *     goes out before asserting ShutdownRequested (see next item).
     *
     * CI_ShutdownRequested: This is an indicator from the UPS to the server
     *     that it would like the server to begin shutting down. In conjunction
     *     with APCShutdownAfterDelay this can be used to offload the decision of
     *     when to shut down the server to the UPS.
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
       for (i=0; i<exponent; i++) {
          val = val / 10;
       }
       return val;
    } else {
       for (i=0; i<exponent; i++) {
          val = val * 10;
       }
    }
    return val;
}
