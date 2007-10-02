/*
 * apcsmart.h
 *
 * Public header file for the APC Smart protocol driver
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

#ifndef _APCSMART_H
#define _APCSMART_H

#include "drivers.h"

class ApcSmartDriver: public UpsDriver
{
public:

   ApcSmartDriver(UPSINFO *ups) : UpsDriver(ups, "apcsmart") {}
   virtual ~ApcSmartDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState();
   virtual bool Close();

   // We provide default do-nothing implementations
   // for these methods since not all drivers need them.
   virtual bool KillPower();
   virtual bool Setup();
   virtual bool ProgramEeprom(int command, char *data);
   virtual bool EntryPoint(int command, void *data);

private:

   static SelfTestResult decode_testresult(char* str);
   static LastXferCause decode_lastxfer(char *str);

   int apc_enable();
   char *smart_poll(char cmd);
   int getline(char *s, int len);
   void UPSlinkCheck();

   void change_ups_battery_date(char *newdate);
   void change_ups_name(char *newname);
   void change_extended();
   int change_ups_eeprom_item(char *title, char cmd, char *setting);

   char *get_apc_model_V_codes(const char *s);
   char *get_apc_model_b_codes(const char *s);   
   void get_apc_model();

   struct termios _oldtio;
   struct termios _newtio;
};

#endif   /* _APCSMART_H */
