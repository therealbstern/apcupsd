/*
 * drivers.h
 *
 * Header file for exporting UPS drivers.
 */

/*
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 * Copyright (C) 1996-1999 Andre M. Hedrick <andre@suse.com>
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

#ifndef _DRIVERS_H
#define _DRIVERS_H

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
 *   Put the UPS into hibernation mode, killing output power.
 *   This function always returns.
 *
 * shutdown()
 *   Turn off the UPS completely.
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
 * program_eeprom(ups, command, data)
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

class UpsDriver
{
public:
   UpsDriver(UPSINFO *ups) : _ups(ups) {}
   virtual ~UpsDriver() {}

   virtual bool Open() = 0;
   virtual bool Close() = 0;
   virtual bool read_static_data() = 0;
   virtual bool read_volatile_data() = 0;
   virtual bool get_capabilities() = 0;
   virtual bool check_state() = 0;

   virtual bool setup()          { return true;  }
   virtual bool kill_power()     { return false; }
   virtual bool shutdown()       { return false; }

   virtual bool program_eeprom(int cmd, const char *data) { return false; }
   virtual bool entry_point(int cmd, void *data)          { return false; }

protected:
   UPSINFO *_ups;
};

typedef struct upsdriver {
   const char *driver_name;
   UpsDriver *(* factory) (UPSINFO *ups);
} UPSDRIVER;


/* Some defines that helps code readability. */
#define device_open(ups) ups->driver->Open()
#define device_setup(ups) ups->driver->setup()
#define device_close(ups) ups->driver->Close()
#define device_kill_power(ups) ups->driver->kill_power()
#define device_shutdown(ups) ups->driver->shutdown()
#define device_read_static_data(ups) ups->driver->read_static_data()
#define device_read_volatile_data(ups) ups->driver->read_volatile_data()
#define device_get_capabilities(ups) ups->driver->get_capabilities()
#define device_check_state(ups) ups->driver->check_state()
#define device_program_eeprom(ups, command, data) ups->driver->program_eeprom(command, data)
#define device_entry_point(ups, command, data) ups->driver->entry_point(command, data)

/* Now some defines for device_entry_point commands. */

/* Dumb entry points. */
#define DEVICE_CMD_DTR_ENABLE       0x00
#define DEVICE_CMD_DTR_ST_DISABLE   0x01

/* Smart entry points. */
#define DEVICE_CMD_GET_SELFTEST_MSG 0x02
#define DEVICE_CMD_CHECK_SELFTEST   0x03
#define DEVICE_CMD_SET_DUMB_MODE    0x04

/* Support routines. */
UpsDriver *attach_driver(UPSINFO *ups);


#endif /*_DRIVERS_H */
