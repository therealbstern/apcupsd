/*
 *
 *  Interface for apcupsd to Linux HIDDEV USB driver.
 *
 *   Kern Sibbald, June MMI 
 *
 *   Parts of this code (especially the initialization and walking
 *     the reports) were derived from a test program hid-ups.c by:    
 *
 *     Copyright (c) 2001 Vojtech Pavlik <vojtech@ucw.cz>
 *     Copyright (c) 2001 Paul Stewart <hiddev@wetlogic.net>
 *
 */


#include "apc.h"
#include "usb.h"
#include <math.h>

#include <asm/types.h>
#include <linux/hiddev.h>

/* Forware referenced function */
static int find_usb_application(UPSINFO *ups);

extern UPSCOMMANDS cmd[];
extern UPSCMDMSG cmd_msg[];


/* USB Vendor ID's */
#define Vendor_APC 0x51D
#define Vendor_MGE 0x463

static int linkcheck = FALSE;
#define LINK_RETRY_INTERVAL 5	     /* retry every 5 seconds */

/* Various known USB codes */
#define UPS_USAGE   0x840000
#define UPS_VOLTAGE 0x840030
#define UPS_OUTPUT  0x84001c
#define UPS_BATTERY 0x840012

/* These are the data_type expected for our know_info */
#define T_NONE	   0		      /* No units */
#define T_INDEX    1		      /* String index */
#define T_CAPACITY 2		      /* Capacity (usually %) */
#define T_BITS	   3		      /* bit field */
#define T_UNITS    4		      /* use units/exponent field */
#define T_DATE	   5		      /* date */
#define T_APCDATE  6		      /* APC date */

/* These are the resulting value types returned */     
#define V_DOUBLE   1		     /* double */ 
#define V_STRING   2		     /* string pointer */
#define V_INTEGER  3		     /* integer */

/* These are the desired Physical usage values we want */
#define P_ANY	  0		     /* any value */
#define P_OUTPUT  0x84001c	     /* output values */
#define P_BATTERY 0x840012	     /* battery values */
#define P_INPUT   0x84001a	     /* input values */

/* No Command Index, don't save this value */
#define CI_NONE -1

/*
 * This table is used when walking through the USB reports to see
 *   what information found in the UPS that we want. If the
 *   usage_code and the physical code match, then we make an
 *   entry in the command index table containing the usage information
 *   provided by the UPS as well as the data type from this table.
 *   Entries in the table with ci == CI_NONE are not used, for the
 *   moment, they are retained just so they are not forgotten.
 */
struct s_known_info {
    int ci;			      /* command index */
    unsigned usage_code;	      /* usage code */
    unsigned physical;		      /* physical usage */
    int data_type;		      /* data type expected */
} known_info[] = {
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
    { CI_MANDAT,	      0x850085, P_ANY,	  T_DATE    },	/* ManufactureDate */
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
    { CI_STATUS,	      0xFF860060, P_ANY,  T_BITS    },	/* APCStatusFlag */
    { CI_DSHUTD,	      0xFF860076, P_ANY,  T_UNITS   },	/* APCShutdownAfterDelay */
    { CI_NONE,		      0xFF860005, P_ANY,  T_NONE    },	/* APCGeneralCollection */
    { CI_APCForceShutdown,    0xFF86007C, P_ANY,  T_NONE    },	/* APCForceShutdown */
    { CI_NONE,		      0xFF860072, P_ANY,  T_NONE    },	/* APCPanelTest */
    { CI_BattReplaceDate,     0xFF860016, P_ANY,  T_APCDATE },	/* APCBattReplaceDate */
    { CI_NONE,		      0xFF860042, P_ANY,  T_NONE    },	/* APC_UPS_FirmwareRevision */
    { CI_NONE,		      0xFF860079, P_ANY,  T_NONE    },	/* APC_USB_FirmwareRevision */
};
#define KNOWN_INFO_SZ (sizeof(known_info)/sizeof(known_info[0]))


/*
 * When we are traversing the USB reports given by the UPS
 *   and we find an entry corresponding to an entry in the 
 *   known_info table above, we make the following USB_INFO 
 *   entry in the info table of our private data.
 */
typedef struct s_usb_info {
   unsigned physical;		      /* physical value wanted */
   unsigned usage_code; 	      /* usage code wanted */
   unsigned unit_exponent;	      /* exponent */
   unsigned unit;		      /* units */
   int data_type;		      /* data type */
   struct hiddev_usage_ref uref;      /* usage reference */
   int value_type;		      /* Type of returned value */
   double dValue;		      /* Value if double */
   char *sValue;		      /* Value if string */
   int iValue;			      /* integer value */
   char *UnitName;		      /* Name of units */
} USB_INFO;


