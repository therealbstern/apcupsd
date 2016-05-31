/*
 * mapping.h
 *
 * APC MODBUS register mappings
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
 * Register mapping information below was derived from APC/Schneider Electric
 * Application Note #176 <http://www.apc.com/whitepaper/?an=176>
 *
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

#ifndef __APCMODBUSMAPPING_H_
#define __APCMODBUSMAPPING_H_

#include <stdint.h>
#include <time.h>

namespace APCModbusMapping
{
   enum DataType
   {
      DT_STRING,  // ASCII string
      DT_UINT,    // Unsigned integer or float
      DT_INT,     // Signed integer or float
   };

   struct RegInfo
   {
      // MODBUS address of first register
      uint16_t addr;
      // Number of consecutive registers that comprise this data element
      uint16_t nregs;
      // Type of the data
      DataType type;
      // Scaling to be applied to convert INT/UINT to float
      uint8_t scale;
   };

   // Dates in APC MODBUS registers are expressed as number of days since 
   // 1/1/2000. This constant represents 1/1/2000 (UTC) as UNIX timestamp.
   static const time_t MODBUS_BASE_TIMESTAMP = 946684800;

   inline time_t ModbusRegTotime_t(uint64_t reg)
      { return MODBUS_BASE_TIMESTAMP + reg * 60 * 60 * 24; }

   inline uint64_t time_tToModbusReg(time_t t)
      { return (t - MODBUS_BASE_TIMESTAMP) / 60 / 60 / 24; }

   // UPS_STATUS register bits
   static const uint32_t US_ONLINE              = (1U <<  1);
   static const uint32_t US_ONBATTERY           = (1U <<  2);
   static const uint32_t US_OUTPUTOFF           = (1U <<  4);
   static const uint32_t US_FAULT               = (1U <<  5);
   static const uint32_t US_INPUTBAD            = (1U <<  6);
   static const uint32_t US_TEST                = (1U <<  7);
   static const uint32_t US_PENDING_OUTPUTON    = (1U <<  8);
   static const uint32_t US_PENDING_OUTPUTOFF   = (1U <<  9);
   static const uint32_t US_HIGH_EFFICIENCY     = (1U << 13);
   static const uint32_t US_INFO_ALERT          = (1U << 14);

   // SIMPLE_SIGNALING_STATUS register bits
   static const uint32_t SSS_POWER_FAILURE      = (1U << 0);
   static const uint32_t SSS_SHUTDOWN_IMMINENT  = (1U << 1);

   // GENERAL_ERROR register bits
   static const uint32_t GE_SITE_WIRING         = (1U << 0);
   static const uint32_t GE_EEPROM              = (1U << 1);
   static const uint32_t GE_ADC                 = (1U << 2);
   static const uint32_t GE_LOGIC_PS            = (1U << 3);
   static const uint32_t GE_INTERNAL_COMMS      = (1U << 4);
   static const uint32_t GE_UI_BUTTON           = (1U << 5);
   static const uint32_t GE_EPO_ACTIVE          = (1U << 7);

   // POWER_SYSTEM_ERROR register bits
   static const uint32_t PSE_OUTPUT_OVERLOAD    = (1U << 0);
   static const uint32_t PSE_OUTPUT_SHORT       = (1U << 1);
   static const uint32_t PSE_OUTPUT_OVERVOLT    = (1U << 2);
   static const uint32_t PSE_OUTPUT_OVERTEMP    = (1U << 4);
   static const uint32_t PSE_BACKFEED_RELAY     = (1U << 5);
   static const uint32_t PSE_AVR_RELAY          = (1U << 6);
   static const uint32_t PSE_PFCINPUT_RELAY     = (1U << 7);
   static const uint32_t PSE_OUTPUT_RELAY       = (1U << 8);
   static const uint32_t PSE_BYPASS_RELAY       = (1U << 9);
   static const uint32_t PSE_PFC                = (1U << 11);
   static const uint32_t PSE_DC_BUS_OVERVOLT    = (1U << 12);
   static const uint32_t PSE_INVERTER           = (1U << 13);

   // BATTERY_SYSTEM_ERROR register bits
   static const uint32_t BSE_DISCONNECTED       = (1U << 0);
   static const uint32_t BSE_OVERVOLT           = (1U << 1);
   static const uint32_t BSE_NEEDS_REPLACEMENT  = (1U << 2);
   static const uint32_t BSE_OVERTEMP           = (1U << 3);
   static const uint32_t BSE_CHARGER            = (1U << 4);
   static const uint32_t BSE_TEMP_SENSOR        = (1U << 5);
   static const uint32_t BSE_BUS_SOFT_START     = (1U << 6);

   // BATTERY_TEST_STATUS register bits
   static const uint32_t BTS_PENDING            = (1U << 0);   // result
   static const uint32_t BTS_IN_PROGRESS        = (1U << 1);   // result
   static const uint32_t BTS_PASSED             = (1U << 2);   // result
   static const uint32_t BTS_FAILED             = (1U << 3);   // result
   static const uint32_t BTS_REFUSED            = (1U << 4);   // result
   static const uint32_t BTS_ABORTED            = (1U << 5);   // result
   static const uint32_t BTS_SRC_PROTOCOL       = (1U << 6);   // source
   static const uint32_t BTS_SRC_LOCAL_UI       = (1U << 7);   // source
   static const uint32_t BTS_SRC_INTERNAL       = (1U << 8);   // source
   static const uint32_t BTS_INVALID_STATE      = (1U << 9);   // result modifier
   static const uint32_t BTS_INTERNAL_FAULT     = (1U << 10);  // result modifier
   static const uint32_t BTS_STATE_OF_CHARGE    = (1U << 11);  // result modifier

   // CALIBRATION_STATUS register bits
   static const uint32_t CS_PENDING             = (1U << 0);   // result
   static const uint32_t CS_IN_PROGRESS         = (1U << 1);   // result
   static const uint32_t CS_PASSED              = (1U << 2);   // result
   static const uint32_t CS_FAILED              = (1U << 3);   // result
   static const uint32_t CS_REFUSED             = (1U << 4);   // result
   static const uint32_t CS_ABORTED             = (1U << 5);   // result
   static const uint32_t CS_SRC_PROTOCOL        = (1U << 6);   // source
   static const uint32_t CS_SRC_LOCAL_UI        = (1U << 7);   // source
   static const uint32_t CS_SRC_INTERNAL        = (1U << 8);   // source
   static const uint32_t CS_INVALID_STATE       = (1U << 9);   // result modifier
   static const uint32_t CS_INTERNAL_FAULT      = (1U << 10);  // result modifier
   static const uint32_t CS_STATE_OF_CHARGE     = (1U << 11);  // result modifier
   static const uint32_t CS_LOAD_CHANGE         = (1U << 12);  // result modifier
   static const uint32_t CS_AC_INPUT_BAD        = (1U << 13);  // result modifier
   static const uint32_t CS_LOAD_TOO_LOW        = (1U << 14);  // result modifier
   static const uint32_t CS_OVER_CHARGE         = (1U << 15);  // result modifier

   // BATTERY_LIFETIME_STATUS register bits
   static const uint32_t BLS_OK                 = (1U << 0);
   static const uint32_t BLS_NEAR_END           = (1U << 1);
   static const uint32_t BLS_EXCEEDED           = (1U << 2);
   static const uint32_t BLS_NEAR_END_ACK       = (1U << 3);
   static const uint32_t BLS_EXCEEDED_ACK       = (1U << 4);

   // USER_INTERFACE_STATUS register bits
   static const uint32_t UIS_TEST_IN_PROGRESS   = (1U << 0);
   static const uint32_t UIS_AUDIBLE_ALARM      = (1U << 1);
   static const uint32_t UIS_ALARM_MUTED        = (1U << 2);

   // INPUT_STATUS register bits
   static const uint32_t IS_ACCEPTABLE          = (1U << 0);
   static const uint32_t IS_PENDING_ACCEPTABLE  = (1U << 1);
   static const uint32_t IS_LOW_VOLTAGE         = (1U << 2);
   static const uint32_t IS_HIGH_VOLTAGE        = (1U << 3);
   static const uint32_t IS_DISTORTED           = (1U << 4);
   static const uint32_t IS_BOOST               = (1U << 5);
   static const uint32_t IS_TRIM                = (1U << 6);
   static const uint32_t IS_LOW_FREQ            = (1U << 7);
   static const uint32_t IS_HIGH_FREQ           = (1U << 8);
   static const uint32_t IS_FREQ_PHASE_UNLOCKED = (1U << 9);

   // OUTPUT_VOLTAGE_SETTING register bits
   static const uint32_t OVS_100VAC             = (1U << 0);
   static const uint32_t OVS_120VAC             = (1U << 1);
   static const uint32_t OVS_200VAC             = (1U << 2);
   static const uint32_t OVS_208VAC             = (1U << 3);
   static const uint32_t OVS_220VAC             = (1U << 4);
   static const uint32_t OVS_230VAC             = (1U << 5);
   static const uint32_t OVS_240VAC             = (1U << 6);

   // SIMPLE_SIGNALING_CMD register bits
   static const uint32_t SSC_REQUEST_SHUTDOWN   = (1U << 0);
   static const uint32_t SSC_REMOTE_OFF         = (1U << 1);
   static const uint32_t SSC_REMOTE_ON          = (1U << 2);

   // BATTERY_TEST_CMD register bits
   static const uint32_t BTC_START_TEST         = (1U << 0);
   
   // CALIBRATION_CMD register bits
   static const uint32_t CC_START_CALIBRATION   = (1U << 0);
   static const uint32_t CC_ABORT_CALIBRATION   = (1U << 1);
   
   // USER_INTERFACE_CMD register bits
   static const uint32_t UIC_SHORT_TEST         = (1U << 0);
   static const uint32_t UIC_CONTINUOUS_TEST    = (1U << 1);
   static const uint32_t UIC_MUTE_ALARMS        = (1U << 2);
   static const uint32_t UIC_UNMUTE_ALARMS      = (1U << 3);
   static const uint32_t UIC_ACK_BATTERY_ALARMS = (1U << 5);

   // OUTLET_STATUS register bits
   static const uint32_t OS_STATE_ON               = (1U << 0);
   static const uint32_t OS_STATE_OFF              = (1U << 1);
   static const uint32_t OS_PROCESS_REBOOT         = (1U << 2);   // modifier
   static const uint32_t OS_PROCESS_SHUTDOWN       = (1U << 3);   // modifier
   static const uint32_t OS_PROCESS_SLEEP          = (1U << 4);   // modifier
   static const uint32_t OS_PEND_OFF_DELAY         = (1U << 7);   // modifier
   static const uint32_t OS_PEND_ON_AC_PRESENCE    = (1U << 8);   // modifier
   static const uint32_t OS_PEND_ON_MIN_RUNTIME    = (1U << 9);   // modifier
   static const uint32_t OS_MEMBER_GROUP_PROC1     = (1U << 10);  // modifier
   static const uint32_t OS_MEMBER_GROUP_PROC2     = (1U << 11);  // modifier
   static const uint32_t OS_LOW_RUNTIME            = (1U << 12);  // modifier

   // OUTLET_COMMAND register bits
   static const uint32_t OC_CANCEL              = (1U << 0);   // command
   static const uint32_t OC_OUTPUT_ON           = (1U << 1);   // command
   static const uint32_t OC_OUTPUT_OFF          = (1U << 2);   // command
   static const uint32_t OC_OUTPUT_SHUTDOWN     = (1U << 3);   // command
   static const uint32_t OC_OUTPUT_REBOOT       = (1U << 4);   // command
   static const uint32_t OC_COLD_BOOT_ALLOWED   = (1U << 5);   // modifier
   static const uint32_t OC_USE_ON_DELAY        = (1U << 6);   // modifier
   static const uint32_t OC_USE_OFF_DELAY       = (1U << 7);   // modifier
   static const uint32_t OC_TARGET_UOG          = (1U << 8);   // target
   static const uint32_t OC_TARGET_SOG0         = (1U << 9);   // target
   static const uint32_t OC_TARGET_SOG1         = (1U << 10);  // target
   static const uint32_t OC_TARGET_SOG2         = (1U << 11);  // target
   static const uint32_t OC_SRC_USB             = (1U << 12);  // source
   static const uint32_t OC_SRC_LOCAL_UI        = (1U << 13);  // source
   static const uint32_t OC_SRC_RJ45            = (1U << 14);  // source
   static const uint32_t OC_SRC_SMARTSLOT1      = (1U << 15);  // source

   // SOG_RELAY_CONFIG register bits
   static const uint32_t SRC_MOG_PRESENT        = (1U << 0);
   static const uint32_t SRC_SOG0_PRESENT       = (1U << 1);
   static const uint32_t SRC_SOG1_PRESENT       = (1U << 2);
   static const uint32_t SRC_SOG2_PRESENT       = (1U << 3);
   static const uint32_t SRC_SOG3_PRESENT       = (1U << 4);

   // UPS_STATUS_CHANGE_CAUSE enum
   static const uint32_t USCC_SYSTEM_INIT                    = 0;
   static const uint32_t USCC_HIGH_INPUT_VOLTAGE             = 1;
   static const uint32_t USCC_LOW_INPUT_VOLTAGE              = 2;
   static const uint32_t USCC_DISTORTED_INPUT                = 3;
   static const uint32_t USCC_RAPID_CHANGE                   = 4;
   static const uint32_t USCC_HIGH_INPUT_FREQ                = 5;
   static const uint32_t USCC_LOW_INPUT_FREQ                 = 6;
   static const uint32_t USCC_FREQ_PHASE_DIFF                = 7;
   static const uint32_t USCC_ACCEPTABLE_INPUT               = 8;
   static const uint32_t USCC_AUTOMATIC_TEST                 = 9;
   static const uint32_t USCC_TEST_ENDED                     = 10;
   static const uint32_t USCC_LOCAL_UI_CMD                   = 11;
   static const uint32_t USCC_PROTOCOL_CMD                   = 12;
   static const uint32_t USCC_LOW_BATTERY_VOLTAGE            = 13;
   static const uint32_t USCC_GENERAL_ERROR                  = 14;
   static const uint32_t USCC_POWER_SYSTEM_ERROR             = 15;
   static const uint32_t USCC_BATTERY_SYSTEM_ERROR           = 16;
   static const uint32_t USCC_ERROR_CLEARED                  = 17;
   static const uint32_t USCC_AUTO_RESTART                   = 18;
   static const uint32_t USCC_OUTPUT_DISTORTED               = 19;
   static const uint32_t USCC_OUTPUT_ACCEPTABLE              = 20;
   static const uint32_t USCC_EPO_INTERFACE                  = 21;
   static const uint32_t USCC_INPUT_PHASE_DELTA_OUT_OF_RANGE = 22;
   static const uint32_t USCC_INPUT_NEUTRAL_DISCONNECTED     = 23;
   static const uint32_t USCC_ATS_TRANSFER                   = 24;
   static const uint32_t USCC_CONFIG_CHANGE                  = 25;
   static const uint32_t USCC_ALERT_ASSERTED                 = 26;
   static const uint32_t USCC_ALERT_CLEARED                  = 27;
   static const uint32_t USCC_PLUG_RATING_EXCEEDED           = 28;
   static const uint32_t USCC_OUTLET_GRP_STATE_CHANGE        = 29;
   static const uint32_t USCC_FAILURE_BYPASS_EXPIRED         = 30;

   // Registers
   extern const RegInfo REG_UPS_STATUS;
   extern const RegInfo REG_UPS_STATUS_CHANGE_CAUSE;
   extern const RegInfo REG_MOG_OUTLET_STATUS;
   extern const RegInfo REG_SOG0_OUTLET_STATUS;
   extern const RegInfo REG_SOG1_OUTLET_STATUS;
   extern const RegInfo REG_SOG2_OUTLET_STATUS;
   extern const RegInfo REG_SOG3_OUTLET_STATUS;
   extern const RegInfo REG_SIMPLE_SIGNALLING_STATUS;
   extern const RegInfo REG_GENERAL_ERROR;
   extern const RegInfo REG_POWER_SYSTEM_ERROR;
   extern const RegInfo REG_BATTERY_SYSTEM_ERROR;
   extern const RegInfo REG_BATTERY_TEST_STATUS;
   extern const RegInfo REG_CALIBRATION_STATUS;
   extern const RegInfo REG_BATTERY_LIFETIME_STATUS;
   extern const RegInfo REG_USER_INTERFACE_STATUS;
   extern const RegInfo REG_RUNTIME_REMAINING;
   extern const RegInfo REG_STATE_OF_CHARGE_PCT;
   extern const RegInfo REG_BATTERY_VOLTAGE;
   extern const RegInfo REG_BATTERY_DATE;
   extern const RegInfo REG_BATTERY_TEMPERATURE;
   extern const RegInfo REG_OUTPUT_0_REAL_POWER_PCT;
   extern const RegInfo REG_OUTPUT_0_APPARENT_POWER_PCT;
   extern const RegInfo REG_OUTPUT_0_CURRENT;
   extern const RegInfo REG_OUTPUT_0_VOLTAGE;
   extern const RegInfo REG_OUTPUT_FREQUENCY;
   extern const RegInfo REG_OUTPUT_ENERGY;
   extern const RegInfo REG_INPUT_STATUS;
   extern const RegInfo REG_INPUT_0_VOLTAGE;
   extern const RegInfo REG_INPUT_EFFICIENCY;
   extern const RegInfo REG_MOG_TURN_OFF_COUNTDOWN;
   extern const RegInfo REG_MOG_TURN_ON_COUNTDOWN;
   extern const RegInfo REG_MOG_STAY_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG0_TURN_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG0_TURN_ON_COUNTDOWN;
   extern const RegInfo REG_SOG0_STAY_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG1_TURN_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG1_TURN_ON_COUNTDOWN;
   extern const RegInfo REG_SOG1_STAY_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG2_TURN_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG2_TURN_ON_COUNTDOWN;
   extern const RegInfo REG_SOG2_STAY_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG3_TURN_OFF_COUNTDOWN;
   extern const RegInfo REG_SOG3_TURN_ON_COUNTDOWN;
   extern const RegInfo REG_SOG3_STAY_OFF_COUNTDOWN;
   extern const RegInfo REG_FW_VERSION;
   extern const RegInfo REG_MODEL;
   extern const RegInfo REG_SKU;
   extern const RegInfo REG_SERIAL_NUMBER;
   extern const RegInfo REG_BATTERY_SKU;
   extern const RegInfo REG_EXTERNAL_BATTERY_SKU;
   extern const RegInfo REG_OUTPUT_APPARENT_POWER_RATING;
   extern const RegInfo REG_OUTPUT_REAL_POWER_RATING;
   extern const RegInfo REG_SOG_RELAY_CONFIG;
   extern const RegInfo REG_MANUFACTURE_DATE;
   extern const RegInfo REG_OUTPUT_VOLTAGE_SETTING;
   extern const RegInfo REG_BATTERY_DATE_SETTING;
   extern const RegInfo REG_NAME;
   extern const RegInfo REG_MOG_TURN_OFF_COUNT_SETTING;
   extern const RegInfo REG_MOG_TURN_ON_COUNT_SETTING;
   extern const RegInfo REG_MOG_STAY_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG0_TURN_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG0_TURN_ON_COUNT_SETTING;
   extern const RegInfo REG_SOG0_STAY_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG1_TURN_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG1_TURN_ON_COUNT_SETTING;
   extern const RegInfo REG_SOG1_STAY_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG2_TURN_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG2_TURN_ON_COUNT_SETTING;
   extern const RegInfo REG_SOG2_STAY_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG3_TURN_OFF_COUNT_SETTING;
   extern const RegInfo REG_SOG3_TURN_ON_COUNT_SETTING;
   extern const RegInfo REG_SOG3_STAY_OFF_COUNT_SETTING;
   extern const RegInfo REG_UPS_CMD;
   extern const RegInfo REG_OUTLET_CMD;
   extern const RegInfo REG_SIMPLE_SIGNALLING_CMD;
   extern const RegInfo REG_BATTERY_TEST_CMD;
   extern const RegInfo REG_CALIBRATION_CMD;
   extern const RegInfo REG_USER_INTERFACE_CMD;
   extern const RegInfo REG_MODBUS_MAP_ID;
   extern const RegInfo REG_TEST_STRING;
   extern const RegInfo REG_TEST_4B_NUMBER_1;
   extern const RegInfo REG_TEST_4B_NUMBER_2;
   extern const RegInfo REG_TEST_2B_NUMBER_1;
   extern const RegInfo REG_TEST_2B_NUMBER_2;
   extern const RegInfo REG_TEST_BPI_NUMBER_1;
   extern const RegInfo REG_TEST_BPI_NUMBER_2;
};

#endif // __APCMODBUSMAPPING_H_
