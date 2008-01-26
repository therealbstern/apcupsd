#include "defines.h"
#include "string.h"

// Returns a mapping of Apcupsd CI* to SmartUPS command byte
int *GetSmartCmdMap()
{
   int *cmdmap = new int[CI_MAXCI+1];
   memset(cmdmap, 0, sizeof(int) * (CI_MAXCI+1));

   cmdmap[CI_STATUS] = APC_CMD_STATUS;
   cmdmap[CI_LQUAL] = APC_CMD_LQUAL;
   cmdmap[CI_WHY_BATT] = APC_CMD_WHY_BATT;
   cmdmap[CI_ST_STAT] = APC_CMD_ST_STAT;
   cmdmap[CI_VLINE] = APC_CMD_VLINE;
   cmdmap[CI_VMAX] = APC_CMD_VMAX;
   cmdmap[CI_VMIN] = APC_CMD_VMIN;
   cmdmap[CI_VOUT] = APC_CMD_VOUT;
   cmdmap[CI_BATTLEV] = APC_CMD_BATTLEV;
   cmdmap[CI_VBATT] = APC_CMD_VBATT;
   cmdmap[CI_LOAD] = APC_CMD_LOAD;
   cmdmap[CI_FREQ] = APC_CMD_FREQ;
   cmdmap[CI_RUNTIM] = APC_CMD_RUNTIM;
   cmdmap[CI_ITEMP] = APC_CMD_ITEMP;
   cmdmap[CI_DIPSW] = APC_CMD_DIPSW;
   cmdmap[CI_SENS] = APC_CMD_SENS;
   cmdmap[CI_DWAKE] = APC_CMD_DWAKE;
   cmdmap[CI_DSHUTD] = APC_CMD_DSHUTD;
   cmdmap[CI_LTRANS] = APC_CMD_LTRANS;
   cmdmap[CI_HTRANS] = APC_CMD_HTRANS;
   cmdmap[CI_RETPCT] = APC_CMD_RETPCT;
   cmdmap[CI_DALARM] = APC_CMD_DALARM;
   cmdmap[CI_DLBATT] = APC_CMD_DLBATT;
   cmdmap[CI_IDEN] = APC_CMD_IDEN;
   cmdmap[CI_STESTI] = APC_CMD_STESTI;
   cmdmap[CI_MANDAT] = APC_CMD_MANDAT;
   cmdmap[CI_SERNO] = APC_CMD_SERNO;
   cmdmap[CI_BATTDAT] = APC_CMD_BATTDAT;
   cmdmap[CI_NOMBATTV] = APC_CMD_NOMBATTV;
   cmdmap[CI_HUMID] = APC_CMD_HUMID;
   cmdmap[CI_REVNO] = APC_CMD_REVNO;
   cmdmap[CI_REG1] = APC_CMD_REG1;
   cmdmap[CI_REG2] = APC_CMD_REG2;
   cmdmap[CI_REG3] = APC_CMD_REG3;
   cmdmap[CI_EXTBATTS] = APC_CMD_EXTBATTS;
   cmdmap[CI_ATEMP] = APC_CMD_ATEMP;
   cmdmap[CI_UPSMODEL] = APC_CMD_UPSMODEL;
   cmdmap[CI_NOMOUTV] = APC_CMD_NOMOUTV;
   cmdmap[CI_BADBATTS] = APC_CMD_BADBATTS;
   cmdmap[CI_EPROM] = APC_CMD_EPROM;
   cmdmap[CI_ST_TIME] = APC_CMD_ST_TIME;
   cmdmap[CI_CYCLE_EPROM] = APC_CMD_CYCLE_EPROM;
   cmdmap[CI_UPS_CAPS] = APC_CMD_UPS_CAPS;

   return cmdmap;
}

