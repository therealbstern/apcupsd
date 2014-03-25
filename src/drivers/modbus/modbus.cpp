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

#include "apc.h"
#include "modbus.h"
#include "astring.h"

/* Win32 needs O_BINARY; sane platforms have never heard of it */
#ifndef O_BINARY
#define O_BINARY 0
#endif

using namespace APCModbusMapping;

const ModbusUpsDriver::CiInfo ModbusUpsDriver::CI_TABLE[] =
{
//   ci                  dynamic  addr ln  type
   { CI_UPSMODEL,        false,   &REG_MODEL                        },
   { CI_SERNO,           false,   &REG_SERIAL_NUMBER                },
   { CI_IDEN,            false,   &REG_NAME                         },
   { CI_MANDAT,          false,   &REG_MANUFACTURE_DATE             },
   { CI_BATTDAT,         false,   &REG_BATTERY_DATE_SETTING         },
   { CI_NOMOUTV,         false,   &REG_OUTPUT_VOLTAGE_SETTING       },
   { CI_REVNO,           false,   &REG_FW_VERSION                   },
   { CI_BUPSelfTest,     false,   &REG_MODBUS_MAP_ID                }, // Purposely after CI_REVNO
   { CI_NOMPOWER,        false,   &REG_OUTPUT_REAL_POWER_RATING     }, // TODO: Decide real or apparent power
   { CI_DSHUTD,          false,   &REG_MOG_TURN_OFF_COUNT_SETTING   },
   { CI_DWAKE,           false,   &REG_MOG_TURN_ON_COUNT_SETTING    },
// { CI_NOMBATTV,        false,   upsAdvBatteryNominalVoltage,
// { CI_AlarmTimer,      false,   upsAdvConfigAlarmTimer,
// { CI_DALARM,          false,   upsAdvConfigAlarm,
// { CI_DLBATT,          false,   upsAdvConfigLowBatteryRunTime,
// { CI_RETPCT,          false,   upsAdvConfigMinReturnCapacity,
// { CI_SENS,            false,   upsAdvConfigSensitivity,
// { CI_EXTBATTS,        false,   upsAdvBatteryNumOfBattPacks,
// { CI_STESTI,          false,   
// { CI_LTRANS,          false,    
// { CI_HTRANS,          false,    

   { CI_VLINE,           true,    &REG_INPUT_0_VOLTAGE              },
   { CI_VOUT,            true,    &REG_OUTPUT_0_VOLTAGE             },
   { CI_VBATT,           true,    &REG_BATTERY_VOLTAGE              },
   { CI_FREQ,            true,    &REG_OUTPUT_FREQUENCY             },
   { CI_ITEMP,           true,    &REG_BATTERY_TEMPERATURE          },
   { CI_BATTLEV,         true,    &REG_STATE_OF_CHARGE_PCT          },
   { CI_RUNTIM,          true,    &REG_RUNTIME_REMAINING            },
   { CI_STATUS,          true,    &REG_UPS_STATUS                   },
   { CI_WHY_BATT,        true,    &REG_UPS_STATUS_CHANGE_CAUSE      },// purposely after CI_STATUS
   { CI_Calibration,     true,    &REG_CALIBRATION_STATUS           },
   { CI_ST_STAT,         true,    &REG_BATTERY_TEST_STATUS          },
   { CI_Overload,        true,    &REG_POWER_SYSTEM_ERROR           },
   { CI_NeedReplacement, true,    &REG_BATTERY_SYSTEM_ERROR         },
   { CI_BatteryPresent,  true,    &REG_BATTERY_SYSTEM_ERROR         },
   { CI_Boost,           true,    &REG_INPUT_STATUS                 },
   { CI_Trim,            true,    &REG_INPUT_STATUS                 },
   { CI_LOAD,            true,    &REG_OUTPUT_0_REAL_POWER          }, // TODO: Decide real or apparent power
   { CI_LowBattery,      true,    &REG_SIMPLE_SIGNALLING_STATUS     },
// { CI_VMIN,            true,    upsAdvInputMinLineVoltage
// { CI_VMAX,            true,    upsAdvInputMaxLineVoltage
                                  
// { CI_ATEMP,           true,    mUpsEnvironAmbientTemperature
// { CI_HUMID,           true,    mUpsEnvironRelativeHumidity
// { CI_BADBATTS,        true,    upsAdvBatteryNumOfBadBattPacks

   { -1 }   /* END OF TABLE */   
};

