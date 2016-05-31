/*
 * modbus.h
 *
 * Public header file for the modbus driver.
 */

/*
 * Copyright (C) 2013 Adam Kropelin
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

/*
 * Thanks go to APC/Schneider for providing the Apcupsd team with early access
 * to MODBUS protocol information to facilitate an Apcupsd driver.
 *
 * APC/Schneider has published the following relevant application notes:
 *
 * AN176: Modbus Implementation in APC Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=176>
 * AN177: Software interface for Switched Outlet and UPS Management in Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=177>
 * AN178: USB HID Implementation in Smart-UPS
 *    <http://www.apc.com/whitepaper/?an=178>
 */

#ifndef _MODBUS_H
#define _MODBUS_H

#include <stdint.h>
#include "mapping.h"

class astring;
class ModbusComm;

class ModbusUpsDriver: public UpsDriver
{
public:
   ModbusUpsDriver(UPSINFO *ups);
   virtual ~ModbusUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new ModbusUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool shutdown();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();
   virtual bool entry_point(int command, void *data);

   bool write_string_to_ups(const APCModbusMapping::RegInfo &reg, const char *str);
   bool write_int_to_ups(const APCModbusMapping::RegInfo &reg, uint64_t val);
   bool write_dbl_to_ups(const APCModbusMapping::RegInfo &reg, double val);
   bool read_string_from_ups(const APCModbusMapping::RegInfo &reg, astring *val);
   bool read_int_from_ups(const APCModbusMapping::RegInfo &reg, uint64_t *val);
   bool read_dbl_from_ups(const APCModbusMapping::RegInfo &reg, double *val);

private:

   struct CiInfo
   {
      int ci;
      bool dynamic;
      const APCModbusMapping::RegInfo *reg;
   };

   static const CiInfo CI_TABLE[];
   const CiInfo *GetCiInfo(int ci);
   bool UpdateCis(bool dynamic);
   bool UpdateCi(const CiInfo *info);
   bool UpdateCi(int ci);

   time_t _commlost_time;
   ModbusComm *_comm;
};

#endif   /* _MODBUS_H */
