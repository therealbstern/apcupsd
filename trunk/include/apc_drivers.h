/*
 *  apc_drivers.h -- header file for exporting UPS drivers.
 *
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 */

/*
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *                           675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS APCUPSD-Team BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE APCUPSD-Team HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE APCUPSD-Team SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN AS IS BASIS, AND THE APCUPSD-Team HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE APCUPSD-Team HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, APCC.  THE APCUPSD-Team DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH APCC.  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */

#ifndef _APC_DRIVERS_H
#define _APC_DRIVERS_H

/*
 * This is the generic drivers structure. It contain any routine needed for
 * managing a device (or family of devices, like Smart UPSes).
 *
 * Routines defined:
 *
 * open()
 *   Opens the device and setup the file descriptor. Returns a working file
 *   descriptor. This function does not interact with hardware functionality.
 *   In case of error, this function does not return. It simply exit.
 *
 * setup()
 *   Setup the device for operations. This function interacts with hardware to
 *   make sure on the other end there is an UPS and that the link is working.
 *   In case of error, this function does not return. It simply exit.
 *
 * close()
 *   Closes the device returning it to the original status.
 *   This function always returns.
 *
 * kill_power()
 *   Kills the UPS power.
 *   This function always returns.
 *
 * read_ups_static_data()
 *   Gets the static data from UPS like the UPS name.
 *   This function always returns.
 *
 * read_ups_volatile_data()
 *   Fills UPSINFO with dynamic UPS data.
 *   This function always returns.
 *   This function must lock the UPSINFO structure.
 *
 * get_ups_capabilities()
 *   Try to understand what capabilities the UPS is able to perform.
 *   This function always returns.
 *
 * check_ups_state()
 *   Check if the UPS changed state.
 *   This function always returns.
 *   This function must lock the UPSINFO structure.
 *
 * ups_program_eeprom(ups, command, data)
 *   Commit changes to the internal UPS eeprom.
 *   This function performs the eeprom change command (using data),
 *     then returns.
 *
 * ups_generic_entry_point()
 *  This is a generic entry point into the drivers for specific driver
 *  functions called from inside the apcupsd core.
 *  This function always return.
 *  This function must lock the UPSINFO structure.
 *  This function gets a void * that contain data. This pointer can be used
 *  to pass data to the function or to get return values from the function,
 *  depending on the value of "command" and the general design of the specific
 *  command to be executed.
 *  Each driver will have its specific functions and will ignore any
 *  function that does not understand.
 */

typedef struct upsdriver {
    /*
     * Data side of the driver structure.
     */
    char *driver_name;

    /*
     * Functions side of the driver structure.
     */
    int (*open)(UPSINFO *ups);
    int (*setup)(UPSINFO *ups);
    int (*close)(UPSINFO *ups);
    int (*kill_power)(UPSINFO *ups);
    int (*read_ups_static_data)(UPSINFO *ups);
    int (*read_ups_volatile_data)(UPSINFO *ups);
    int (*get_ups_capabilities)(UPSINFO *ups);
    int (*check_ups_state)(UPSINFO *ups);
    int (*ups_program_eeprom)(UPSINFO *ups, int command, char *data);
    int (*ups_entry_point)(UPSINFO *ups, int command, void *data);
} UPSDRIVER;

/*
 * Some defines that helps code readability.
 */
#define device_open(ups) \
    if (ups->driver) ups->driver->open(ups)
#define device_setup(ups) \
    if (ups->driver) ups->driver->setup(ups)
#define device_close(ups) \
    if (ups->driver) ups->driver->close(ups)
#define device_kill_power(ups) \
    if (ups->driver) ups->driver->kill_power(ups)
#define device_read_static_data(ups) \
    if (ups->driver) ups->driver->read_ups_static_data(ups)
#define device_read_volatile_data(ups) \
    if (ups->driver) ups->driver->read_ups_volatile_data(ups)
#define device_get_capabilities(ups) \
    if (ups->driver) ups->driver->get_ups_capabilities(ups)
#define device_check_state(ups) \
    if (ups->driver) ups->driver->check_ups_state(ups)
#define device_program_eeprom(ups, command, data) \
    if (ups->driver) ups->driver->ups_program_eeprom(ups, command, data)
#define device_entry_point(ups, command, data) \
    if (ups->driver) ups->driver->ups_entry_point(ups, command, data)

/*
 * Now some defines for device_entry_point commands.
 */
/*
 * Dumb entry points.
 */
#define DEVICE_CMD_DTR_ENABLE       0x00
#define DEVICE_CMD_DTR_ST_DISABLE   0x01
/*
 * Smart entry points.
 */
#define DEVICE_CMD_GET_SELFTEST_MSG 0x02
#define DEVICE_CMD_CHECK_SELFTEST   0x03
#define DEVICE_CMD_SET_DUMB_MODE    0x04

/*
 * Support routines.
 */
UPSDRIVER *attach_driver(UPSINFO *ups);

#endif /*_APC_DRIVERS_H */