ModbusUpsDriver::ModbusUpsDriver(UPSINFO *ups) :
   UpsDriver(ups),
   _slaveaddr(DEFAULT_SLAVE_ADDR)
{
}

/*
 * Read UPS events. I.e. state changes.
 */
bool ModbusUpsDriver::check_state()
{
   bool ret, onbatt, online;

   // Determine when we need to exit by
   struct timeval exittime, now;
   gettimeofday(&exittime, NULL);
   exittime.tv_sec += _ups->wait_time;

   while(1)
   {
      // See if it's time to exit
      gettimeofday(&now, NULL);
      if (now.tv_sec > exittime.tv_sec ||
          (now.tv_sec == exittime.tv_sec &&
           now.tv_usec >= exittime.tv_usec))
      {
         return false;
      }

      // Try to recover from commlost by reopening port
      ret = !_ups->is_commlost() || Open();
      if (ret)
      {
         // Remember current online/onbatt state
         online = _ups->is_online();
         onbatt = _ups->is_onbatt();

         // Poll CI_STATUS
         ret = UpdateCi(CI_STATUS);
      }

      if (ret)
      {
         // If we were commlost, we're not any more
         if (_ups->is_commlost())
         {
            _ups->clear_commlost();
            generate_event(_ups, CMDCOMMOK);
         }

         // Exit immediately if on battery state changed
         if ((online ^ _ups->is_online()) ||
             (onbatt ^ _ups->is_onbatt()))
         {
            return true;
         }
      }
      else
      {
         time_t now = time(NULL);
         if (_ups->is_commlost())
         {
            // We already know we're commlost.
            // Log an event every 10 minutes.
            if ((now - _commlost_time) >= 10*60)
            {
               _commlost_time = now;
               log_event(_ups, event_msg[CMDCOMMFAILURE].level,
                  event_msg[CMDCOMMFAILURE].msg);            
            }
         }
         else
         {
            // Just became commlost...set commlost flag and log an event.
            _commlost_time = now;
            _ups->set_commlost();
            generate_event(_ups, CMDCOMMFAILURE);
         }
      }

      sleep(1);
   }
}

bool ModbusUpsDriver::Open()
{
   // Close if we're already open
   if (_ups->fd != -1)
   {
      close(_ups->fd);
      _ups->fd = -1;
   }

   char *opendev = _ups->device;

#ifdef HAVE_MINGW
   // On Win32 add \\.\ UNC prefix to COMx in order to correctly address
   // ports >= COM10.
   char device[MAXSTRING];
   if (!strnicmp(_ups->device, "COM", 3)) {
      snprintf(device, sizeof(device), "\\\\.\\%s", _ups->device);
      opendev = device;
   }
#endif

   if ((_ups->fd = open(opendev, O_RDWR | O_NOCTTY | O_NDELAY | O_BINARY)) < 0)
   {
      Dmsg(0, "%s: open(\"%s\") fails: %s\n", __func__, 
         opendev, strerror(errno));
      return false;
   }

   /* Cancel the no delay we just set */
   int cmd = fcntl(_ups->fd, F_GETFL, 0);
   fcntl(_ups->fd, F_SETFL, cmd & ~O_NDELAY);

   /* Save old settings */
   tcgetattr(_ups->fd, &_oldtio);

   _newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
   _newtio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   _newtio.c_oflag = 0;         /* Raw output */
   _newtio.c_lflag = 0;         /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
    defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_NETBSD_OS) || \
    defined(HAVE_QNX_OS)
   _newtio.c_ispeed = B9600;    /* Set input speed */
   _newtio.c_ospeed = B9600;    /* Set output speed */
#endif   /* __openbsd__ || __freebsd__ || __netbsd__  */

   /* read() blocks until at least 1 char is received or 100ms btw chars */
   _newtio.c_cc[VMIN] = 0;
   _newtio.c_cc[VTIME] = 0;

#if defined(HAVE_OSF1_OS) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
   (void)cfsetospeed(&_newtio, B9600);
   (void)cfsetispeed(&_newtio, B9600);
