/*
 * apcstatus.c
 *
 * Output STATUS information.
 */

/*
 * Copyright (C) 1999-2005 Kern Sibbald
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

#include "apc.h"

#define DBGLN() do { Dmsg2(100, "%s:%d\n", __FILE__, __LINE__); } while(0)

/* Send the full status of the UPS to the client. */
int output_status(UPSINFO *ups, int sockfd,
   void s_open(UPSINFO *ups),
   void s_write(UPSINFO *ups, const char *fmt, ...),
   int s_close(UPSINFO *ups, int sockfd))
{
   char datetime[MAXSTRING];
   char buf[MAXSTRING];
   time_t now = time(NULL);
   int time_on_batt;
   struct tm tm;
   char status[100];
   unsigned long statflag;

   s_open(ups);

   /*
    * Lock the UPS structure for reading so the driver doesn't haul
    * off and start updating fields on us. Note that this creates
    * the potential for a misbehaving client to create long lock
    * hold times, essentially locking the driver out. This could
    * be mitigated by making a local copy of UPSINFO under the
    * lock, then releasing the lock and feeding the client from
    * the copy.
    */
   read_lock(ups);

   if (ups->poll_time == 0)        /* this is always zero on slave */
      ups->poll_time = now;

   localtime_r(&ups->poll_time, &tm);
   strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);

   /* put the last UPS poll time on the DATE record */
   s_write(ups, "DATE     : %s\n", datetime);

   gethostname(buf, sizeof buf);
   s_write(ups, "HOSTNAME : %s\n", buf);
   s_write(ups, "VERSION  : " APCUPSD_RELEASE " (" ADATE ") " APCUPSD_HOST "\n");

   if (*ups->upsname)
      s_write(ups, "UPSNAME  : %s\n", ups->upsname);

   s_write(ups, "CABLE    : %s\n", ups->cable.long_name);
   s_write(ups, "MODEL    : %s\n", ups->mode.long_name);
   s_write(ups, "UPSMODE  : %s\n", ups->upsclass.long_name);

   localtime_r(&ups->start_time, &tm);
   strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
   s_write(ups, "STARTTIME: %s\n", datetime);

   if (ups->sharenet.type != DISABLE)
      s_write(ups, "SHARE    : %s\n", ups->sharenet.long_name);

   /* If slave, send last update time/date from master */
   if (ups->is_slave()) {    /* we must be a slave */
      if (ups->last_master_connect_time == 0) {
         s_write(ups, "MASTERUPD: No connection to Master\n");
      } else {
         localtime_r(&ups->last_master_connect_time, &tm);
         strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
         s_write(ups, "MASTERUPD: %s\n", datetime);
      }

      s_write(ups, "MASTER   : %s\n", ups->master_name);
   }

   switch (ups->mode.type) {
   case BK:
   case SHAREBASIC:
      if (!ups->is_onbatt()) {
         s_write(ups, "LINEFAIL : OK\n");
         s_write(ups, "BATTSTAT : OK\n");
      } else {
         s_write(ups, "LINEFAIL : DOWN\n");

         if (!ups->is_battlow())
            s_write(ups, "BATTSTAT : RUNNING\n");
         else
            s_write(ups, "BATTSTAT : FAILING\n");
      }

      s_write(ups, "STATFLAG : 0x%08X Status Flag\n", ups->Status);
      break;

   case BKPRO:
   case VS:
      if (!ups->is_onbatt()) {
         s_write(ups, "LINEFAIL : OK\n");
         s_write(ups, "BATTSTAT : OK\n");
         if (ups->is_boost())
            s_write(ups, "MAINS    : LOW\n");
         else if (ups->is_trim())
            s_write(ups, "MAINS    : HIGH\n");
         else
            s_write(ups, "MAINS    : OK\n");
      } else {
         s_write(ups, "LINEFAIL : DOWN\n");
         if (!ups->is_battlow())
            s_write(ups, "BATTSTAT : RUNNING\n");
         else
            s_write(ups, "BATTSTAT : FAILING\n");
      }

      /*
       * If communication is lost the only valid flag at this point
       * is UPS_commlost and all the other flags are possibly wrong.
       * For this reason the old code used to override UPS_online zeroing
       * it for printing purposes.
       * But this needs more careful checking and may be this is not what
       * we want. After all when UPS_commlost is asserted the other flags
       * contain interesting information about the last known state of UPS
       * and Kern's intention is to use them to infer the next action of
       * a disconnected slave. Personally I think this can be true also
       * when apcupsd loses communication over any other kind of transport
       * like serial or usb or others.
       */
      s_write(ups, "STATFLAG : 0x%08X Status Flag\n", ups->Status);

      /* Note! Fall through is wanted */

   case NBKPRO:
   case SMART:
   case SHARESMART:
   case MATRIX:
   case APCSMART_UPS:
   case USB_UPS:
   case TEST_UPS:
   case DUMB_UPS:
   case NETWORK_UPS:
   case SNMP_UPS:
   case PCNET_UPS:
      status[0] = 0;

      /* Now output human readable form */
//      if (ups->is_calibration())
//         astrncat(status, "CAL ", sizeof(status));
DBGLN();
      if (ups->is_trim())
         astrncat(status, "TRIM ", sizeof(status));

      if (ups->is_boost())
         astrncat(status, "BOOST ", sizeof(status));

      if (ups->is_online())
         astrncat(status, "ONLINE ", sizeof(status));

      if (ups->is_onbatt())
         astrncat(status, "ONBATT ", sizeof(status));

      if (ups->is_overload())
         astrncat(status, "OVERLOAD ", sizeof(status));

      if (ups->is_battlow())
         astrncat(status, "LOWBATT ", sizeof(status));

      if (ups->is_replacebatt())
         astrncat(status, "REPLACEBATT ", sizeof(status));

      if (!ups->is_battpresent())
         astrncat(status, "NOBATT ", sizeof(status));

      if (ups->is_slave())
         astrncat(status, "SLAVE ", sizeof(status));

      if (ups->is_slavedown())
         astrncat(status, "SLAVEDOWN", sizeof(status));

      /* These override the above */
      if (ups->is_commlost())
         astrncpy(status, "COMMLOST ", sizeof(status));

      if (ups->is_shutdown())
         astrncpy(status, "SHUTTING DOWN", sizeof(status));

      s_write(ups, "STATUS   : %s\n", status);

      if (ups->UPS_Cap[CI_VLINE])
         s_write(ups, "LINEV    : %03d.%03d Volts\n",
            ups->info.get(CI_VLINE) / 1000, ups->info.get(CI_VLINE) % 1000);
      else
         Dmsg0(20, "NO CI_VLINE\n");

      if (ups->UPS_Cap[CI_LOAD])
         s_write(ups, "LOADPCT  : %03d.%1d Percent Load Capacity\n",
            ups->info.get(CI_LOAD) / 10, ups->info.get(CI_LOAD) % 10);

      if (ups->UPS_Cap[CI_BATTLEV])
         s_write(ups, "BCHARGE  : %03d.%1d Percent\n",
            ups->info.get(CI_BATTLEV) / 10, ups->info.get(CI_BATTLEV) % 10);

DBGLN();
      if (ups->UPS_Cap[CI_RUNTIM])
         s_write(ups, "TIMELEFT : %3d.%1d Minutes\n",
            ups->info.get(CI_RUNTIM) / 60, ups->info.get(CI_RUNTIM) % 60);

//      s_write(ups, "MBATTCHG : %d Percent\n", ups->percent);
//      s_write(ups, "MINTIMEL : %d Minutes\n", ups->runtime);
//      s_write(ups, "MAXTIME  : %d Seconds\n", ups->maxtime);

      if (ups->UPS_Cap[CI_VMAX])
         s_write(ups, "MAXLINEV : %03d.%03d Volts\n",
            ups->info.get(CI_VMAX) / 1000, ups->info.get(CI_VMAX) % 1000);

      if (ups->UPS_Cap[CI_VMIN])
         s_write(ups, "MINLINEV : %03d.%03d Volts\n",
            ups->info.get(CI_VMIN) / 1000, ups->info.get(CI_VMIN) % 1000);

      if (ups->UPS_Cap[CI_VOUT])
         s_write(ups, "OUTPUTV  : %03d.%03d Volts\n",
            ups->info.get(CI_VOUT) / 1000, ups->info.get(CI_VOUT) % 1000);

      if (ups->UPS_Cap[CI_SENS]) {
         switch (ups->info.get(CI_SENS).strval()[0]) {
         case 'A':                /* Matrix only */
            s_write(ups, "SENSE    : Auto Adjust\n");
            break;
         case 'L':
            s_write(ups, "SENSE    : Low\n");
            break;
         case 'M':
            s_write(ups, "SENSE    : Medium\n");
            break;
         case 'H':
            s_write(ups, "SENSE    : High\n");
            break;
         default:
            s_write(ups, "SENSE    : Unknown\n");
            break;
         }
      }

DBGLN();
      if (ups->UPS_Cap[CI_DWAKE])
         s_write(ups, "DWAKE    : %03d Seconds\n", ups->info.get(CI_DWAKE).lval());

      if (ups->UPS_Cap[CI_DSHUTD])
         s_write(ups, "DSHUTD   : %03d Seconds\n", ups->info.get(CI_DSHUTD).lval());

      if (ups->UPS_Cap[CI_DLBATT])
         s_write(ups, "DLOWBATT : %02d Minutes\n", ups->info.get(CI_DLBATT)/60);

      if (ups->UPS_Cap[CI_LTRANS])
         s_write(ups, "LOTRANS  : %03d.0 Volts\n", ups->info.get(CI_LTRANS)/1000);

      if (ups->UPS_Cap[CI_HTRANS])
         s_write(ups, "HITRANS  : %03d.0 Volts\n", ups->info.get(CI_HTRANS)/1000);

      if (ups->UPS_Cap[CI_RETPCT])
         s_write(ups, "RETPCT   : %03d.0 Percent\n", ups->info.get(CI_RETPCT)/10);

      if (ups->UPS_Cap[CI_ITEMP])
         s_write(ups, "ITEMP    : %02d.%1d C Internal\n",
            ups->info.get(CI_ITEMP) / 10, ups->info.get(CI_ITEMP) % 10);

DBGLN();
      if (ups->UPS_Cap[CI_DALARM]) {
         switch (ups->info.get(CI_DALARM).strval()[0]) {
         case 'T':
            s_write(ups, "ALARMDEL : 30 seconds\n");
            break;
         case 'L':
            s_write(ups, "ALARMDEL : Low Battery\n");
            break;
         case 'N':
            s_write(ups, "ALARMDEL : No alarm\n");
            break;
         case '0':
            s_write(ups, "ALARMDEL : 5 seconds\n");
            break;
         default:
            s_write(ups, "ALARMDEL : Always\n");
            break;
         }
      }
DBGLN();

      if (ups->UPS_Cap[CI_VBATT])
         s_write(ups, "BATTV    : %02d.%1d Volts\n",
            ups->info.get(CI_VBATT) / 1000,
            (ups->info.get(CI_VBATT) % 1000) / 100);

DBGLN();
      if (ups->UPS_Cap[CI_FREQ])
         s_write(ups, "LINEFREQ : %02d.%1d Hz\n",
            ups->info.get(CI_FREQ) / 10, (ups->info.get(CI_FREQ) % 10));

DBGLN();
      /* Output cause of last transfer to batteries */
      switch (ups->info.get(CI_WHY_BATT)) {
      case XFER_NONE:
         s_write(ups, "LASTXFER : No transfers since turnon\n");
         break;
      case XFER_SELFTEST:
         s_write(ups, "LASTXFER : Automatic or explicit self test\n");
         break;
      case XFER_FORCED:
         s_write(ups, "LASTXFER : Forced by software\n");
         break;
      case XFER_UNDERVOLT:
         s_write(ups, "LASTXFER : Low line voltage\n");
         break;
      case XFER_OVERVOLT:
         s_write(ups, "LASTXFER : High line voltage\n");
         break;
      case XFER_RIPPLE:
         s_write(ups, "LASTXFER : Unacceptable line voltage changes\n");
         break;
      case XFER_NOTCHSPIKE:
         s_write(ups, "LASTXFER : Line voltage notch or spike\n");
         break;
      case XFER_FREQ:
         s_write(ups, "LASTXFER : Input frequency out of range\n");
         break;
      case XFER_UNKNOWN:
         s_write(ups, "LASTXFER : UNKNOWN EVENT\n");
         break;
      case XFER_NA:
      default:
         /* UPS doesn't report this data */
         break;
      }

DBGLN();
      s_write(ups, "NUMXFERS : %d\n", ups->num_xfers);
      if (ups->num_xfers > 0) {
         localtime_r(&ups->last_onbatt_time, &tm);
         strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
         s_write(ups, "XONBATT  : %s\n", datetime);
      }

      if (ups->is_onbatt() && ups->last_onbatt_time > 0)
         time_on_batt = now - ups->last_onbatt_time;
      else
         time_on_batt = 0;
      s_write(ups, "TONBATT  : %d seconds\n", time_on_batt);
      s_write(ups, "CUMONBATT: %d seconds\n", ups->cum_time_on_batt + time_on_batt);

      if (ups->last_offbatt_time > 0) {
         localtime_r(&ups->last_offbatt_time, &tm);
         strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
         s_write(ups, "XOFFBATT : %s\n", datetime);
      } else {
         s_write(ups, "XOFFBATT : N/A\n");
      }

      if (ups->LastSelfTest != 0) {
         localtime_r(&ups->LastSelfTest, &tm);
         strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
         s_write(ups, "LASTSTEST: %s\n", datetime);
      }

      /* Status of last self test */
      switch (ups->info.get(CI_ST_STAT)) {
      case TEST_NONE:
         s_write(ups, "SELFTEST : NO\n");
         break;
      case TEST_FAILED:
         s_write(ups, "SELFTEST : NG\n");
         break;
      case TEST_WARNING:
         s_write(ups, "SELFTEST : WN\n");
         break;
      case TEST_INPROGRESS:
         s_write(ups, "SELFTEST : IP\n");
         break;
      case TEST_PASSED:
         s_write(ups, "SELFTEST : OK\n");
         break;
      case TEST_FAILCAP:
         s_write(ups, "SELFTEST : BT\n");
         break;
      case TEST_FAILLOAD:
         s_write(ups, "SELFTEST : NG\n");
         break;
      case TEST_UNKNOWN:
         s_write(ups, "SELFTEST : ??\n");
         break;
      case TEST_NA:
      default:
         /* UPS doesn't report this data */
         break;
      }
DBGLN();

      /* Self test interval */
      if (ups->UPS_Cap[CI_STESTI])
         s_write(ups, "STESTI   : %s\n", ups->info.get(CI_STESTI).strval());

      /* output raw bits */
      statflag = ups->Status;
      if (ups->is_onbatt())
         statflag |= UPS_onbatt;
      if (ups->is_online())
         statflag |= UPS_online;
      if (ups->is_trim())
         statflag |= UPS_trim;
      if (ups->is_boost())
         statflag |= UPS_boost;
      if (ups->is_overload())
         statflag |= UPS_overload;
      if (ups->is_battlow())
         statflag |= UPS_battlow;
      if (ups->is_replacebatt())
         statflag |= UPS_replacebatt;
      if (ups->is_battpresent())
         statflag |= UPS_battpresent;
      s_write(ups, "STATFLAG : 0x%08X Status Flag\n", statflag);
/*
      if (ups->UPS_Cap[CI_DIPSW])
         s_write(ups, "DIPSW    : 0x%02X Dip Switch\n", ups->dipsw);

      if (ups->UPS_Cap[CI_REG1])
         s_write(ups, "REG1     : 0x%02X Register 1\n", ups->reg1);

      if (ups->UPS_Cap[CI_REG2])
         s_write(ups, "REG2     : 0x%02X Register 2\n", ups->reg2);

      if (ups->UPS_Cap[CI_REG3])
         s_write(ups, "REG3     : 0x%02X Register 3\n", ups->reg3);
*/
      if (ups->UPS_Cap[CI_MANDAT])
         s_write(ups, "MANDATE  : %s\n", ups->info.get(CI_MANDAT).strval());

      if (ups->UPS_Cap[CI_SERNO])
         s_write(ups, "SERIALNO : %s\n", ups->info.get(CI_SERNO).strval());

//      if (ups->UPS_Cap[CI_BATTDAT] || ups->UPS_Cap[CI_BattReplaceDate])
//         s_write(ups, "BATTDATE : %s\n", ups->battdat);

      if (ups->UPS_Cap[CI_NOMOUTV])
         s_write(ups, "NOMOUTV  : %03d Volts\n", ups->info.get(CI_NOMOUTV) / 1000);

      if (ups->UPS_Cap[CI_NOMINV])
         s_write(ups, "NOMINV   : %03d Volts\n", ups->info.get(CI_NOMINV) / 1000);

      if (ups->UPS_Cap[CI_NOMBATTV])
         s_write(ups, "NOMBATTV : %03d.%1d Volts\n", 
            ups->info.get(CI_NOMBATTV) / 1000,
            (ups->info.get(CI_NOMBATTV) % 1000) / 100);

      if (ups->UPS_Cap[CI_NOMPOWER])
         s_write(ups, "NOMPOWER : %d Watts\n", ups->info.get(CI_NOMPOWER) / 10);
DBGLN();

      if (ups->UPS_Cap[CI_HUMID])
         s_write(ups, "HUMIDITY : %3d.%1d\n",
            ups->info.get(CI_HUMID) / 10, ups->info.get(CI_HUMID) % 10);

      if (ups->UPS_Cap[CI_ATEMP])
         s_write(ups, "AMBTEMP  : %3d.%1d\n",
            ups->info.get(CI_ATEMP) / 10, ups->info.get(CI_ATEMP) % 10);

      if (ups->UPS_Cap[CI_EXTBATTS])
         s_write(ups, "EXTBATTS : %d\n", ups->info.get(CI_EXTBATTS).lval());

      if (ups->UPS_Cap[CI_BADBATTS])
         s_write(ups, "BADBATTS : %d\n", ups->info.get(CI_BADBATTS).lval());

      if (ups->UPS_Cap[CI_REVNO])
         s_write(ups, "FIRMWARE : %s\n", ups->info.get(CI_REVNO).strval());

      if (ups->UPS_Cap[CI_UPSMODEL])
         s_write(ups, "APCMODEL : %s\n", ups->info.get(CI_UPSMODEL).strval());

      break;
DBGLN();

   default:
      break;
   }

   read_unlock(ups);

   /* put the current time in the END APC record */
   localtime_r(&now, &tm);
   strftime(datetime, 100, "%a %b %d %X %Z %Y", &tm);
   s_write(ups, "END APC  : %s\n", datetime);

   return s_close(ups, sockfd);
}

/*
 * stat_open(), stat_close(), and stat_write() are called
 * by output_status() to open,close, and write the status
 * report.
 */
void stat_open(UPSINFO *ups)
{
}

int stat_close(UPSINFO *ups, int fd)
{
   return 0;
}

void stat_print(UPSINFO *ups, const char *fmt, ...)
{
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   vfprintf(stdout, (char *)fmt, arg_ptr);
   va_end(arg_ptr);
}
