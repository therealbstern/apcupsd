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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#ifndef _APCSMART_H
#define _APCSMART_H

class ApcSmartUpsDriver: public UpsDriver
{
public:
   ApcSmartUpsDriver(UPSINFO *ups);
   virtual ~ApcSmartUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new ApcSmartUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();
   virtual bool setup();
   virtual bool entry_point(int command, void *data);
   virtual bool program_eeprom(int command, const char *data);
   virtual bool shutdown();

   // Public for apctest
   char *smart_poll(char cmd);
   int getline(char *s, int len);
   void writechar(char a);

private:

   static SelfTestResult decode_testresult(char* str);
   static LastXferCause decode_lastxfer(char *str);
   static const char *get_model_from_oldfwrev(const char *s);

   void UPSlinkCheck();

   int apcsmart_ups_shutdown_with_delay(int shutdown_delay);
   int apcsmart_ups_get_shutdown_delay();
   void apcsmart_ups_warn_shutdown(int shutdown_delay);

   void change_ups_battery_date(const char *newdate);
   void change_ups_name(const char *newname);
   void change_extended();
   int change_ups_eeprom_item(const char *title, const char cmd, const char *setting);

   struct termios _oldtio;
   struct termios _newtio;
   bool _linkcheck;
};

#endif   /* _APCSMART_H */