const char *CItoString(int ci)
{
   switch (ci)
   {
      case CI_UPSMODEL:                return "UPSMODEL";
      case CI_LQUAL:                   return "LQUAL";
      case CI_WHY_BATT:                return "WHY_BATT";
      case CI_ST_STAT:                 return "ST_STAT";
      case CI_VLINE:                   return "VLINE";
      case CI_VMAX:                    return "VMAX";
      case CI_VMIN:                    return "VMIN";
      case CI_VOUT:                    return "VOUT";
      case CI_BATTLEV:                 return "BATTLEV";
      case CI_VBATT:                   return "VBATT";
      case CI_LOAD:                    return "LOAD";
      case CI_FREQ:                    return "FREQ";
      case CI_RUNTIM:                  return "RUNTIM";
      case CI_ITEMP:                   return "ITEMP";
      case CI_DIPSW:                   return "DIPSW";
      case CI_SENS:                    return "SENS";
      case CI_DWAKE:                   return "DWAKE";
      case CI_DSHUTD:                  return "DSHUTD";
      case CI_LTRANS:                  return "LTRANS";
      case CI_HTRANS:                  return "HTRANS";
      case CI_RETPCT:                  return "RETPCT";
      case CI_DALARM:                  return "DALARM";
      case CI_DLBATT:                  return "DLBATT";
      case CI_IDEN:                    return "IDEN";
      case CI_STESTI:                  return "STESTI";
      case CI_MANDAT:                  return "MANDAT";
      case CI_SERNO:                   return "SERNO";
      case CI_BATTDAT:                 return "BATTDAT";
      case CI_NOMBATTV:                return "NOMBATTV";
      case CI_HUMID:                   return "HUMID";
      case CI_REVNO:                   return "REVNO";
      case CI_REG1:                    return "REG1";
      case CI_REG2:                    return "REG2";
      case CI_REG3:                    return "REG3";
      case CI_EXTBATTS:                return "EXTBATTS";
      case CI_ATEMP:                   return "ATEMP";
      case CI_NOMOUTV:                 return "NOMOUTV";
      case CI_BADBATTS:                return "BADBATTS";
      case CI_EPROM:                   return "EPROM";
      case CI_ST_TIME:                 return "ST_TIME";
      case CI_Manufacturer:            return "Manufacturer";
      case CI_ShutdownRequested:       return "ShutdownRequested";
      case CI_ShutdownImminent:        return "ShutdownImminent";
      case CI_DelayBeforeReboot:       return "DelayBeforeReboot";
      case CI_BelowRemCapLimit:        return "BelowRemCapLimit";
      case CI_RemTimeLimitExpired:     return "RemTimeLimitExpired";
      case CI_Charging:                return "Charging";
      case CI_Discharging:             return "Discharging";
      case CI_RemCapLimit:             return "RemCapLimit";
      case CI_RemTimeLimit:            return "RemTimeLimit";
      case CI_WarningCapacityLimit:    return "WarningCapacityLimit";
      case CI_CapacityMode:            return "CapacityMode";
      case CI_BattPackLevel:           return "BattPackLevel";
      case CI_CycleCount:              return "CycleCount";
      case CI_ACPresent:               return "ACPresent";
      case CI_Boost:                   return "Boost";
      case CI_Trim:                    return "Trim";
      case CI_Overload:                return "Overload";
      case CI_NeedReplacement:         return "NeedReplacement";
      case CI_BattReplaceDate:         return "BattReplaceDate";
      case CI_APCForceShutdown:        return "APCForceShutdown";
      case CI_DelayBeforeShutdown:     return "DelayBeforeShutdown";
      case CI_APCDelayBeforeStartup:   return "APCDelayBeforeStartup";
      case CI_APCDelayBeforeShutdown:  return "APCDelayBeforeShutdown";
      case CI_NOMINV:                  return "NOMINV";
      case CI_NOMPOWER:                return "NOMPOWER";
      case CI_BatteryPresent:          return "BatteryPresent";
      case CI_BattLow:                 return "BattLow";

      /* Only seen on the BackUPS Pro USB (so far) */
      case CI_BUPBattCapBeforeStartup: return "BUPBattCapBeforeStartup";
      case CI_BUPDelayBeforeStartup:   return "BUPDelayBeforeStartup";
      case CI_BUPSelfTest:             return "BUPSelfTest";
      case CI_BUPHibernate:            return "BUPHibernate";

      /*
       * We don't actually handle these, but use them as a signal
       * to re-examine the other UPS data items. (USB only)
       */
      case CI_IFailure:                return "IFailure";
      case CI_PWVoltageOOR:            return "PWVoltageOOR";
      case CI_PWFrequencyOOR:          return "PWFrequencyOOR";
      case CI_OverCharged:             return "OverCharged";
      case CI_OverTemp:                return "OverTemp";
      case CI_CommunicationLost:       return "CommunicationLost";
      case CI_ChargerVoltageOOR:       return "ChargerVoltageOOR";
      case CI_ChargerCurrentOOR:       return "ChargerCurrentOOR";
      case CI_CurrentNotRegulated:     return "CurrentNotRegulated";
      case CI_VoltageNotRegulated:     return "VoltageNotRegulated";

      /* Items below this line are not "probed" for */
      case CI_CYCLE_EPROM:             return "CYCLE_EPROM";
      case CI_UPS_CAPS:                return "UPS_CAPS";
      case CI_STATUS:                  return "STATUS";

      default:                         return "UNKNOWN";
   }
}