#endif  /* do it the POSIX way */

   tcflush(_ups->fd, TCIFLUSH);
   tcsetattr(_ups->fd, TCSANOW, &_newtio);
   tcflush(_ups->fd, TCIFLUSH);

   return true;
}

bool ModbusUpsDriver::Close()
{
   write_lock(_ups);
   
   close(_ups->fd);
   _ups->fd = -1;

   write_unlock(_ups);
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
bool ModbusUpsDriver::get_capabilities()
{
/////////
   const CiInfo *info = CI_TABLE;
   while (info->reg && info->ci != CI_STATUS)
      info++;
   uint8_t *data = ReadRegister(info->reg->addr, info->reg->nregs);
   if (!data)
   {
      _ups->set_commlost();
      return false;
   }
   delete [] data;
/////////
   for (const CiInfo *info = CI_TABLE; info->reg; info++)
   {
      uint8_t *data = ReadRegister(info->reg->addr, info->reg->nregs);
      if (data)
      {
         _ups->UPS_Cap[info->ci] = true;
         delete [] data;
      }
   }

   return _ups->UPS_Cap[CI_STATUS];
}

const ModbusUpsDriver::CiInfo *ModbusUpsDriver::GetCiInfo(int ci)
{
   const CiInfo *info = CI_TABLE;
   while (info->reg && info->ci != ci)
      info++;
   return info->reg ? info : NULL;
}

bool ModbusUpsDriver::UpdateCi(int ci)
{
   const CiInfo *info = GetCiInfo(ci);
   return info ? UpdateCi(info) : false;
}

bool ModbusUpsDriver::UpdateCi(const CiInfo *info)
{
   uint8_t *data = ReadRegister(info->reg->addr, info->reg->nregs);
   if (!data)
   {
      Dmsg(0, "%s: Failed reading %u/%u\n", __func__, 
         info->reg->addr, info->reg->nregs);
      return false;
   }

   const unsigned int nbytes = info->reg->nregs * sizeof(uint16_t);

   uint64_t uint = 0;
   int64_t sint = 0;
   double dbl = 0;
   astring str;

   if (info->reg->type == DT_STRING)
   {
      // String type...
      for (unsigned int i = 0; i < nbytes; ++i)
      {
         // Extract from response message
         // Constrain to valid ASCII subset defined by APC
         if (data[i] < 0x20 || data[i] > 0x7E)
            str += ' ';
         else
            str += data[i];
      }

      // Strip leading and trailing whitespace
      str.trim();
   }
   else
   {
      // Integer type...
      for (unsigned int i = 0; i < nbytes; ++i)
      {
         // Extract from response message, MSB first
         uint = (uint << 8) | data[i];
      }

      if (info->reg->type == DT_INT)
      {
         // Sign extend
         sint = uint;
         sint <<= (8 * (sizeof(uint) - nbytes));
         sint >>= (8 * (sizeof(uint) - nbytes));
         // Scale
         if (info->reg->scale)
            dbl = (double)sint / (1ULL << info->reg->scale);
      }
      else if (info->reg->scale)
      {
         // Scale
         dbl =  (double)uint / (1ULL << info->reg->scale);
      }
   }

   // Done with data
   delete [] data;

   astring tmpstr;
   struct tm tmp;
   time_t date;
   switch (info->ci)
   {
   case CI_UPSMODEL:
      Dmsg(80, "Got CI_UPSMODEL: %s\n", str.str());
      strlcpy(_ups->upsmodel, str, sizeof(_ups->upsmodel));
      break;
   case CI_SERNO:
      Dmsg(80, "Got CI_SERNO: %s\n", str.str());
      strlcpy(_ups->serial, str, sizeof(_ups->serial));
      break;
   case CI_IDEN:
      Dmsg(80, "Got CI_IDEN: %s\n", str.str());
      strlcpy(_ups->upsname, str, sizeof(_ups->upsname));
      break;
   case CI_REVNO:
      Dmsg(80, "Got CI_REVNO: %s\n", str.str());
      strlcpy(_ups->firmrev, str, sizeof(_ups->firmrev));
      break;
   case CI_BUPSelfTest:
      Dmsg(80, "Got MODBUS_MAP_ID: %s\n", str.str());
      tmpstr = _ups->firmrev; // Append to REVNO
      tmpstr += " / " + str;
      strlcpy(_ups->firmrev, tmpstr, sizeof(_ups->firmrev));
      break;
   case CI_MANDAT:
      Dmsg(80, "Got CI_MANDAT: %llu\n", uint);
      // uint is in days since 1/1/2000
      date = ModbusRegTotime_t(uint);
      gmtime_r(&date, &tmp);
      strftime(_ups->birth, sizeof(_ups->birth), "%Y-%m-%d", &tmp);
      break;
   case CI_BATTDAT:
      Dmsg(80, "Got CI_BATTDAT: %llu\n", uint);
      // uint is in days since 1/1/2000
      date = ModbusRegTotime_t(uint);
      gmtime_r(&date, &tmp);
      strftime(_ups->battdat, sizeof(_ups->battdat), "%Y-%m-%d", &tmp);
      break;
   case CI_NOMOUTV:
      Dmsg(80, "Got CI_NOMOUTV: %llx\n", uint);
      if (uint & OVS_100VAC)
         _ups->NomOutputVoltage = 100;
      else if (uint & OVS_120VAC)
         _ups->NomOutputVoltage = 120;
      else if (uint & OVS_200VAC)
         _ups->NomOutputVoltage = 200;
      else if (uint & OVS_208VAC)
         _ups->NomOutputVoltage = 208;
      else if (uint & OVS_220VAC)
         _ups->NomOutputVoltage = 220;
      else if (uint & OVS_230VAC)
         _ups->NomOutputVoltage = 230;
      else if (uint & OVS_240VAC)
         _ups->NomOutputVoltage = 240;
      else
         _ups->NomOutputVoltage = -1;
      break;
   case CI_NOMPOWER:
      Dmsg(80, "Got CI_NOMPOWER: %llu\n", uint);
      _ups->NomPower = uint;
      break;
   case CI_DSHUTD:
      Dmsg(80, "Got CI_DSHUTD: %lld\n", sint);
      _ups->dshutd = sint;
      break;
   case CI_DWAKE:
      Dmsg(80, "Got CI_DWAKE: %lld\n", sint);
      _ups->dwake = sint;
      break;
   case CI_LowBattery:
      Dmsg(80, "Got CI_LowBattery: %llu\n", uint);
      _ups->set_battlow(uint & SSS_SHUTDOWN_IMMINENT);
      break;
#if 0
   case CI_STESTI:
      Dmsg(80, "Got CI_STESTI: %llx\n", uint);
      if (uint & 0x1)
         strlcpy(_ups->selftest, "OFF", sizeof(_ups->selftest));
      else if (uint & 0x2)
         strlcpy(_ups->selftest, "ON", sizeof(_ups->selftest));
      else if (uint & 0x14)
         strlcpy(_ups->selftest, "168", sizeof(_ups->selftest));
      else if (uint & 0x28)
         strlcpy(_ups->selftest, "336", sizeof(_ups->selftest));
      break;
   case CI_LTRANS:
      Dmsg(80, "Got CI_LTRANS: %llu\n", uint);
      _ups->lotrans = uint;
      break;
   case CI_HTRANS:
      Dmsg(80, "Got CI_HTRANS: %llu\n", uint);
      _ups->hitrans = uint;
      break;
#endif
   case CI_LOAD:
      Dmsg(80, "Got CI_LOAD: %f\n", dbl);
      _ups->UPSLoad = dbl;
      break;
   case CI_VLINE:
      Dmsg(80, "Got CI_VLINE: %f\n", dbl);
      _ups->LineVoltage = dbl;
      break;
   case CI_VOUT:
      Dmsg(80, "Got CI_VOUT: %f\n", dbl);
      _ups->OutputVoltage = dbl;
      break;
   case CI_VBATT:
      Dmsg(80, "Got CI_VBATT: %f\n", dbl);
      _ups->BattVoltage = dbl;
      break;
   case CI_FREQ:
      Dmsg(80, "Got CI_FREQ: %f\n", dbl);
      _ups->LineFreq = dbl;
      break;
   case CI_ITEMP:
      Dmsg(80, "Got CI_ITEMP: %f\n", dbl);
      _ups->UPSTemp = dbl;
      break;
   case CI_BATTLEV:
      Dmsg(80, "Got CI_BATTLEV: %f\n", dbl);
      _ups->BattChg = dbl;
      break;
   case CI_RUNTIM:
      Dmsg(80, "Got CI_RUNTIM: %llu\n", uint);
      _ups->TimeLeft = uint / 60; // secs to mins
      break;
   case CI_ST_STAT:
      Dmsg(80, "Got CI_ST_STAT: 0x%llx\n", uint);
      if (uint & (BTS_PENDING|BTS_IN_PROGRESS))
         _ups->testresult = TEST_INPROGRESS;
      else if (uint & BTS_PASSED)
         _ups->testresult = TEST_PASSED;
      else if (uint & BTS_FAILED)
         _ups->testresult = TEST_FAILED;
      else if (uint == 0)
         _ups->testresult = TEST_NONE;
      else
         _ups->testresult = TEST_UNKNOWN;
      break;
   case CI_WHY_BATT:
      Dmsg(80, "Got CI_WHY_BATT: %llx\n", uint);
      // Only update if we're on battery now
      if (_ups->is_onbatt())
      {
         switch(uint)
         {
         case USCC_HIGH_INPUT_VOLTAGE:
            _ups->lastxfer = XFER_OVERVOLT;
            break;
         case USCC_LOW_INPUT_VOLTAGE:
            _ups->lastxfer = XFER_UNDERVOLT;
            break;
         case USCC_DISTORTED_INPUT:
            _ups->lastxfer = XFER_NOTCHSPIKE;
            break;
         case USCC_RAPID_CHANGE:
            _ups->lastxfer = XFER_RIPPLE;
            break;
         case USCC_HIGH_INPUT_FREQ:
         case USCC_LOW_INPUT_FREQ:
         case USCC_FREQ_PHASE_DIFF:
            _ups->lastxfer = XFER_FREQ;
            break;
         case USCC_AUTOMATIC_TEST:
            _ups->lastxfer = XFER_SELFTEST;
            break;
         case USCC_LOCAL_UI_CMD:
         case USCC_PROTOCOL_CMD:
            _ups->lastxfer = XFER_FORCED;
            break;
         default:
            _ups->lastxfer = XFER_UNKNOWN;
            break;
         }
      }
      break;
   case CI_STATUS:
      Dmsg(80, "Got CI_STATUS: 0x%llx\n", uint);
      // Clear the following flags: only one status will be TRUE
      _ups->clear_online();
      _ups->clear_onbatt();
      if (uint & US_ONLINE)
         _ups->set_online();
      else if (uint & US_ONBATTERY)
         _ups->set_onbatt();
      break;
   case CI_Calibration:
      Dmsg(80, "Got CI_Calibration: 0x%llx\n", uint);
      _ups->set_calibration(uint & (CS_PENDING|CS_IN_PROGRESS));
      break;
   case CI_Overload:
      Dmsg(80, "Got CI_Overload: 0x%llx\n", uint);
      _ups->set_overload(uint & PSE_OUTPUT_OVERLOAD);
      break;
   case CI_NeedReplacement:
      Dmsg(80, "Got CI_NeedReplacement: 0x%llx\n", uint);
      _ups->set_replacebatt(uint & BSE_NEEDS_REPLACEMENT);
      break;
   case CI_BatteryPresent:
      Dmsg(80, "Got CI_BatteryPresent: 0x%llx\n", uint);
      _ups->set_battpresent(!(uint & BSE_DISCONNECTED));
      break;
   case CI_Boost:
      Dmsg(80, "Got CI_Boost: 0x%llx\n", uint);
      _ups->set_boost(uint & IS_BOOST);
      break;
   case CI_Trim:
      Dmsg(80, "Got CI_Trim: 0x%llx\n", uint);
      _ups->set_trim(uint & IS_TRIM);
      break;
   }

   return true;
}

bool ModbusUpsDriver::UpdateCis(bool dynamic)
{
   for (unsigned int i = 0; CI_TABLE[i].ci != -1; i++)
   {
      if (_ups->UPS_Cap[CI_TABLE[i].ci] && CI_TABLE[i].dynamic == dynamic)
      {
         if (!UpdateCi(CI_TABLE+i))
            return false;
      }
   }

   return true;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
bool ModbusUpsDriver::read_static_data()
{
   write_lock(_ups);
   bool ret = UpdateCis(false);
   write_unlock(_ups);
   return ret;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every N seconds to get
 * a current idea of what the UPS is doing.
 */
bool ModbusUpsDriver::read_volatile_data()
{
   write_lock(_ups);
   bool ret = UpdateCis(true);
   if (ret)
   {
      // Successful query, update timestamp
      _ups->poll_time = time(NULL);
   }
   write_unlock(_ups);
   return ret;
}

bool ModbusUpsDriver::kill_power()
{
   return write_int_to_ups(REG_SIMPLE_SIGNALLING_CMD, SSC_REQUEST_SHUTDOWN);
}

bool ModbusUpsDriver::shutdown()
{
   return write_int_to_ups(REG_SIMPLE_SIGNALLING_CMD, SSC_REMOTE_OFF);
}

bool ModbusUpsDriver::entry_point(int command, void *data)
{
   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg(80, "Checking self test.\n");
      /* Reason for last transfer to batteries */
      if (_ups->UPS_Cap[CI_WHY_BATT] && UpdateCis(true))
      {
         Dmsg(80, "Transfer reason: %d\n", _ups->lastxfer);

         /* See if this is a self test rather than power failure */
         if (_ups->lastxfer == XFER_SELFTEST) {
            /*
             * set Self Test start time
             */
            _ups->SelfTest = time(NULL);
            Dmsg(80, "Self Test time: %s", ctime(&_ups->SelfTest));
         }
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
   default:
      return false;
   }

   return true;
}

uint8_t *ModbusUpsDriver::ReadRegister(uint16_t reg, unsigned int nregs)
{
   ModbusPdu txpdu;
   ModbusPdu rxpdu;
   const unsigned int nbytes = nregs * sizeof(uint16_t);

   Dmsg(50, "%s: reg=%u, nregs=%u\n", __func__, reg, nregs);

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

bool ModbusUpsDriver::WriteRegister(uint16_t reg, unsigned int nregs, const uint8_t *data)
{
   ModbusPdu txpdu;
   ModbusPdu rxpdu;
   const unsigned int nbytes = nregs * sizeof(uint16_t);

   Dmsg(50, "%s: reg=%u, nregs=%u\n", __func__, reg, nregs);

#if 0
   txpdu[0] = reg >> 8;
   txpdu[1] = reg;
   txpdu[2] = data[0];
   txpdu[3] = data[1];

   if (!SendAndWait(MODBUS_FC_WRITE_REG, &txpdu, 4, &rxpdu, 4))
   {
      return false;
   }
#else
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
#endif

   return true;
}

bool ModbusUpsDriver::SendAndWait(
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
         Dmsg(0, "%s: Wrong size (exp=%u, rx=%x)\n", __func__, rxsz+4, sz);
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

bool ModbusUpsDriver::ModbusTx(const ModbusFrame *frm, unsigned int sz)
{
   // Wait for line to become idle
   if (!ModbusWaitIdle())
      return false;

   Dmsg(100, "%s: Sending frame\n", __func__);
   hex_dump(100, frm, sz);

   // Write frame
   int rc = write(_ups->fd, frm, sz);
   if (rc == -1)
   {
      Dmsg(0, "%s: write() fails: %s\n", __func__, strerror(errno));
      return false;
   }
   else if ((unsigned int)rc != sz)
   {
      Dmsg(0, "%s: write() short (%d of %u)\n", __func__, rc, sz);
      return false;
   }

   return true;
}

bool ModbusUpsDriver::ModbusRx(ModbusFrame *frm, unsigned int *sz)
{
#ifdef HAVE_MINGW
   // Set read timeout since we have no select() support
   COMMTIMEOUTS ct;
   HANDLE h = (HANDLE)_get_osfhandle(_ups->fd);
   ct.ReadIntervalTimeout = MODBUS_INTERCHAR_TIMEOUT_MS;
   ct.ReadTotalTimeoutMultiplier = 0;
   ct.ReadTotalTimeoutConstant = MODBUS_RESPONSE_TIMEOUT_MS;
   ct.WriteTotalTimeoutMultiplier = 0;
   ct.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(h, &ct);

   int rc = read(_ups->fd, frm, MODBUS_MAX_FRAME_SZ);
   if (rc == -1)
   {
      // Fatal read error
      Dmsg(0, "%s: read() fails: %s\n", __func__, strerror(errno));
      return false;
   }
   else if (rc == 0)
   {
      // Timeout
      Dmsg(0, "%s: timeout\n", __func__);
      return false;
   }

   Dmsg(100, "%s: Received frame\n", __func__);
   hex_dump(100, frm, rc);

   // Received a message ok
   *sz = rc;
   return true;
#else
   unsigned int nread = 0;
   unsigned int timeout = MODBUS_RESPONSE_TIMEOUT_MS;
   struct timeval tv;
   fd_set fds;
   int rc;

   while(1)
   {
      // Wait for character(s) to be available for reading
      do
      {
         FD_ZERO(&fds);
         FD_SET(_ups->fd, &fds);

         tv.tv_sec = timeout / 1000;
         tv.tv_usec = (timeout % 1000) * 1000;

//Dmsg(0, "%s: enter...\n", __func__);
         rc = select(_ups->fd+1, &fds, NULL, NULL, &tv);
//Dmsg(0, "%s: ...exit\n", __func__);
      }
      while (rc == -1 && (errno == EAGAIN || errno == EINTR));

      if (rc == -1)
      {
         // fatal select() error
         Dmsg(0, "%s: select() failed: %s\n", __func__, strerror(errno));
         return false;
      }
      else if (rc == 0)
      {
         // If we've received some characters, this is simply the inter-char
         // timeout signalling the end of the frame...i.e. the success case
         if (nread)
         {

   Dmsg(100, "%s: Received frame\n", __func__);
   hex_dump(100, frm, nread);

            *sz = nread;
            return true;
         }

         // No chars read yet so this is a fatal timeout
         Dmsg(0, "%s: -------------------TIMEOUT\n", __func__);
         return false;
      }

      // Received at least 1 character so switch to inter-char timeout
      timeout = MODBUS_INTERCHAR_TIMEOUT_MS;

      // Read characters
      int rc = read(_ups->fd, *frm+nread, MODBUS_MAX_FRAME_SZ-nread);
      if (rc == -1)
      {
         // Fatal read error
         Dmsg(0, "%s: read() fails: %s\n", __func__, strerror(errno));
         return false;
      }
      nread += rc;

if (rc == 0)
{
         Dmsg(0, "%s: 0-length read\n", __func__);
}

      // Check for max message size
      if (nread == MODBUS_MAX_FRAME_SZ)
      {
         *sz = nread;
         return true;
      }
   }
#endif
}

uint16_t ModbusUpsDriver::ModbusCrc(const uint8_t *data, unsigned int sz)
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

bool ModbusUpsDriver::ModbusWaitIdle()
{
   // Determine when we need to exit by
   struct timeval exittime, now;
   gettimeofday(&exittime, NULL);
   exittime.tv_sec += MODBUS_IDLE_WAIT_TIMEOUT_MS / 1000;
   exittime.tv_usec += (MODBUS_IDLE_WAIT_TIMEOUT_MS % 1000) * 1000;
   if (exittime.tv_usec >= 1000000)
   {
      exittime.tv_sec++;
      exittime.tv_usec -= 1000000;
   }

#ifdef HAVE_MINGW
   // Set read timeout since we have no select() support
   COMMTIMEOUTS ct;
   HANDLE h = (HANDLE)_get_osfhandle(_ups->fd);
   ct.ReadIntervalTimeout = MAXDWORD;
   ct.ReadTotalTimeoutMultiplier = MAXDWORD;
   ct.ReadTotalTimeoutConstant = MODBUS_INTERFRAME_TIMEOUT_MS;
   ct.WriteTotalTimeoutMultiplier = 0;
   ct.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(h, &ct);
#endif

   while (1)
   {
      unsigned char tmp;

#ifdef HAVE_MINGW
      int rc = read(_ups->fd, &tmp, 1);
      if (rc == 0)
      {
         // timeout: line is now idle
         return true;
      }
      else if (rc == -1)
      {
         // fatal read error
         Dmsg(0, "%s: read() failed: %s\n", __func__, strerror(errno));
         return false;
      }

      // char received: unexpected...
      Dmsg(100, "%s: Discarding unexpected character 0x%x\n",
         __func__, tmp);
#else
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(_ups->fd, &fds);

      struct timeval timeout;
      timeout.tv_sec = MODBUS_INTERFRAME_TIMEOUT_MS / 1000;
      timeout.tv_usec = (MODBUS_INTERFRAME_TIMEOUT_MS % 1000) * 1000;
//Dmsg(0, "%s: enter...\n", __func__);
      int rc = select(_ups->fd+1, &fds, NULL, NULL, &timeout);
      if (rc == 0)
      {
         // timeout: line is now idle
//Dmsg(0, "%s: ...exit\n", __func__);
         return true;
      }
      else if (rc == -1 && errno != EINTR && errno != EAGAIN)
      {
         // fatal select() error
         Dmsg(0, "%s: select() failed: %s\n", __func__, strerror(errno));
         return false;
      }
      else if (rc == 1)
      {
         // char received: unexpected...
         read(_ups->fd, &tmp, 1);
         Dmsg(100, "%s: Discarding unexpected character 0x%x\n",
            __func__, tmp);
      }
#endif

      gettimeofday(&now, NULL);
      if (now.tv_sec > exittime.tv_sec ||
          (now.tv_sec == exittime.tv_sec &&
           now.tv_usec >= exittime.tv_usec))
      {
         // Line did not become idle soon enough
         Dmsg(0, "%s: Timeout\n", __func__);
         return false;
      }
   }
}

bool ModbusUpsDriver::write_string_to_ups(const APCModbusMapping::RegInfo &reg, const char *str)
{
   if (reg.type != DT_STRING)
      return false;

   int len = reg.nregs * sizeof(uint16_t);
   astring strcpy = str;
   if (strcpy.len() > len)
   {
      // Truncate
      strcpy = strcpy.substr(0, len);
   }
   else
   {
      // Pad
      while (strcpy.len() < len)
         strcpy += ' ';
   }

   return WriteRegister(reg.addr, reg.nregs, (const uint8_t*)strcpy.str());
}

bool ModbusUpsDriver::write_int_to_ups(const APCModbusMapping::RegInfo &reg, uint64_t val)
{
   if (reg.type != DT_UINT && reg.type != DT_INT)
      return false;

   unsigned int len = reg.nregs * sizeof(uint16_t);
   uint8_t data[len];
   for (unsigned int i = 0; i < len; ++i)
      data[i] = val >> (8*(len-i-1));

   return WriteRegister(reg.addr, reg.nregs, data);
}

bool ModbusUpsDriver::read_string_from_ups(const APCModbusMapping::RegInfo &reg, astring *val)
{
   if (reg.type != DT_STRING)
      return false;

   unsigned int len = reg.nregs * sizeof(uint16_t);
   uint8_t *data = ReadRegister(reg.addr, reg.nregs);
   if (!data)
      return false;

   *val = "";
   for (unsigned int i = 0; i < len; ++i)
   {
      // Extract from response message
      // Constrain to valid ASCII subset defined by APC
      if (data[i] < 0x20 || data[i] > 0x7E)
         *val += ' ';
      else
         *val += data[i];
   }
   delete [] data;

   // Strip leading and trailing whitespace
   val->trim();

   return true;
}

bool ModbusUpsDriver::read_int_from_ups(const APCModbusMapping::RegInfo &reg, uint64_t *val)
{
   if (reg.type != DT_UINT && reg.type != DT_INT)
      return false;

   unsigned int len = reg.nregs * sizeof(uint16_t);
   uint8_t *data = ReadRegister(reg.addr, reg.nregs);
   if (!data)
      return false;

   *val = 0;
   for (unsigned int i = 0; i < len; ++i)
      *val = (*val << 8) | data[i];
   delete [] data;

   return true;
}

bool ModbusUpsDriver::read_dbl_from_ups(const APCModbusMapping::RegInfo &reg, double *val)
{
   if (reg.type != DT_UINT && reg.type != DT_INT)
      return false;

   // Read raw unscaled value from UPS
   uint64_t uint;
   if (!read_int_from_ups(reg, &uint))
      return false;

   // Scale
   if (reg.type == DT_INT)
   {
      // Sign extend
      int64_t sint = uint;
      unsigned int nbytes = reg.nregs * sizeof(uint16_t);
      sint <<= (8 * (sizeof(uint) - nbytes));
      sint >>= (8 * (sizeof(uint) - nbytes));
      *val = (double)sint / (1ULL << reg.scale);
   }
   else
   {
      *val = (double)uint / (1ULL << reg.scale);
   }

   return true;
}
