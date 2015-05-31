/*
 * testdriver.c
 *
 * Interface for apcupsd to the Test driver.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
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
#include "testdriver.h"

TestUpsDriver::TestUpsDriver(UPSINFO *ups) :
   UpsDriver(ups)
{
}

/*
 */
bool TestUpsDriver::open_test_device()
{
   _ups->fd = 1;
   return true;
}


/*
 * Read UPS events. I.e. state changes.
 */
bool TestUpsDriver::check_state()
{
   return true;
}

/*
 * Open test port
 */
bool TestUpsDriver::Open()
{
   write_lock(_ups);

   if (!open_test_device())
      Error_abort("Cannot open UPS device %s\n", _ups->device);

   _ups->clear_slave();
   write_unlock(_ups);
   return true;
}

bool TestUpsDriver::Close()
{
   write_lock(_ups);
   
   /* Seems that there is nothing to do. */
   _ups->fd = -1;

   write_unlock(_ups);
   return true;
}

/*
 * Setup capabilities structure for UPS
 */
bool TestUpsDriver::get_capabilities()
{
   int k;

   write_lock(_ups);
   for (k = 0; k <= CI_MAX_CAPS; k++)
      _ups->UPS_Cap[k] = TRUE;

   write_unlock(_ups);
   return true;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
bool TestUpsDriver::read_static_data()
{
   write_lock(_ups);
   /* UPS_NAME */

   /* model, firmware */
   strlcpy(_ups->upsmodel, "Test Driver", sizeof(_ups->upsmodel));
   strlcpy(_ups->firmrev, "Rev 1.0", sizeof(_ups->firmrev));
   strlcpy(_ups->selftest, "336", sizeof(_ups->selftest));

   /* WAKEUP_DELAY */
   _ups->dwake = 2 * 60;

   /* SLEEP_DELAY */
   _ups->dshutd = 2 * 60;

   /* LOW_TRANSFER_LEVEL */
   _ups->lotrans = 190;

   /* HIGH_TRANSFER_LEVEL */
   _ups->hitrans = 240;

   /* UPS_BATT_CAP_RETURN */
   _ups->rtnpct = 15;

   /* LOWBATT_SHUTDOWN_LEVEL */
   _ups->dlowbatt = 2;

   /* UPS_MANUFACTURE_DATE */
   strlcpy(_ups->birth, "2001-09-21", sizeof(_ups->birth));

   /* Last UPS_BATTERY_REPLACE */
   strlcpy(_ups->battdat, "2001-09-21", sizeof(_ups->battdat));

   /* UPS_SERIAL_NUMBER */
   strlcpy(_ups->serial, "NO-123456", sizeof(_ups->serial));

   /* Nominal output voltage when on batteries */
   _ups->NomOutputVoltage = 230;

   /* Nominal battery voltage */
   _ups->nombattv = (double)12;

   write_unlock(_ups);
   return true;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every 5 seconds to get
 * a current idea of what the UPS is doing.
 */
bool TestUpsDriver::read_volatile_data()
{
   /* save time stamp */
   time(&_ups->poll_time);

   write_lock(_ups);

   /* UPS_STATUS -- this is the most important status for apcupsd */

   /* No APC Status value, well, fabricate one */
   _ups->Status = 0;

   _ups->set_online();

   /* LINE_VOLTAGE */
   _ups->LineVoltage = 229.5;
   _ups->LineMin = 225.0;
   _ups->LineMax = 230.0;

   /* OUTPUT_VOLTAGE */
   _ups->OutputVoltage = 228.5;

   /* BATT_FULL Battery level percentage */
   _ups->BattChg = 100;

   /* BATT_VOLTAGE */
   _ups->BattVoltage = 12.5;

   /* UPS_LOAD */
   _ups->UPSLoad = 40.5;

   /* LINE_FREQ */
   _ups->LineFreq = 50;

   /* UPS_RUNTIME_LEFT */
   _ups->TimeLeft = ((double)20 * 60);   /* seconds */

   /* UPS_TEMP */
   _ups->UPSTemp = 32.4;

   /*  Humidity percentage */
   _ups->humidity = 50.1;

   /*  Ambient temperature */
   _ups->ambtemp = 22.5;

   /* Self test results */
   _ups->testresult = TEST_PASSED;

   write_unlock(_ups);

   return true;
}