/*
 * This "private" structure is returned to us in the driver private
 *   field, and allows us to get to all the info we keep on
 *   each UPS. The info field is malloced for each command
 *   we want and the UPS has.
 */
typedef struct s_usb_data {
   int fd;			      /* Our UPS fd when open */
   char orig_device[MAXSTRING];       /* Original port specification */
   short vendor;		      /* UPS vendor id */
   time_t debounce;		      /* last event time for debounce */
   USB_INFO *info[CI_MAXCI+1];	      /* Info pointers for each command */
} USB_DATA;


static void reinitialize_private_structure(UPSINFO *ups)
{
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
    int k;
    Dmsg0(200, "Reinitializing private structure.\n");
    /*
     * We are being reinitialized, so clear the Cap
     *	 array, and release previously allocated memory.
     */
    for (k=0; k <= CI_MAXCI; k++) {
	ups->UPS_Cap[k] = FALSE;
	if (my_data->info[k] != NULL) {
	    free(my_data->info[k]);
	    my_data->info[k] = NULL;
	}
    }
}

/*
 * Internal routine to open the device and ensure that there is
 * a UPS application on the line.  This routine may be called
 * many times because the device may be unplugged and plugged
 * back in -- the joys of USB devices.
 */
static int open_usb_device(UPSINFO *ups)
{
    char *p, *q, *r;
    int start, end;
    char name[MAXSTRING];
    char devname[MAXSTRING];
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

    if (my_data->orig_device[0] == 0) {
       strcpy(my_data->orig_device, ups->device);
    }
    strcpy(name, my_data->orig_device);
    p = strchr(name, '[');
    if (p) {			      /* range specified */
       q = strchr(p+1, '-');
       if (q) {      
	  *q++ = 0;		      /* terminate first number */
          r = strchr(q, ']');
       } else {
	  r = NULL;
       }
       if (!q || !r) {
          Error_abort0("Bad DEVICE configuration range specifed.\n");
       }
       *r = 0;			      /* terminate second number */
       start = atoi(p+1);	      /* scan first number */
       end = atoi(q);		      /* scan second number */
       if (start > end || start < 0) {
          Error_abort0("Bad DEVICE configuration range specifed.\n");
       }
       /* Concatenate %d */
       *p++ = '%';
       *p++ = 'd';
       *p++ = 0;
    } else {
       start = end = 1;
    }

    /*
     * Note, we set ups->fd here so the "core" of apcupsd doesn't
     *	think we are a slave, which is what happens when it is -1
     *	Internally, we use the fd in our own private space   
     */
    ups->fd = 1;
    for ( ; start <= end; start++) {
	sprintf(devname, name, start);
    
	/* Open the device port */
	if ((my_data->fd = open(devname, O_RDWR | O_NOCTTY)) < 0) {
	   continue;
	}
	if (!find_usb_application(ups)) {
	   close(my_data->fd);
	   my_data->fd = -1;
	   continue;
	}
	break;
    }
    if (my_data->fd >= 0) {
       strcpy(ups->device, devname);
       return 1;
    } else {
       ups->device[0] = 0;
       return 0;
    }
}

/*********************************************************************
 * 
 * Called if there is an ioctl() or read() error, we close() and
 *  re open() the port since the device was probably unplugged.
 */
static int usb_link_check(UPSINFO *ups)
{
    int comm_err = TRUE;
    int tlog;
    int once = TRUE;
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

    if (linkcheck) {
	return 0;
    }
    linkcheck = TRUE;		  /* prevent recursion */

    UPS_SET(UPS_COMMLOST);
    Dmsg0(200, "link_check comm lost\n");

    /* Don't warn until we try to get it at least 2 times and fail */
    for (tlog=LINK_RETRY_INTERVAL*2; comm_err; tlog -= (LINK_RETRY_INTERVAL)) {
	if (tlog <= 0) {
	    tlog = 10 * 60; /* notify every 10 minutes */
	    log_event(ups, cmd_msg[CMDCOMMFAILURE].level,
		      cmd_msg[CMDCOMMFAILURE].msg);
	    if (once) {    /* execute script once */
		execute_command(ups, cmd[CMDCOMMFAILURE]);
		once = FALSE;
	    }
	}
	/* Retry every LINK_RETRY_INTERVAL seconds */
	sleep(LINK_RETRY_INTERVAL);
	if (my_data->fd >= 0) {
	   close(my_data->fd);
	   my_data->fd = -1;
	   reinitialize_private_structure(ups);
	}
	if (open_usb_device(ups) && usb_ups_get_capabilities(ups) &&
	    usb_ups_read_static_data(ups)) {
	   comm_err = FALSE;
	} else {
	   continue;
	}
    }

    if (!comm_err) {
	generate_event(ups, CMDCOMMOK);
	UPS_CLEAR(UPS_COMMLOST);
        Dmsg0(200, "link check comm OK.\n");
    }
    linkcheck = FALSE;
    return 1;
}


