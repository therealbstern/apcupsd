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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _DRIVERS_H
#define _DRIVERS_H

// Forward declaration
class UPSINFO;

// Base class for UPS drivers. Driver implementations inherit
// from this class and override appropriate methods as needed.
class UpsDriver
{
public:
   UpsDriver(UPSINFO *ups, const char *name)
      : driver_name(name), _ups(ups) {}
   virtual ~UpsDriver() {}

   // Subclasses must implement these methods
   virtual bool Open() = 0;
   virtual bool GetCapabilities() = 0;
   virtual bool ReadVolatileData() = 0;
   virtual bool ReadStaticData() = 0;
   virtual bool CheckState() = 0;
   virtual bool Close() = 0;

   // We provide default do-nothing implementations
   // for these methods since not all drivers need them.
   virtual bool KillPower()                            { return false; }
   virtual bool Setup()                                { return true;  }
   virtual bool ProgramEeprom(int command, char *data) { return false; }
   virtual bool EntryPoint(int command, void *data)    { return false; }

   const char *driver_name;

protected:
   UPSINFO *_ups;
};

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

/* Some defines that helps code readability. */
#define device_open(ups) \
   do { \
      if (ups->driver) ups->driver->Open(); \
   } while(0)
#define device_setup(ups) \
   do { \
      if (ups->driver) ups->driver->Setup(); \
   } while(0)
#define device_close(ups) \
   do { \
      if (ups->driver) ups->driver->Close(); \
   } while(0)
#define device_kill_power(ups) \
   do { \
      if (ups->driver) ups->driver->KillPower(); \
   } while(0)
#define device_read_static_data(ups) \
   do { \
      if (ups->driver) ups->driver->ReadStaticData(); \
   } while(0)
#define device_read_volatile_data(ups) \
   do { \
      if (ups->driver) ups->driver->ReadVolatileData(); \
   } while(0)
#define device_get_capabilities(ups) \
   do { \
      if (ups->driver) ups->driver->GetCapabilities(); \
   } while(0)
#define device_check_state(ups) \
   do { \
      if (ups->driver) ups->driver->CheckState(); \
   } while(0)
#define device_program_eeprom(ups, command, data) \
   do { \
      if (ups->driver) ups->driver->ProgramEeprom(command, data); \
   } while(0)
#define device_entry_point(ups, command, data) \
   do { \
      if (ups->driver) ups->driver->EntryPoint(command, data); \
   } while(0)

/* For device_entry_point commands. */
enum {
   /* Dumb entry points. */
   DEVICE_CMD_DTR_ENABLE,
   DEVICE_CMD_DTR_ST_DISABLE,

   /* Smart entry points. */
   DEVICE_CMD_GET_SELFTEST_MSG,
   DEVICE_CMD_CHECK_SELFTEST,
   DEVICE_CMD_SET_DUMB_MODE
};

/* Support routines. */
void attach_driver(UPSINFO *ups);

#endif /*_DRIVERS_H */
