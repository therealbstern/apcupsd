/*
 * dumboper.c
 *
 * Functions for simple-signalling (dumb) UPS operations
 */

/*
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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

#include "apc.h"
#include "dumb.h"

bool DumbUpsDriver::kill_power()
{
   int serial_bits = 0;

   switch (_ups->cable.type) {
   case CUSTOM_SIMPLE:            /* killpwr_bit */
   case APC_940_0095A:
   case APC_940_0095B:
   case APC_940_0095C:            /* killpwr_bit */
      serial_bits = TIOCM_RTS;
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                  /* hold for 10 seconds */
      serial_bits = TIOCM_ST;
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      break;

   case APC_940_0119A:
   case APC_940_0127A:
   case APC_940_0128A:
   case APC_940_0020B:            /* killpwr_bit */
   case APC_940_0020C:
      serial_bits = TIOCM_DTR;
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                   /* hold for at least 10 seconds */
      break;

   case APC_940_0023A:            /* killpwr_bit */
      break;

   case MAM_CABLE:
      serial_bits = TIOCM_RTS;
      (void)ioctl(_ups->fd, TIOCMBIC, &serial_bits);
      serial_bits = TIOCM_DTR;
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                   /* hold */
      break;
   }
   return 1;
}

/*
 * Dumb UPSes don't have static UPS data.
 */
bool DumbUpsDriver::read_static_data()
{
   return 1;
}

/*
 * Set capabilities.
 */
bool DumbUpsDriver::get_capabilities()
{
   /* We create a Status word */
   _ups->UPS_Cap[CI_STATUS] = TRUE;

   return 1;
}

/*
 * dumb_ups_check_state is the same as dumb_read_ups_volatile_data
 * because this is the only info we can get from UPS.
 *
 * This routine is polled. We should sleep at least 5 seconds
 * unless we are in a FastPoll situation, otherwise, we burn
 * too much CPU.
 */
bool DumbUpsDriver::read_volatile_data()
{
   int stat = 1;

   /*
    * We generally poll a bit faster because we do 
    * not have interrupts like the smarter devices
    */
   if (_ups->wait_time > TIMER_DUMB)
      _ups->wait_time = TIMER_DUMB;

   sleep(_ups->wait_time);

   write_lock(_ups);

   ioctl(_ups->fd, TIOCMGET, &_sp_flags);

   switch (_ups->cable.type) {
   case CUSTOM_SIMPLE:
      /*
       * This is the ONBATT signal sent by UPS.
       */
      if (_sp_flags & TIOCM_CD) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

      if (!(_sp_flags & TIOCM_CTS)) {
         _ups->set_battlow();
      } else {
         _ups->clear_battlow();
      }

      break;

   case APC_940_0119A:
   case APC_940_0127A:
   case APC_940_0128A:
   case APC_940_0020B:
   case APC_940_0020C:
      if (_sp_flags & TIOCM_CTS) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

      if (_sp_flags & TIOCM_CD) {
         _ups->set_battlow();
      } else {
         _ups->clear_battlow();
      }

      break;

   case APC_940_0023A:
      if (_sp_flags & TIOCM_CD) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

/*
 * Code block preserved for posterity in case I ever get a real
 * 940-0023A cable to test. According to schematic in the apcupsd
 * manual, SR is not connected at all. We used to treat it as a
 * battlow indicator, but I have no evidence that it works, and
 * some evidence that it does not.

      if (_sp_flags & TIOCM_SR) {
         _ups->set_battlow();
      } else {
         _ups->clear_battlow();
      }
*/
      break;

   case APC_940_0095A:
   case APC_940_0095C:
      if (_sp_flags & TIOCM_RNG) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

      if (_sp_flags & TIOCM_CD) {
         _ups->set_battlow();
      } else {
         _ups->clear_battlow();
      }

      break;

   case APC_940_0095B:
      if (_sp_flags & TIOCM_RNG) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

      break;

   case MAM_CABLE:
      if (!(_sp_flags & TIOCM_CTS)) {
         _ups->clear_online();
      } else {
         _ups->set_online();
      }

      if (!(_sp_flags & TIOCM_CD)) {
         _ups->set_battlow();
      } else {
         _ups->clear_battlow();
      }

      break;
   }

   _ups->clear_replacebatt();

   write_unlock(_ups);

   return stat;
}

bool DumbUpsDriver::entry_point(int command, void *data)
{
   int serial_bits = 0;

   switch (command) {
   case DEVICE_CMD_DTR_ENABLE:
      if (_ups->cable.type == CUSTOM_SIMPLE) {
         /* 
          * A power failure just happened.
          *
          * Now enable the DTR for the CUSTOM_SIMPLE cable
          * Note: this enables the the CTS bit, which allows
          * us to detect the UPS_battlow condition!!!!
          */
         serial_bits = TIOCM_DTR;
         (void)ioctl(_ups->fd, TIOCMBIS, &serial_bits);
      }
      break;

   case DEVICE_CMD_DTR_ST_DISABLE:
      if (_ups->cable.type == CUSTOM_SIMPLE) {
         /* 
          * Mains power just returned.
          *
          * Clearing DTR and TxD (ST bit).
          */
         serial_bits = TIOCM_DTR | TIOCM_ST;

/* Leave it set */

/*              (void)ioctl(_ups->fd, TIOCMBIC, &serial_bits);
 */
      }
      break;
   default:
      return 0;
      break;
   }
   return 1;
}
