/*
 * modbus.cpp
 *
 * Driver for APC MODBUS protocol
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

#include "apc.h"
#include "ModbusComm.h"

uint8_t *ModbusComm::ReadRegister(uint16_t reg, unsigned int nregs)
{
   ModbusPdu txpdu;
   ModbusPdu rxpdu;
   const unsigned int nbytes = nregs * sizeof(uint16_t);

   Dmsg(50, "%s: reg=%u, nregs=%u\n", __func__, reg, nregs);

   if (!_open)
   {
      Dmsg(0, "%s: Device not open\n", __func__);
      return NULL;
   }

   txpdu[0] = reg >> 8;
   txpdu[1] = reg;
   txpdu[2] = nregs >> 8;
   txpdu[3] = nregs;

   if (!SendAndWait(MODBUS_FC_READ_HOLDING_REGS, &txpdu, 4, 
                    &rxpdu, nbytes+1))
   {
      return NULL;
   }

   if (rxpdu[0] != nbytes)
   {
      // Invalid size
      Dmsg(0, "%s: Wrong number of data bytes received (exp=%u, rx=%u)\n", 
         __func__, nbytes, rxpdu[0]);
   }

   uint8_t *ret = new uint8_t[nbytes];
   memcpy(ret, rxpdu+1, nbytes);
   return ret;
}

bool ModbusComm::WriteRegister(uint16_t reg, unsigned int nregs, const uint8_t *data)
{
   ModbusPdu txpdu;
   ModbusPdu rxpdu;
   const unsigned int nbytes = nregs * sizeof(uint16_t);

   Dmsg(50, "%s: reg=%u, nregs=%u\n", __func__, reg, nregs);

   if (!_open)
   {
      Dmsg(0, "%s: Device not open\n", __func__);
      return false;
   }

   txpdu[0] = reg >> 8;
   txpdu[1] = reg;
   txpdu[2] = nregs >> 8;
   txpdu[3] = nregs;
   txpdu[4] = nbytes;
   memcpy(txpdu+5, data, nbytes);

   if (!SendAndWait(MODBUS_FC_WRITE_MULTIPLE_REGS, &txpdu, nbytes+5, &rxpdu, 4))
   {
      return false;
   }

   // Response should match first 4 bytes of command (reg and nregs)
   if (memcmp(rxpdu, txpdu, 4))
   {
      // Did not write expected number of registers
      Dmsg(0, "%s: Write failed reg=%u, nregs=%u, writereg=%u, writenregs=%u\n", 
         __func__, reg, nregs, (rxpdu[0] << 8) | rxpdu[1], 
         (rxpdu[2] << 8) | rxpdu[3]);
      return false;
   }

   return true;
}

bool ModbusComm::SendAndWait(
   uint8_t fc, 
   const ModbusPdu *txpdu, unsigned int txsz, 
   ModbusPdu *rxpdu, unsigned int rxsz)
{
   ModbusFrame txfrm;
   ModbusFrame rxfrm;
   unsigned int sz;

   // Ensure caller isn't trying to send an oversized PDU
   if (txsz > MODBUS_MAX_PDU_SZ || rxsz > MODBUS_MAX_PDU_SZ)
      return false;

   // Prepend slave address and function code
   txfrm[0] = _slaveaddr;
   txfrm[1] = fc;

   // Add PDU
   memcpy(txfrm+2, txpdu, txsz);

   // Calculate crc
   uint16_t crc = ModbusCrc(txfrm, txsz+2);

   // CRC goes out LSB first, unlike other MODBUS fields
   txfrm[txsz+2] = crc;
   txfrm[txsz+3] = crc >> 8;

   int retries = 2;
   do
   {
      if (!ModbusTx(&txfrm, txsz+4))
      {
         // Failure to send is immediately fatal
         return false;
      }

      if (!ModbusRx(&rxfrm, &sz))
      {
         // Rx timeout: Retry
         continue;
      }

      if (sz < 4)
      {
         // Runt frame: Retry
         Dmsg(0, "%s: runt frame (%u)\n", __func__, sz);
         continue;
      }

      crc = ModbusCrc(rxfrm, sz-2);
      if (rxfrm[sz-2] != (crc & 0xff) ||
          rxfrm[sz-1] != (crc >> 8))
      {
         // CRC error: Retry
         Dmsg(0, "%s: CRC error\n", __func__);
         continue;
      }

      if (rxfrm[0] != _slaveaddr)
      {
         // Not from expected slave: Retry
         Dmsg(0, "%s: Bad address (exp=%u, rx=%u)\n", 
            __func__, _slaveaddr, rxfrm[0]);
         continue;
      }

      if (rxfrm[1] == (fc | MODBUS_FC_ERROR))
      {
         // Exception response: Immediately fatal
         Dmsg(0, "%s: Exception (code=%u)\n", __func__, rxfrm[2]);
         return false;
      }

      if (rxfrm[1] != fc)
      {
         // Unknown response: Retry
         Dmsg(0, "%s: Unexpected response 0x%02x\n", __func__, rxfrm[1]);
         continue;
      }

      if (sz != rxsz+4)
      {
         // Wrong size: Retry
         Dmsg(0, "%s: Wrong size (exp=%u, rx=%u)\n", __func__, rxsz+4, sz);
         continue;
      }

      // Everything is ok
      memcpy(rxpdu, rxfrm+2, rxsz);
      return true;
   }
   while (retries--);

   // Retries exhausted
   Dmsg(0, "%s: Retries exhausted\n", __func__);
   return false;
}

uint16_t ModbusComm::ModbusCrc(const uint8_t *data, unsigned int sz)
{
   // 1 + x^2 + x^15 + x^16
   static const uint16_t MODBUS_CRC_POLY = 0xA001; 
   uint16_t crc = 0xffff;

   while (sz--)
   {
      crc ^= *data++;
      for (unsigned int i = 0; i < 8; ++i)
      {
         if (crc & 0x1)
            crc = (crc >> 1) ^ MODBUS_CRC_POLY;
         else
            crc >>= 1;
      }
   }

   return crc;
}
