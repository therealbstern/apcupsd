/*
 * ModbusComm.h
 *
 * Public header file for the modbus communications base class.
 */

/*
 * Copyright (C) 2014 Adam Kropelin
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

#ifndef _MODBUSCOMM_H
#define _MODBUSCOMM_H

#include <stdint.h>

class ModbusComm
{
public:
   ModbusComm(uint8_t slaveaddr = DEFAULT_SLAVE_ADDR) : 
      _slaveaddr(slaveaddr), _open(false) {}
   virtual ~ModbusComm() {}

   virtual bool Open(const char *dev) = 0;
   virtual bool Close() = 0;

   virtual uint8_t *ReadRegister(uint16_t addr, unsigned int nregs);
   virtual bool WriteRegister(uint16_t reg, unsigned int nregs, const uint8_t *data);

protected:

   uint16_t ModbusCrc(const uint8_t *data, unsigned int sz);

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

   virtual bool ModbusTx(const ModbusFrame *frm, unsigned int sz) = 0;
   virtual bool ModbusRx(ModbusFrame *frm, unsigned int *sz) = 0;

   uint8_t _slaveaddr;
   bool _open;

private:

   virtual bool SendAndWait(
      uint8_t fc, 
      const ModbusPdu *txpdu, unsigned int txsz, 
      ModbusPdu *rxpdu, unsigned int rxsz);
};

#endif   /* _MODBUSCOMM_H */
