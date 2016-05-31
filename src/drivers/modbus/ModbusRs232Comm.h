/*
 * ModbusRs232Comm.h
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

#ifndef _MODBUSRS232COMM_H
#define _MODBUSRS232COMM_H

#include "ModbusComm.h"

class ModbusRs232Comm: public ModbusComm
{
public:
   ModbusRs232Comm(uint8_t slaveaddr = DEFAULT_SLAVE_ADDR);
   virtual ~ModbusRs232Comm() {}

   virtual bool Open(const char *dev);
   virtual bool Close();

private:

   virtual bool ModbusTx(const ModbusFrame *frm, unsigned int sz);
   virtual bool ModbusRx(ModbusFrame *frm, unsigned int *sz);

   bool ModbusWaitIdle();

   int _fd;
};

#endif   /* _MODBUSRS232COMM_H */
