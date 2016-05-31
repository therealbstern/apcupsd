/*
 * ModbusUsbComm.cpp
 *
 * USB communication layer for MODBUS driver
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
#include "ModbusUsbComm.h"

#define ModbusRTURx 0xFF8600FC
#define ModbusRTUTx 0xFF8600FD

ModbusUsbComm::ModbusUsbComm(uint8_t slaveaddr) :
   ModbusComm(slaveaddr)
{
}

bool ModbusUsbComm::Open(const char *path)
{
   // In case we're already open
   Close();

   // Attempt to locate and open the UPS on USB
   if (!_hidups.Open(path))
      return false;

   // Find ModbusRTUTx report
   hid_item_t item;
   if (!_hidups.LocateItem(ModbusRTUTx, -1, -1, -1, HID_KIND_INPUT, &item))
   {
      Dmsg(0, "%s: Unable to find ModbusRTUTx usage\n", __func__);
      goto error;
   }
   _rxrpt = item.report_ID;
   Dmsg(100, "%s: Found ModbusRTUTx in report id %d\n", __func__, _rxrpt);

   // Find ModbusRTURx report
   if (!_hidups.LocateItem(ModbusRTURx, -1, -1, -1, HID_KIND_OUTPUT, &item))
   {
      Dmsg(0, "%s: Unable to find ModbusRTURx usage\n", __func__);
      goto error;
   }
   _txrpt = item.report_ID;
   Dmsg(100, "%s: Found ModbusRTURx in report id %d\n", __func__, _txrpt);

   _open = true;
   return true;

error:
   _hidups.Close();
   return false;
}

bool ModbusUsbComm::Close()
{
   _open = false;
   _hidups.Close();
   return true;
}

bool ModbusUsbComm::ModbusTx(const ModbusFrame *frm, unsigned int sz)
{
   // MODBUS/USB is limited to 63 bytes of payload (we don't bother with
   // fragmentation/reassembly). Since we drop the CRC (last 2 bytes) because 
   // MODBUS/USB doesn't use it, frame size is 2 less than what caller says.
   if (sz-2 > MODBUS_USB_REPORT_MAX_FRAME_SIZE)
      return false;

   // Wait for idle 
   if (!WaitIdle())
      return false;

   Dmsg(100, "%s: Sending frame\n", __func__);
   hex_dump(100, frm, sz);

   // We add HID report id as the first byte of the report, then at most 63
   // bytes of payload.
   uint8_t rpt[MODBUS_USB_REPORT_SIZE] = {0};
   rpt[0] = _txrpt;
   memcpy(rpt+1, frm, sz-2);
   return _hidups.InterruptWrite(USB_ENDPOINT_OUT|1, rpt, 
      MODBUS_USB_REPORT_SIZE, MODBUS_RESPONSE_TIMEOUT_MS) == 
         (int)MODBUS_USB_REPORT_SIZE;
}

bool ModbusUsbComm::ModbusRx(ModbusFrame *frm, unsigned int *sz)
{
   struct timeval now, exittime;

   // Determine time at which we need to exit
   gettimeofday(&exittime, NULL);
   exittime.tv_sec += MODBUS_RESPONSE_TIMEOUT_MS / 1000;
   exittime.tv_usec += (MODBUS_RESPONSE_TIMEOUT_MS % 1000) * 1000;
   if (exittime.tv_usec >= 1000000)
   {
      ++exittime.tv_sec;
      exittime.tv_usec -= 1000000;
   }

   int ret;
   uint8_t rpt[MODBUS_USB_REPORT_SIZE];
   while (1)
   {
      gettimeofday(&now, NULL);
      int timeout = TV_DIFF_MS(now, exittime);
      if (timeout <= 0 || 
          (ret = _hidups.InterruptRead(USB_ENDPOINT_IN|1, rpt, 
             MODBUS_USB_REPORT_SIZE, timeout)) == -ETIMEDOUT)
      {
         Dmsg(0, "%s: TIMEOUT\n", __func__);
         return false;
      }

      // Temporary failure
      if (ret == -EINTR || ret == -EAGAIN)
         continue;

      // Fatal error
      if (ret <= 0)
      {
         Dmsg(0, "%s: Read error: %d\n", __func__, ret);
         return false;
      }

      // Filter out non-MODBUS reports
      if (rpt[0] != _rxrpt)
      {
         Dmsg(100, "%s: Ignoring report id %u\n", __func__, rpt[0]);
         continue;
      }

      // Bad report size ... fatal
      if (ret != (int)MODBUS_USB_REPORT_SIZE)
      {
         Dmsg(0, "%s: Bad size %d\n", __func__, ret);
         return false;
      }

      // We always get a full report containing MODBUS_USB_REPORT_MAX_FRAME_SIZE
      // bytes of data. Clip to actual size of live data by looking at the MODBUS 
      // PDU header. This is a blatant layering violation, but no way around it 
      // here. Which byte(s) we look at and how we calculate the length depends
      // on the opcode.
      unsigned frmsz;
      if (rpt[2] == MODBUS_FC_READ_HOLDING_REGS)
      {
         // READ_HOLDING_REGS response includes a size byte.
         // Add 3 bytes to PDU size to account for size byte itself 
         // plus frame header (slaveaddr and op code).
         frmsz = rpt[3] + 3;
      }
      else if (rpt[2] == MODBUS_FC_WRITE_MULTIPLE_REGS)
      {
         // WRITE_MULTIPLE_REGS response is always a fixed length
         // 2 byte frame header (slaveaddr and op code)
         // 2 byte register starting address
         // 2 byte register count
         frmsz = 6;
      }
      else
      {
         // Unsupported response message...we can't calculate its length
         Dmsg(0, "%s: Unknown response type %x\n", __func__, rpt[2]);
         hex_dump(0, rpt,  MODBUS_USB_REPORT_SIZE);
         return false;
      }

      if (frmsz > MODBUS_USB_REPORT_MAX_FRAME_SIZE)
      {
         Dmsg(0, "%s: Fragmented PDU received...not supported\n", __func__);
         return false;
      }

      // Copy data to caller's buffer. Live data starts after USB report id byte.
      memcpy(frm, rpt+1, frmsz);

      // MODBUS/USB doesn't provide a CRC. 
      // Fill one in to make upper layer happy.
      uint16_t crc = ModbusCrc(*frm, frmsz);
      (*frm)[frmsz] = crc & 0xff;
      (*frm)[frmsz+1] = crc >> 8;

      *sz = frmsz + 2;
      hex_dump(100, frm, *sz);
      return true;
   }
}

#define S_TO_NS(x)  ( (x) * 1000000000ULL )
#define MS_TO_NS(x) ( (x) * 1000000ULL )
#define US_TO_NS(x) ( (x) * 1000ULL )
#define NS_TO_MS(x) ( ((x)+999999) / 1000000ULL )

uint64_t ModbusUsbComm::GetTod()
{
   struct timeval now;
   gettimeofday(&now, NULL);
   return S_TO_NS(now.tv_sec) + US_TO_NS(now.tv_usec);
}

bool ModbusUsbComm::WaitIdle()
{
   // Current TOD
   uint64_t now = GetTod();

   // When we will give up by
   uint64_t exittime = now + MS_TO_NS(MODBUS_IDLE_WAIT_TIMEOUT_MS);

   // Initial idle target
   uint64_t target = now + MS_TO_NS(MODBUS_INTERFRAME_TIMEOUT_MS);

   uint8_t rpt[MODBUS_USB_REPORT_SIZE];
   while (target <= exittime)
   {
      int rc = _hidups.InterruptRead(USB_ENDPOINT_IN|1, rpt, 
         MODBUS_USB_REPORT_SIZE, NS_TO_MS(target-now));

      if (rc == -ETIMEDOUT)
      {
         // timeout: line is now idle
         return true;
      }
      else if (rc <= 0 && rc != -EINTR && rc != -EAGAIN)
      {
         // fatal error
         Dmsg(0, "%s: interrupt_read failed: %s\n", __func__, strerror(-rc));
         return false;
      }

      // Refresh TOD
      now = GetTod();

      if (rc > 0)
      {
         if (rpt[0] == _rxrpt)
         {
            Dmsg(0, "%s: Out of sync\n", __func__);
            hex_dump(0, rpt, rc);

            // Reset wait time
            target = now + MS_TO_NS(MODBUS_INTERFRAME_TIMEOUT_MS);
         }
         else
         {
            // Non-MODBUS reports are not an issue, just continue waiting
            Dmsg(100, "%s: Non-MODBUS report id %u\n", __func__, rpt[0]);
         }
      }
   }

   return false;
}
