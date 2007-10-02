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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _TESTDRIVER_H
#define _TESTDRIVER_H

#include "drivers.h"

class TestDriver: public UpsDriver
{
public:

   TestDriver(UPSINFO *ups) : UpsDriver(ups, "test") {}
   virtual ~TestDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState();
   virtual bool Close();

private:

   bool open_test_device();
};

#endif   /* _TEST_DRIVER_H */
