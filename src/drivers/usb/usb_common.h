#ifndef _USB_COMMON_H
#define _USB_COMMON_H

#define MAX_VOLATILE_POLL_RATE 5      /* max rate to update volatile data */
#define LINK_RETRY_INTERVAL    5      /* retry every 5 seconds */

/* USB Vendor ID's */
#define VENDOR_APC 0x51D
#define VENDOR_MGE 0x463

/* Various known USB codes */
#define UPS_USAGE   0x840004
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
#define P_PWSUM   0x840024           /* power summary */
 
/* No Command Index, don't save this value */
#define CI_NONE -1

struct s_known_info {
    int ci;			      /* command index */
    unsigned usage_code;	      /* usage code */
    unsigned physical;		      /* physical usage */
    int data_type;		      /* data type expected */
};

typedef struct s_usb_value {
   int value_type;		      /* Type of returned value */
   double dValue;		      /* Value if double */
   char *sValue;		      /* Value if string */
   int iValue;			      /* integer value */
   char *UnitName;		      /* Name of units */
} USB_VALUE;

/* Check if the UPS has the given capability */
#define UPS_HAS_CAP(ci) (ups->UPS_Cap[ci])

/* Platform-specific code needs to call back to these operations */
int usb_ups_get_capabilities(UPSINFO *ups);
int usb_ups_read_static_data(UPSINFO *ups);

/* Useful helper functions for use by platform-specific code */
double pow_ten(int exponent);

#endif /* _USB_COMMON_H */
