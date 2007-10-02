/*
 * dumb.h
 *
 * Public header file for the simple-signalling (aka "dumb") driver
 */

/*
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

#ifndef _DUMB_H
#define _DUMB_H

#include "drivers.h"

class DumbDriver: public UpsDriver
{
public:

   DumbDriver(UPSINFO *ups) : UpsDriver(ups, "dumb") {}
   virtual ~DumbDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData() { return true; }
   virtual bool CheckState() { return ReadVolatileData(); }
   virtual bool Close();

   // Optional methods
   virtual bool KillPower();
   virtual bool Setup();
   virtual bool EntryPoint(int command, void *data);

private:

   struct termios _oldtio;
   struct termios _newtio;
   int _sp_flags;                  /* Serial port flags */
   time_t _debounce;               /* last event time for debounce */
};

#endif   /* _DUMB_H */
