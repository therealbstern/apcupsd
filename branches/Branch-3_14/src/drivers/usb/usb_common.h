/*
 * usb_common.h
 *
 * Public USB driver interface exposed to platform-specific USB sub-drivers.
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

#ifndef _USB_COMMON_H
#define _USB_COMMON_H

/* Max rate to update volatile data */
#define MAX_VOLATILE_POLL_RATE 5

/* How often to retry the link (seconds) */
#define LINK_RETRY_INTERVAL    5 

/* USB Vendor ID's */ 
#define VENDOR_APC 0x051D

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

#endif  /* _USB_COMMON_H */