/* 
 * See if the USB device speaks UPS language
 */
static int find_usb_application(UPSINFO *ups)
{
    int i, ret;
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

    for (i=0; (ret = ioctl(my_data->fd, HIDIOCAPPLICATION, i)) > 0; i++) {
       if ((ret & 0xffff000) == (UPS_USAGE & 0xffff0000)) {
	  return 1;
       }
    }
    return 0;
}

/*
 * Get a field value
 */
static int get_value(UPSINFO *ups, int ci, USB_INFO *uinfo)
{
    struct hiddev_string_descriptor sdesc;
    struct hiddev_report_info rinfo;
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
    USB_INFO *info;
    int exponent;

    if (!ups->UPS_Cap[ci] || !my_data->info[ci]) {
	return 0;		      /* UPS does not have capability */
    }
    info = my_data->info[ci];	      /* point to our info structure */
    rinfo.report_type = info->uref.report_type;
    rinfo.report_id = info->uref.report_id;
    if (ioctl(my_data->fd, HIDIOCGREPORT, &rinfo) < 0) {  /* update Report */
       return 0;
    }
    if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->uref) < 0) {  /* update UPS value */
       return 0;
    }
    exponent = info->unit_exponent;
    if (exponent > 7) {
       exponent = exponent - 16;
    }
    if (info->data_type == T_INDEX) { /* get string */
	if (info->uref.value == 0) {
	   return 0;
	}
	sdesc.index = info->uref.value;
	if (ioctl(my_data->fd, HIDIOCGSTRING, &sdesc) < 0) {
	    return 0;
	}
	strncpy(ups->buf, sdesc.value, ups->buf_len);
	info->value_type = V_STRING;
	info->sValue = ups->buf;
    } else if (info->data_type == T_UNITS) {
	info->value_type = V_DOUBLE;
	switch (info->unit) {
	case 0x00F0D121:
            info->UnitName = "Volts";
	    exponent -= 7;	      /* remove bias */
	    break;
	case 0x00100001:
	    exponent += 2;	      /* remove bias */
            info->UnitName = "Amps";
	    break;
	case 0xF001:
            info->UnitName = "Hertz";
	    break;
	case 0x1001:
            info->UnitName = "Seconds";
	    break;
	case 0xD121:
	    exponent -= 7;	      /* remove bias */
            info->UnitName = "Watts";
	    break;
	case 0x010001:
            info->UnitName = "Degrees K";
	    break;
	case 0x0101001:
            info->UnitName = "AmpSecs";
	    if (exponent == 0) {
	       info->dValue = info->uref.value;
	    } else {
	       info->dValue = ((double)info->uref.value) * pow(10, exponent);
	    }
	    break;
	default:
            info->UnitName = "";
	    info->value_type = V_INTEGER;
	    info->iValue = info->uref.value; 
	    break;
	}
	if (exponent == 0) {
	   info->dValue = info->uref.value;
	} else {
	   info->dValue = ((double)info->uref.value) * pow(10, exponent);
	}
    } else {   /* should be T_NONE */
        info->UnitName = "";
	info->value_type = V_INTEGER;
	info->iValue = info->uref.value; 
	if (exponent == 0) {
	   info->dValue = info->uref.value;
	} else {
	   info->dValue = ((double)info->uref.value) * pow(10, exponent);
	}
        Dmsg3(200, "Def val=%d exp=%d dVal=%f\n", info->uref.value,
	      exponent, info->dValue);
    }
    memcpy(uinfo, info, sizeof(USB_INFO));
    return 1;
}

/*
 * Read UPS events. I.e. state changes.
 */
