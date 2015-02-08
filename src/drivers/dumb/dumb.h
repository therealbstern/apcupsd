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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#ifndef _DUMB_H
#define _DUMB_H

class DumbUpsDriver: public UpsDriver
{
public:
   DumbUpsDriver(UPSINFO *ups);
   virtual ~DumbUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new DumbUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool check_state() { return read_volatile_data(); }
   virtual bool Open();
   virtual bool Close();
   virtual bool setup();
   virtual bool entry_point(int command, void *data);

private:
   int _sp_flags;                   /* Serial port flags */
   struct termios _oldtio;
   struct termios _newtio;
};

#endif   /* _DUMB_H */
