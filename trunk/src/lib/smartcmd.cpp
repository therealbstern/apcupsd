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