int usb_ups_check_state(UPSINFO *ups)
{
    int i;
    int retval;
    struct hiddev_event ev[64];
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;


    for ( ;; ) {
	fd_set rfds;
	struct timeval tv;
 
	FD_ZERO(&rfds);
	FD_SET(my_data->fd, &rfds);
	tv.tv_sec = ups->wait_time;
	tv.tv_usec = 0;
	   
	errno = 0;
	retval = select((my_data->fd)+1, &rfds, NULL, NULL, &tv);

	switch (retval) {
	case 0: /* No chars available in TIMER seconds. */
	    return 0;
	case -1:
	    if (errno == EINTR || errno == EAGAIN) { /* assume SIGCHLD */
		continue;
	    }
            Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
	    usb_link_check(ups);      /* link is down, wait */
	    return 0;
	default:
	    break;
	}


	do {
	   retval = read(my_data->fd, &ev, sizeof(ev));
	} while (retval == -1 && (errno == EAGAIN || errno == EINTR));

	if (retval < 0) {	      /* error */
           Dmsg1(200, "read error: ERR=%s\n", strerror(errno));
	   usb_link_check(ups);       /* notify that link is down, wait */
	   return 0;
	}

	if (retval == 0 || retval < (int)sizeof(ev[0])) {
	   return 0;
	}

	/* Walk through events
	 * Detected:
	 *   Discharging
	 *   BelowRemCapLimit
	 *   ACPresent
	 *   RemainingCapacity
	 *   RunTimeToEmpty
	 *   ShutdownImminent
	 *   NeedReplacement
	 *
	 * Perhaps Add:
	 *   APCStatusFlag
	 *   Charging
	 */   
	if (my_data->debounce) {
	   /* Wait 6 seconds before returning next sample.
	    * This eliminates the annoyance of detecting
	    * transitions to battery that last less than
	    * 6 seconds.
 */   
	   sleep(6 - (time(NULL) - my_data->debounce));
	   my_data->debounce = 0;
	}
	write_lock(ups);
	for (i=0; i < (retval/(int)sizeof(ev[0])); i++) {
	    if (ev[i].hid == ups->UPS_Cmd[CI_Discharging]) {
		/* If first time on batteries, debounce */
		if (!UPS_ISSET(UPS_ONBATT) && ev[i].value) {
		   my_data->debounce = time(NULL);
		}
		if (ev[i].value) {
	    UPS_CLEAR_ONLINE();
	} else {
	    UPS_SET_ONLINE();
	}
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_BelowRemCapLimit]) {
	    if (ev[i].value) {
		UPS_SET(UPS_BATTLOW);
	    } else {
		UPS_CLEAR(UPS_BATTLOW);
	    }
                Dmsg1(200, "UPS_BATTLOW = %d\n", UPS_ISSET(UPS_BATTLOW));
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_ACPresent]) {
		/* If first time on batteries, debounce */
		if (!UPS_ISSET(UPS_ONBATT) && !ev[i].value) {
		   my_data->debounce = time(NULL);
		}
		if (!ev[i].value) {
	    UPS_CLEAR_ONLINE();
	} else {
	    UPS_SET_ONLINE();
	}
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_RemainingCapacity]) {
		ups->BattChg = ev[i].value;
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_RunTimeToEmpty]) {
		if (my_data->vendor == Vendor_APC) {
		    ups->TimeLeft = ((double)ev[i].value) / 60;  /* seconds */
		} else {
		    ups->TimeLeft = ev[i].value;  /* minutes */
		}
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_NeedReplacement]) {
	    if (ev[i].value) {
		UPS_SET(UPS_REPLACEBATT);
	    } else {
		UPS_CLEAR(UPS_REPLACEBATT);
	    }
	    } else if (ev[i].hid == ups->UPS_Cmd[CI_ShutdownImminent]) {
	    if (ev[i].value) {
		UPS_SET(UPS_SHUTDOWNIMM);
	    } else {
		UPS_CLEAR(UPS_SHUTDOWNIMM);
	    }
            Dmsg1(200, "ShutdownImminent=%d\n", UPS_ISSET(UPS_SHUTDOWNIMM));
	}
            Dmsg1(200, "Status=%d\n", ups->Status);
	}
	write_unlock(ups);
	break;
    }
    return 1;
}

/*
 * Open usb port
 *   This is called once by the core code and is the first 
 *   routine called.
 */
int usb_ups_open(UPSINFO *ups)
{
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

    write_lock(ups);
    if (my_data == NULL) {
       my_data = (USB_DATA *)malloc(sizeof(USB_DATA));
       if (my_data == NULL) {
          log_event(ups, LOG_ERR, "Out of memory.");
	  write_unlock(ups);
	  exit(1);
       }
       memset(my_data, 0, sizeof(USB_DATA));
       ups->driver_internal_data = my_data;
    } else {
       reinitialize_private_structure(ups);
    }
    if (my_data->orig_device[0] == 0) {
       strcpy(my_data->orig_device, ups->device);
    }
    if (!open_usb_device(ups)) {
	write_unlock(ups);
        Error_abort1(_("Cannot open UPS device %s\n"), ups->device);
    }
    UPS_CLEAR(UPS_SLAVE);
    write_unlock(ups);
    return 1;
}

