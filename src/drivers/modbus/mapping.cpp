/*
 * mapping.cpp
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

#include "mapping.h"

using namespace APCModbusMapping;

// APC MODBUS Registers                                              addr  nr  type    scale
const RegInfo APCModbusMapping::REG_UPS_STATUS                   = {    0,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_UPS_STATUS_CHANGE_CAUSE      = {    2,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_MOG_OUTLET_STATUS            = {    3,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SOG0_OUTLET_STATUS           = {    6,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SOG1_OUTLET_STATUS           = {    9,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SOG2_OUTLET_STATUS           = {   12,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SOG3_OUTLET_STATUS           = {   15,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SIMPLE_SIGNALLING_STATUS     = {   18,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_GENERAL_ERROR                = {   19,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_POWER_SYSTEM_ERROR           = {   20,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_SYSTEM_ERROR         = {   22,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_TEST_STATUS          = {   23,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_CALIBRATION_STATUS           = {   24,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_LIFETIME_STATUS      = {   25,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_USER_INTERFACE_STATUS        = {   26,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_RUNTIME_REMAINING            = {  128,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_STATE_OF_CHARGE_PCT          = {  130,  1, DT_UINT,   9 };
const RegInfo APCModbusMapping::REG_BATTERY_VOLTAGE              = {  131,  1, DT_INT,    5 };
const RegInfo APCModbusMapping::REG_BATTERY_DATE                 = {  133,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_TEMPERATURE          = {  135,  1, DT_INT,    7 };
const RegInfo APCModbusMapping::REG_OUTPUT_0_REAL_POWER_PCT      = {  136,  1, DT_UINT,   8 };
const RegInfo APCModbusMapping::REG_OUTPUT_0_APPARENT_POWER_PCT  = {  138,  1, DT_UINT,   8 };
const RegInfo APCModbusMapping::REG_OUTPUT_0_CURRENT             = {  140,  1, DT_UINT,   5 };
const RegInfo APCModbusMapping::REG_OUTPUT_0_VOLTAGE             = {  142,  1, DT_UINT,   6 };
const RegInfo APCModbusMapping::REG_OUTPUT_FREQUENCY             = {  144,  1, DT_UINT,   7 };
const RegInfo APCModbusMapping::REG_OUTPUT_ENERGY                = {  145,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_INPUT_STATUS                 = {  150,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_INPUT_0_VOLTAGE              = {  151,  1, DT_UINT,   6 };
const RegInfo APCModbusMapping::REG_INPUT_EFFICIENCY             = {  154,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_MOG_TURN_OFF_COUNTDOWN       = {  155,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_MOG_TURN_ON_COUNTDOWN        = {  156,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_MOG_STAY_OFF_COUNTDOWN       = {  157,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_TURN_OFF_COUNTDOWN      = {  159,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_TURN_ON_COUNTDOWN       = {  160,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_STAY_OFF_COUNTDOWN      = {  161,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_TURN_OFF_COUNTDOWN      = {  163,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_TURN_ON_COUNTDOWN       = {  164,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_STAY_OFF_COUNTDOWN      = {  165,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_TURN_OFF_COUNTDOWN      = {  167,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_TURN_ON_COUNTDOWN       = {  168,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_STAY_OFF_COUNTDOWN      = {  169,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_TURN_OFF_COUNTDOWN      = {  171,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_TURN_ON_COUNTDOWN       = {  172,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_STAY_OFF_COUNTDOWN      = {  173,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_FW_VERSION                   = {  516,  8, DT_STRING    };
const RegInfo APCModbusMapping::REG_MODEL                        = {  532, 16, DT_STRING    };
const RegInfo APCModbusMapping::REG_SKU                          = {  548, 16, DT_STRING    };
const RegInfo APCModbusMapping::REG_SERIAL_NUMBER                = {  564,  8, DT_STRING    };
const RegInfo APCModbusMapping::REG_BATTERY_SKU                  = {  572,  8, DT_STRING    };
const RegInfo APCModbusMapping::REG_EXTERNAL_BATTERY_SKU         = {  580,  8, DT_STRING    };
const RegInfo APCModbusMapping::REG_OUTPUT_APPARENT_POWER_RATING = {  588,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_OUTPUT_REAL_POWER_RATING     = {  589,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_SOG_RELAY_CONFIG             = {  590,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_MANUFACTURE_DATE             = {  591,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_OUTPUT_VOLTAGE_SETTING       = {  592,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_DATE_SETTING         = {  595,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_NAME                         = {  596,  8, DT_STRING    };
const RegInfo APCModbusMapping::REG_MOG_TURN_OFF_COUNT_SETTING   = { 1029,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_MOG_TURN_ON_COUNT_SETTING    = { 1030,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_MOG_STAY_OFF_COUNT_SETTING   = { 1031,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_TURN_OFF_COUNT_SETTING  = { 1033,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_TURN_ON_COUNT_SETTING   = { 1034,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG0_STAY_OFF_COUNT_SETTING  = { 1035,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_TURN_OFF_COUNT_SETTING  = { 1037,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_TURN_ON_COUNT_SETTING   = { 1038,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG1_STAY_OFF_COUNT_SETTING  = { 1039,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_TURN_OFF_COUNT_SETTING  = { 1041,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_TURN_ON_COUNT_SETTING   = { 1042,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG2_STAY_OFF_COUNT_SETTING  = { 1043,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_TURN_OFF_COUNT_SETTING  = { 1045,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_TURN_ON_COUNT_SETTING   = { 1046,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_SOG3_STAY_OFF_COUNT_SETTING  = { 1047,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_UPS_CMD                      = { 1536,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_OUTLET_CMD                   = { 1538,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_SIMPLE_SIGNALLING_CMD        = { 1540,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_BATTERY_TEST_CMD             = { 1541,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_CALIBRATION_CMD              = { 1542,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_USER_INTERFACE_CMD           = { 1543,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_MODBUS_MAP_ID                = { 2048,  2, DT_STRING    };
const RegInfo APCModbusMapping::REG_TEST_STRING                  = { 2050,  4, DT_STRING    };
const RegInfo APCModbusMapping::REG_TEST_4B_NUMBER_1             = { 2054,  2, DT_UINT      };
const RegInfo APCModbusMapping::REG_TEST_4B_NUMBER_2             = { 2056,  2, DT_INT       };
const RegInfo APCModbusMapping::REG_TEST_2B_NUMBER_1             = { 2058,  1, DT_UINT      };
const RegInfo APCModbusMapping::REG_TEST_2B_NUMBER_2             = { 2059,  1, DT_INT       };
const RegInfo APCModbusMapping::REG_TEST_BPI_NUMBER_1            = { 2060,  1, DT_INT,    6 };
const RegInfo APCModbusMapping::REG_TEST_BPI_NUMBER_2            = { 2061,  1, DT_INT,    6 };
