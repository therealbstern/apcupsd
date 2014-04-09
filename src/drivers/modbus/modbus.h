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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
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

   // MODBUS constants
   static const uint8_t DEFAULT_SLAVE_ADDR = 1;

   // MODBUS timeouts
   static const unsigned int MODBUS_INTERCHAR_TIMEOUT_MS = 25; // Spec is 15, increase for compatibility with USB serial dongles
   static const unsigned int MODBUS_INTERFRAME_TIMEOUT_MS = 45; // Spec is 35, increase due to UPS missing messages occasionally
   static const unsigned int MODBUS_IDLE_WAIT_TIMEOUT_MS = 100;
   static const unsigned int MODBUS_RESPONSE_TIMEOUT_MS = 500;

   // MODBUS function codes
   static const uint8_t MODBUS_FC_ERROR = 0x80;
   static const uint8_t MODBUS_FC_READ_HOLDING_REGS = 0x03;
   static const uint8_t MODBUS_FC_WRITE_REG = 0x06;
   static const uint8_t MODBUS_FC_WRITE_MULTIPLE_REGS = 0x10;

   // MODBUS message sizes
   static const unsigned int MODBUS_MAX_FRAME_SZ = 256;
   static const unsigned int MODBUS_MAX_PDU_SZ = MODBUS_MAX_FRAME_SZ - 4;

   typedef uint8_t ModbusFrame[MODBUS_MAX_FRAME_SZ];
   typedef uint8_t ModbusPdu[MODBUS_MAX_PDU_SZ];

   uint8_t *ReadRegister(uint16_t addr, unsigned int nregs);
   bool WriteRegister(uint16_t reg, unsigned int nregs, const uint8_t *data);

   bool SendAndWait(
      uint8_t fc, 
      const ModbusPdu *txpdu, unsigned int txsz, 
      ModbusPdu *rxpdu, unsigned int rxsz);

   bool ModbusTx(const ModbusFrame *frm, unsigned int sz);
   bool ModbusRx(ModbusFrame *frm, unsigned int *sz);

   bool ModbusWaitIdle();

   uint16_t ModbusCrc(const uint8_t *data, unsigned int sz);

   uint8_t _slaveaddr;
   struct termios _oldtio;
   struct termios _newtio;
   time_t _commlost_time;
};

#endif   /* _MODBUS_H */
