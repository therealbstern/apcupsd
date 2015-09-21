/*
 * ModbusUsbComm.h
 *
 * Public header file for the modbus driver.
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

#ifndef _MODBUSUSBCOMM_H
#define _MODBUSUSBCOMM_H

#include "ModbusComm.h"
#include "HidUps.h"

class ModbusUsbComm: public ModbusComm
{
public:
   ModbusUsbComm(uint8_t slaveaddr = DEFAULT_SLAVE_ADDR);
   virtual ~ModbusUsbComm() {}

   virtual bool Open(const char *dev);
   virtual bool Close();

private:

   virtual bool ModbusTx(const ModbusFrame *frm, unsigned int sz);
   virtual bool ModbusRx(ModbusFrame *frm, unsigned int *sz);
   bool WaitIdle();
   static uint64_t GetTod();

   static const unsigned MODBUS_USB_REPORT_SIZE = 64;
   static const unsigned MODBUS_USB_REPORT_MAX_FRAME_SIZE = 
      MODBUS_USB_REPORT_SIZE - 1;

   HidUps _hidups;
   uint8_t _rxrpt;
   uint8_t _txrpt;
};

#endif   /* _MODBUSUSBCOMM_H */
