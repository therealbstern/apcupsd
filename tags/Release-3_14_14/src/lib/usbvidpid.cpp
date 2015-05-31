/*
 * Copyright (C) 2015 Adam Kropelin
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

#include "usb_common.h"

static const uint16_t VENDOR_APC       = 0x051D;
static const uint16_t VENDOR_SCHNEIDER = 0x16DE;
static const uint16_t VENDOR_HP        = 0x03F0;

//
// Table of supported USB idVendor/idProduct tuples
//
// When updating this table, also update platforms including:
//    * platforms/mingw/winusb/apcupsd.inf
//    * platforms/darwin/Info.plist
//    * platforms/sun/Makefile
//
static const struct
{
   uint16_t vid;
   uint16_t pid;
} SUPPORTED_DEVICES[] =
{
   {VENDOR_APC,         0x0002}, // Older APC UPSes
   {VENDOR_APC,         0x0003}, // AP9620 LCC and newer UPSes
   {VENDOR_APC,         0x0004},

   // As of Jan 2015, the following are for future APC/Schneider products
   {VENDOR_APC,         0xC801},
   {VENDOR_APC,         0xC802},
   {VENDOR_APC,         0xC803},
   {VENDOR_APC,         0xC804},
   {VENDOR_APC,         0xC805},
   {VENDOR_SCHNEIDER,   0xC801},
   {VENDOR_SCHNEIDER,   0xC802},
   {VENDOR_SCHNEIDER,   0xC803},
   {VENDOR_SCHNEIDER,   0xC804},
   {VENDOR_SCHNEIDER,   0xC805},

   // Some oddballs
   {VENDOR_APC,         0x0001}, // Early prototype units of AP9620 
   {VENDOR_APC,         0xffff}, // Early prototype units of AP9620 
   {VENDOR_APC,         0x0005}, // An SMX750 appears to use PID 5

   // The HP T1500 G3 has been shown to work well with apcupsd.
   // Not sure if it is an APC clone or just "very compatible".
   {VENDOR_HP,          0x1FE3},

   {0}
};

bool MatchVidPid(uint16_t vid, uint16_t pid)
{
   for (unsigned int i = 0; SUPPORTED_DEVICES[i].vid; ++i)
   {
      if (SUPPORTED_DEVICES[i].vid == vid && 
          SUPPORTED_DEVICES[i].pid == pid)
         return true;
   }
   return false;
}