int usb_ups_setup(UPSINFO *ups) {
    /*
     * Seems that there is nothing to do.
     */
    return 1;
}

/*
 * This is the last routine called from apcupsd core code 
     */
int usb_ups_close(UPSINFO *ups)
{
    write_lock(ups);
    if (ups->driver_internal_data) {
	free(ups->driver_internal_data);
	ups->driver_internal_data = NULL;
    }
    write_unlock(ups);
    return 1;
}

/*
 * Setup capabilities structure for UPS
 */
int usb_ups_get_capabilities(UPSINFO *ups)
{
    int rtype[2] = { HID_REPORT_TYPE_FEATURE, HID_REPORT_TYPE_INPUT};
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
    struct hiddev_report_info rinfo;
    struct hiddev_field_info finfo;
    struct hiddev_usage_ref uref;
    int i, j, k, n;

    write_lock(ups);
    if (ioctl(my_data->fd, HIDIOCINITREPORT, 0) < 0) {
       write_unlock(ups);
       Error_abort1("Cannot init USB HID report. ERR=%s\n", strerror(errno));
    }

    /*
     * Walk through all available reports and determine
     * what information we can use.
     */
    for (n=0; n < (int)sizeof(rtype); n++) {
	rinfo.report_type = rtype[n];
	rinfo.report_id = HID_REPORT_ID_FIRST;
	while (ioctl(my_data->fd, HIDIOCGREPORTINFO, &rinfo) >= 0) {
	    for (i = 0; i < (int)rinfo.num_fields; i++) { 

		memset(&finfo, 0, sizeof(finfo));
		finfo.report_type = rinfo.report_type;
		finfo.report_id = rinfo.report_id;
		finfo.field_index = i;
		if (ioctl(my_data->fd, HIDIOCGFIELDINFO, &finfo) < 0) {
		    continue;
		}
		memset(&uref, 0, sizeof(uref));
		for (j = 0; j < (int)finfo.maxusage; j++) {
		    uref.report_type = finfo.report_type;
		    uref.report_id = finfo.report_id;
		    uref.field_index = i;
		    uref.usage_index = j;
		    if (ioctl(my_data->fd, HIDIOCGUCODE, &uref) < 0) {
		       continue;
		    }
		    ioctl(my_data->fd, HIDIOCGUSAGE, &uref);
		    /*
                     * We've got a UPS usage entry, now walk down our
		     * know_info table and see if we have a match.
		     * If so, allocate a new entry for it.
		     */
		    for (k=0; k <= (int)KNOWN_INFO_SZ; k++) {
			USB_INFO *info;
			int ci = known_info[k].ci;

			if (ci != CI_NONE &&
			     !ups->UPS_Cap[ci] &&
			     uref.usage_code == known_info[k].usage_code &&
			     (known_info[k].physical == P_ANY ||
			      known_info[k].physical == finfo.physical)) {
			    ups->UPS_Cap[ci] = TRUE;
			    /* ***FIXME*** remove UPS_Cmd */
			    ups->UPS_Cmd[ci] = uref.usage_code;
			    info = (USB_INFO *)malloc(sizeof(USB_INFO));
			    if (!info) {
			       write_unlock(ups);
                               Error_abort0(_("Out of memory.\n"));
			    }
			    my_data->info[ci] = info;
			    info->physical = finfo.physical;
			    info->usage_code = uref.usage_code;
			    info->unit_exponent = finfo.unit_exponent;
			    info->unit = finfo.unit;
			    info->data_type = known_info[k].data_type;
			    memcpy(&info->uref, &uref, sizeof(uref));
                            Dmsg2(200, "Got ci=%d, usage=0x%x\n",ci,
			       known_info[k].usage_code);
			    break;
			}
		    }
		}
	    }
	    rinfo.report_id |= HID_REPORT_ID_NEXT;
	}
    }
    ups->UPS_Cap[CI_STATUS] = TRUE;   /* we have status flag */
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
    USB_INFO uinfo;
    struct hiddev_devinfo dinfo;
    int v, yy, mm, dd;
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

    write_lock(ups);

    if (ioctl(my_data->fd, HIDIOCGDEVINFO, &dinfo) >= 0) {
       my_data->vendor = dinfo.vendor;
    }
    
#ifdef xxxx
    /* ALARM_STATUS */
    if (ups->UPS_Cap[CI_DALARM])
	strncpy(ups->beepstate, smart_poll(ups->UPS_Cmd[CI_DALARM], ups), 
	       sizeof(ups->beepstate));
    /* UPS_SELFTEST Interval */
    if (get_value(ups, CI_STESTI, &uinfo)) {
	ups->selftest = uinfo.uref.value;
    }
#endif


    /* UPS_NAME */
    if (ups->upsname[0] == 0 && get_value(ups, CI_IDEN, &uinfo)) {
	if (ups->buf[0] != 0) {
	    strncpy(ups->upsname, ups->buf, sizeof(ups->upsname)-1);
	    ups->upsname[sizeof(ups->upsname)-1] = 0;
	}
    }

    /* model, firmware */
    if (get_value(ups, CI_UPSMODEL, &uinfo)) {
	char *p;
	/* Truncate Firmware info on APC Product string */
        if ((p=strchr(ups->buf, 'F')) && *(p+1) == 'W' && *(p+2) == ':') {
	    *(p-1) = 0;
	    strncpy(ups->firmrev, p+4, sizeof(ups->firmrev)-1);
	    ups->firmrev[sizeof(ups->firmrev)-1] = 0;
	    ups->UPS_Cap[CI_REVNO] = TRUE;
	}
	strncpy(ups->upsmodel, ups->buf, sizeof(ups->upsmodel)-1);
	ups->upsmodel[sizeof(ups->upsmodel)-1] = 0;
	strncpy(ups->mode.long_name, ups->buf, sizeof(ups->mode.long_name)-1);
	ups->mode.long_name[sizeof(ups->mode.long_name)-1] = 0;
    }
       

    /* WAKEUP_DELAY */
    if (get_value(ups, CI_DWAKE, &uinfo)) {
	ups->dwake = (int)uinfo.dValue;
    }

    /* SLEEP_DELAY */
    if (get_value(ups, CI_DSHUTD, &uinfo)) {
	ups->dshutd = (int)uinfo.dValue;
    }

    /* LOW_TRANSFER_LEVEL */
    if (get_value(ups, CI_LTRANS, &uinfo)) {
	ups->lotrans = (int)uinfo.dValue;
    }

    /* HIGH_TRANSFER_LEVEL */
    if (get_value(ups, CI_HTRANS, &uinfo)) {
	ups->hitrans = (int)uinfo.dValue;
    }

    /* UPS_BATT_CAP_RETURN */
    if (get_value(ups, CI_RETPCT, &uinfo)) {
	ups->rtnpct = (int)uinfo.dValue;
    }


    /* LOWBATT_SHUTDOWN_LEVEL */
    if (get_value(ups, CI_DLBATT, &uinfo)) {
	ups->dlowbatt = (int)uinfo.dValue;
    }


    /* UPS_MANUFACTURE_DATE */
    if (get_value(ups, CI_MANDAT, &uinfo)) {
        sprintf(ups->birth, "%4d-%02d-%02d", (uinfo.uref.value >> 9) + 1980,
			    (uinfo.uref.value >> 5) & 0xF, uinfo.uref.value & 0x1F);
    }

    /* Last UPS_BATTERY_REPLACE */
    if (get_value(ups, CI_BATTDAT, &uinfo)) {
        sprintf(ups->battdat, "%4d-%02d-%02d", (uinfo.uref.value >> 9) + 1980,
			    (uinfo.uref.value >> 5) & 0xF, uinfo.uref.value & 0x1F);
    }
 
    /* APC_BATTERY_DATE */
    if (get_value(ups, CI_BattReplaceDate, &uinfo)) {
	v = uinfo.uref.value;
	yy = ((v>>4) & 0xF)*10 + (v&0xF) + 2000;
	v >>= 8;
	dd = ((v>>4) & 0xF)*10 + (v&0xF);
	v >>= 8;
	mm = ((v>>4) & 0xF)*10 + (v&0xF);	
        sprintf(ups->battdat, "%4d-%02d-%02d", yy, mm, dd);
    }


    /* UPS_SERIAL_NUMBER */
    if (get_value(ups, CI_SERNO, &uinfo)) {
	char *p;
	strncpy(ups->serial, ups->buf, sizeof(ups->serial));
	/*
	 * If serial number has garbage, trash it.
	 */
	for (p=ups->serial; *p; p++) {
            if (*p < ' ' || *p > 'z') {
	       *ups->serial = 0;
	       ups->UPS_Cap[CI_SERNO] = FALSE;
	    }
	}
    }


    /* Nominal output voltage when on batteries */
    if (get_value(ups, CI_NOMOUTV, &uinfo)) {
	ups->NomOutputVoltage = (int)uinfo.dValue;
    }

    /* Nominal battery voltage */
    if (get_value(ups, CI_NOMBATTV, &uinfo)) {
	ups->nombattv = uinfo.dValue;
    }
    write_unlock(ups);
    return 1;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every 5 seconds to get
 *  a current idea of what the UPS is doing.
 */
int usb_ups_read_volatile_data(UPSINFO *ups)
{
    USB_INFO uinfo;

    time(&ups->poll_time);	  /* save time stamp */

    write_lock(ups);

    /* UPS_STATUS -- this is the most important status for apcupsd */

    ups->Status &= ~0xff;	     /* Clear APC part of Status */
    if (get_value(ups, CI_STATUS, &uinfo)) {
	ups->Status |= (uinfo.iValue & 0xff); /* set new APC part */
    } else {
	/* No APC Status value, well, fabricate one */
	if (get_value(ups, CI_ACPresent, &uinfo) && uinfo.uref.value) {
	    UPS_SET_ONLINE();
	}
	if (get_value(ups, CI_Discharging, &uinfo) && uinfo.uref.value) {
	    UPS_CLEAR_ONLINE();
	}
	if (get_value(ups, CI_BelowRemCapLimit, &uinfo) && uinfo.uref.value) {
	UPS_SET(UPS_BATTLOW);
            Dmsg1(200, "BelowRemCapLimit=%d\n", uinfo.uref.value);
	}
	if (get_value(ups, CI_RemTimeLimitExpired, &uinfo) && uinfo.uref.value) {
	UPS_SET(UPS_BATTLOW);
            Dmsg0(200, "RemTimeLimitExpired\n");
	}
	if (get_value(ups, CI_ShutdownImminent, &uinfo) && uinfo.uref.value) {
	UPS_SET(UPS_BATTLOW);
            Dmsg0(200, "ShutdownImminent\n");
	}
	if (get_value(ups, CI_Boost, &uinfo) && uinfo.uref.value) {
	    UPS_SET(UPS_SMARTBOOST);
	}
	if (get_value(ups, CI_Trim, &uinfo) && uinfo.uref.value) {
	    UPS_SET(UPS_SMARTTRIM);
	}
	if (get_value(ups, CI_Overload, &uinfo) && uinfo.uref.value) {
	    UPS_SET(UPS_OVERLOAD);
	}
	if (get_value(ups, CI_NeedReplacement, &uinfo) && uinfo.uref.value) {
	    UPS_SET(UPS_REPLACEBATT);
	}
    }

    /* LINE_VOLTAGE */
    if (get_value(ups, CI_VLINE, &uinfo)) {
	ups->LineVoltage = uinfo.dValue;
    }

    /* OUTPUT_VOLTAGE */
    if (get_value(ups, CI_VOUT, &uinfo)) {
	ups->OutputVoltage = uinfo.dValue;
    }

    /* BATT_FULL Battery level percentage */
    if (get_value(ups, CI_BATTLEV, &uinfo)) {
	ups->BattChg = uinfo.dValue;
    }

    /* BATT_VOLTAGE */
    if (get_value(ups, CI_VBATT, &uinfo)) {
	ups->BattVoltage = uinfo.dValue;
    }

    /* UPS_LOAD */
    if (get_value(ups, CI_LOAD, &uinfo)) {
	ups->UPSLoad = uinfo.dValue;
    }

    /* LINE_FREQ */
    if (get_value(ups, CI_FREQ, &uinfo)) {
	ups->LineFreq = uinfo.dValue;
    }

    /* UPS_RUNTIME_LEFT */
    if (get_value(ups, CI_RUNTIM, &uinfo)) {
	ups->TimeLeft = uinfo.dValue / 60;   /* convert to minutes */
    }

    /* UPS_TEMP */
    if (get_value(ups, CI_ITEMP, &uinfo)) {
	ups->UPSTemp = uinfo.dValue - 273.15; /* convert to deg C. */
    }

    /*	Humidity percentage */ 
    if (get_value(ups, CI_HUMID, &uinfo)) {
	ups->humidity = uinfo.dValue;
    }

    /*	Ambient temperature */ 
    if (get_value(ups, CI_ATEMP, &uinfo)) {
	ups->ambtemp = uinfo.dValue;
    }

    /* Self test results */
    if (get_value(ups, CI_ST_STAT, &uinfo)) {
	switch (uinfo.uref.value) {
	case 1: 		      /* passed */
           strcpy(ups->X, "OK");
	   break;
	case 2: 		      /* Warning */
           strcpy(ups->X, "WN");
	   break;
	case 3: 		      /* Error */
	case 4: 		      /* Aborted */
           strcpy(ups->X, "NG");
	   break;
	case 5: 		      /* In progress */
           strcpy(ups->X, "IP");
	   break;
	case 6: 		      /* None */
           strcpy(ups->X, "NO");
	   break;
	default:
	   break;
	}
    }

    write_unlock(ups);

    return 1;
}

static int write_int_to_ups(UPSINFO *ups, int ci, int value, char *name)
{
    struct hiddev_report_info rinfo;
    struct hiddev_field_info finfo;
    USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
    USB_INFO *info;
    int old_value, new_value;
    int i;

    errno = 0; 
    if (ups->UPS_Cap[ci] && my_data->info[ci]) {
	info = my_data->info[ci];	  /* point to our info structure */
	rinfo.report_type = info->uref.report_type;
	rinfo.report_id = info->uref.report_id;
	if (ioctl(my_data->fd, HIDIOCGREPORT, &rinfo) < 0) {  /* update Report */
            Dmsg1(000, "HIDIOCGREPORT for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
	rinfo.report_type = info->uref.report_type;
	rinfo.report_id = info->uref.report_id;
	if (ioctl(my_data->fd, HIDIOCGREPORTINFO, &rinfo) < 0) {  /* get Report */
            Dmsg1(000, "HIDIOCGREPORT for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
        Dmsg1(000, "REPORTINFO num_fields=%d\n", rinfo.num_fields);
	finfo.report_type = info->uref.report_type;
	finfo.report_id = info->uref.report_id;
	if (ioctl(my_data->fd, HIDIOCGFIELDINFO, &finfo) < 0) {  /* Get field info */
            Dmsg1(000, "HIDIOCGFIELDINFO for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
        Dmsg7(000, "FIELDINFO type=%d id=%d index=%d logical_min=%d \n\
logical_max=%d exponent=%d unit=0x%x\n",
	    finfo.report_type, finfo.report_id, finfo.field_index,
	    finfo.logical_minimum, finfo.logical_maximum, 
	    finfo.unit_exponent, finfo.unit);
        Dmsg3(000, "GUSAGE type=%d id=%d index=%d\n", info->uref.report_type,
	   info->uref.report_id, info->uref.field_index);
	if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->uref) < 0) {  /* get UPS value */
            Dmsg1(000, "HIDIOCSUSAGE for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
	old_value = info->uref.value;
	info->uref.value = value;
        Dmsg3(000, "SUSAGE type=%d id=%d index=%d\n", info->uref.report_type,
	   info->uref.report_id, info->uref.field_index);
	if (ioctl(my_data->fd, HIDIOCSUSAGE, &info->uref) < 0) {  /* update UPS value */
            Dmsg1(000, "HIDIOCSUSAGE for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
	for (i=0; i < 1; i++) {
	   if (ioctl(my_data->fd, HIDIOCSREPORT, &rinfo) < 0) {  /* update Report */
               Dmsg1(000, "HIDIOCSREPORT for shutdown failed. ERR=%s\n", strerror(errno));
	       return 0;
	   }
	}
	if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->uref) < 0) {  /* get UPS value */
            Dmsg1(000, "HIDIOCSUSAGE for shutdown failed. ERR=%s\n", strerror(errno));
	    return 0;
	}
	new_value = info->uref.value;
        Dmsg2(100, "shutdown ci=%d value=%d OK.\n", ci, value);
        Dmsg4(000, "%s before=%d set=%d after=%d\n", name, old_value, value, new_value);
	return 1;
    }
    return 0;
}

#define CI_APCShutdownAfterDelay CI_DSHUTD

int usb_ups_kill_power(UPSINFO *ups)
{

    if (!usb_ups_get_capabilities(ups)) {
       return 0;
    }

    write_int_to_ups(ups, CI_DelayBeforeShutdown, 20, "CI_DelayBeforeShutdown");

    write_int_to_ups(ups, CI_ShutdownRequested, 1, "CI_ShutdownRequested");

    write_int_to_ups(ups, CI_APCShutdownAfterDelay, 30, "CI_APCShutdownAfterDelay");

    write_int_to_ups(ups, CI_APCForceShutdown, 1, "CI_APCForceShutdown");

  /******* DEBUG testing */
    write_int_to_ups(ups, CI_WarningCapacityLimit, 40, "CIWarningCapacityLimit");
    write_int_to_ups(ups, CI_RemCapLimit, 20, "CI_RemCapLibmit");
   

    Dmsg0(200, "Leave usb_ups_kill_power\n");
    Dmsg0(000, "Kill power does not yet work. Please ignore the debug output.\n");
    return 1;
}

int usb_ups_program_eeprom(UPSINFO *ups) 
{
#if 0
    printf(_("This model cannot be configured.\n"));
#endif
    return 0;
}

int usb_ups_entry_point(UPSINFO *ups, int command, void *data)	
{
    return 0;
}   
