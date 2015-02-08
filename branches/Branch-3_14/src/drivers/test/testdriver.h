/*
 * testdriver.h
 *
 * Public header file for the test driver.
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

#ifndef _TESTDRIVER_H
#define _TESTDRIVER_H

class TestUpsDriver: public UpsDriver
{
public:
   TestUpsDriver(UPSINFO *ups);
   virtual ~TestUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new TestUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();

private:

   bool open_test_device();
};

#endif   /* _TEST_DRIVER_H */
