#include "apc.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define PCNET_PORT   3052

/* Convert UPS response to enum and string */
static SelfTestResult decode_testresult(const char* str)
{
   /*
    * Responses are:
    * "OK" - good battery, 
    * "BT" - failed due to insufficient capacity, 
    * "NG" - failed due to overload, 
    * "NO" - no results available (no test performed in last 5 minutes) 
    */
   if (str[0] == 'O' && str[1] == 'K')
      return TEST_PASSED;
   else if (str[0] == 'B' && str[1] == 'T')
      return TEST_FAILCAP;
   else if (str[0] == 'N' && str[1] == 'G')
      return TEST_FAILLOAD;

   return TEST_NONE;
}

/* Convert UPS response to enum and string */
static LastXferCause decode_lastxfer(const char *str)
{
   Dmsg1(80, "Transfer reason: %c\n", *str);

   switch (*str) {
   case 'N':
      return XFER_NA;
   case 'R':
      return XFER_RIPPLE;
   case 'H':
      return XFER_OVERVOLT;
   case 'L':
      return XFER_UNDERVOLT;
   case 'T':
      return XFER_NOTCHSPIKE;
   case 'O':
      return XFER_NONE;
   case 'K':
      return XFER_FORCED;
   case 'S':
      return XFER_SELFTEST;
   default:
      return XFER_UNKNOWN;
   }
}

static void pcnet_process_data(UPSINFO* ups, const char *key, const char *value)
{
   unsigned long cmd;
   int ci;

   Dmsg2(200, "pcnet_process_data: key='%s' value='%s'\n", key, value);

   /* Make sure we have a value */
   if (*value == '\0')
      return;

   /* Key must be 2 hex digits */
   if (!isxdigit(key[0]) || !isxdigit(key[1]))
      return;

   /* Convert command to CI */
   cmd = strtoul(key, NULL, 16);
   for (ci=0; ci<CI_MAXCI; ci++)
      if (ups->UPS_Cmd[ci] == cmd)
         break;

   /* No match? */
   if (ci == CI_MAXCI)
      return;

   /* Mark this CI as available */
   ups->UPS_Cap[ci] = true;

   /* Have a connection now */
   ups->clear_commlost();

   /* Handle the data */
   switch (ci) {
      /*
       * VOLATILE DATA
       */
   case CI_STATUS:
      Dmsg1(80, "Got CI_STATUS: %s\n", value);
      ups->Status &= ~0xFF;        /* clear APC byte */
      ups->Status |= strtoul(value, NULL, 16) & 0xFF;  /* set APC byte */
      break;
   case CI_LQUAL:
      Dmsg1(80, "Got CI_LQUAL: %s\n", value);
      strncpy(ups->linequal, value, sizeof(ups->linequal));
      break;
   case CI_WHY_BATT:
      Dmsg1(80, "Got CI_WHY_BATT: %s\n", value);
      ups->lastxfer = decode_lastxfer(value);
      break;
   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %s\n", value);
      ups->testresult = decode_testresult(value);
      break;
   case CI_VLINE:
      Dmsg1(80, "Got CI_VLINE: %s\n", value);
      ups->LineVoltage = atof(value);
      break;
   case CI_VMIN:
      Dmsg1(80, "Got CI_VMIN: %s\n", value);
      ups->LineMin = atof(value);
      break;
   case CI_VMAX:
      Dmsg1(80, "Got CI_VMAX: %s\n", value);
      ups->LineMax = atof(value);
      break;
   case CI_VOUT:
      Dmsg1(80, "Got CI_VOUT: %s\n", value);
      ups->OutputVoltage = atof(value);
      break;
   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %s\n", value);
      ups->BattChg = atof(value);
      break;
   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %s\n", value);
      ups->BattVoltage = atof(value);
      break;
   case CI_LOAD:
      Dmsg1(80, "Got CI_LOAD: %s\n", value);
      ups->UPSLoad = atof(value);
      break;
   case CI_FREQ:
      Dmsg1(80, "Got CI_FREQ: %s\n", value);
      ups->LineFreq = atof(value);
      break;
   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %s\n", value);
      ups->TimeLeft = atof(value);
      break;
   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %s\n", value);
      ups->UPSTemp = atof(value);
      break;
   case CI_DIPSW:
      Dmsg1(80, "Got CI_DIPSW: %s\n", value);
      ups->dipsw = strtoul(value, NULL, 16);
      break;
   case CI_REG1:
      Dmsg1(80, "Got CI_REG1: %s\n", value);
      ups->reg1 = strtoul(value, NULL, 16);
      break;
   case CI_REG2:
      ups->reg2 = strtoul(value, NULL, 16);
      ups->set_battpresent(!(ups->reg2 & 0x20));
      break;
   case CI_REG3:
      Dmsg1(80, "Got CI_REG3: %s\n", value);
      ups->reg3 = strtoul(value, NULL, 16);
      break;
   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %s\n", value);
      ups->humidity = atof(value);
      break;
   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %s\n", value);
      ups->ambtemp = atof(value);
      break;
   case CI_ST_TIME:
      Dmsg1(80, "Got CI_ST_TIME: %s\n", value);
      ups->LastSTTime = atof(value);
      break;
      
      /*
       * STATIC DATA
       */
   case CI_SENS:
      Dmsg1(80, "Got CI_SENS: %s\n", value);
      strncpy(ups->sensitivity, value, sizeof(ups->sensitivity));
      break;
   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %s\n", value);
      ups->dwake = (int)atof(value);
      break;
   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %s\n", value);
      ups->dshutd = (int)atof(value);
      break;
   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %s\n", value);
      ups->lotrans = (int)atof(value);
      break;
   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %s\n", value);
      ups->hitrans = (int)atof(value);
      break;
   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %s\n", value);
      ups->rtnpct = (int)atof(value);
      break;
   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %s\n", value);
      strncpy(ups->beepstate, value, sizeof(ups->beepstate));
      break;
   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %s\n", value);
      ups->dlowbatt = (int)atof(value);
      break;
   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", value);
      if (ups->upsname[0] == 0)
         strncpy(ups->upsname, value, sizeof(ups->upsname));
      break;
   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %s\n", value);
      strncpy(ups->selftest, value, sizeof(ups->selftest));
      break;
   case CI_MANDAT:
      Dmsg1(80, "Got CI_MANDAT: %s\n", value);
      strncpy(ups->birth, value, sizeof(ups->birth));
      break;
   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", value);
      strncpy(ups->serial, value, sizeof(ups->serial));
      break;
   case CI_BATTDAT:
      Dmsg1(80, "Got CI_BATTDAT: %s\n", value);
      strncpy(ups->battdat, value, sizeof(ups->battdat));
      break;
   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %s\n", value);
      ups->NomOutputVoltage = (int)atof(value);
      break;
   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %s\n", value);
      ups->nombattv = atof(value);
      break;
   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", value);
      strncpy(ups->firmrev, value, sizeof(ups->firmrev));
      break;
   case CI_EXTBATTS:
      Dmsg1(80, "Got CI_EXTBATTS: %s\n", value);
      ups->extbatts = (int)atof(value);
      break;
   case CI_BADBATTS:
      Dmsg1(80, "Got CI_BADBATTS: %s\n", value);
      ups->badbatts = (int)atof(value);
      break;
   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", value);
      strncpy(ups->upsmodel, value, sizeof(ups->upsmodel));
      break;
   case CI_EPROM:
      Dmsg1(80, "Got CI_EPROM: %s\n", value);
      strncpy(ups->eprom, value, sizeof(ups->eprom));
      break;
   default:
      Dmsg1(100, "Unknown CI (%d)\n", ci);
      break;
   }
}

/*
 * Read UPS events. I.e. state changes.
 */
int pcnet_ups_check_state(UPSINFO *ups)
{
   struct timeval tv, now, exit;
   fd_set rfds;
   bool done = false;
   char buf[4096];
   struct sockaddr_in from;
   socklen_t fromlen;
   char *key, *end, *value, *ptr;
   int retval;

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += ups->wait_time;

   while (!done) {

      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_usec >= exit.tv_usec)) {
         /* Done already? How time flies... */
         return 0;
      }

      tv.tv_sec = exit.tv_sec - now.tv_sec;
      tv.tv_usec = exit.tv_usec - now.tv_usec;
      if (tv.tv_usec < 0) {
         tv.tv_sec--;              /* Normalize */
         tv.tv_usec += 1000000;
      }

      FD_ZERO(&rfds);
      FD_SET(ups->fd, &rfds);

      retval = select(ups->fd + 1, &rfds, NULL, NULL, &tv);

      switch (retval) {
      case 0:                     /* No chars available in TIMER seconds. */
         return 0;
      case -1:
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;
         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
//         usb_link_check(ups);      /* link is down, wait */
         return 0;
      default:
         break;
      }

      do {
         fromlen = sizeof(from);
         retval = recvfrom(ups->fd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

      if (retval < 0) {            /* error */
         Dmsg1(200, "recvfrom error: ERR=%s\n", strerror(errno));
//         usb_link_check(ups);      /* notify that link is down, wait */
         return 0;
      }

      Dmsg4(200, "Packet from: %d.%d.%d.%d\n",
         (ntohl(from.sin_addr.s_addr) >> 24) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 16) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 8) & 0xff,
         ntohl(from.sin_addr.s_addr) & 0xff);

      /* Ensure the packet is nul-terminated */
      buf[retval] = '\0';

      if (debug_level >= 300) {
         logf("Interrupt data: ");
         for (int i = 0; i <= retval; i++)
            logf("%02x, ", buf[i]);
         logf("\n");
      }

      write_lock(ups);

      ptr = buf;
      while (*ptr) {
         /* Find the beginning of the line */
         while (isspace(*ptr))
            ptr++;
         key = ptr;

         /* Find the end of the line */
         while (*ptr && *ptr != '\r' && *ptr != '\n')
            ptr++;
         end = ptr;
         if (*ptr != '\0')
            ptr++;

         /* Remove trailing whitespace */
         do {
            *end-- = '\0';
         } while (end >= key && isspace(*end));

         Dmsg1(200, "line='%s'\n", key);

         /* Split the string */
         if ((value = strchr(key, '=')) == NULL)
            continue;
         *value++ = '\0';

         pcnet_process_data(ups, key, value);
      }

      write_unlock(ups);
   }

   return 1;
}

int pcnet_ups_open(UPSINFO *ups)
{
   struct sockaddr_in addr;
   
   write_lock(ups);

   ups->fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (ups->fd == -1)
      Error_abort1(_("Cannot create socket (%d)\n"), errno);

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(PCNET_PORT);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(ups->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(ups->fd);
      Error_abort1(_("Cannot bind socket (%d)\n"), errno);
   }

   /* Cheat and fixup CI_UPSMODEL to match PCNET */
   ups->UPS_Cmd[CI_UPSMODEL] = 0x01;

   /* Assume we have no connection until the first packet comes in */
   ups->set_commlost();

   write_unlock(ups);
   return 1;
}

int pcnet_ups_setup(UPSINFO *ups)
{
   /* Seems that there is nothing to do. */
   return 1;
}

int pcnet_ups_close(UPSINFO *ups)
{
   write_lock(ups);
   
   close(ups->fd);
   ups->fd = -1;

   write_unlock(ups);
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
int pcnet_ups_get_capabilities(UPSINFO *ups)
{
   /*
    * Unfortunately, we don't know capabilities until we
    * receive the first broadcast status message.
    */
   return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
int pcnet_ups_read_static_data(UPSINFO *ups)
{
   /* All our data gathering is done in pcnet_ups_check_state() */
   return 1;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every N seconds to get
 * a current idea of what the UPS is doing.
 */
int pcnet_ups_read_volatile_data(UPSINFO *ups)
{
   /* All our data gathering is done in pcnet_ups_check_state() */
   return 1;
}

int pcnet_ups_kill_power(UPSINFO *ups)
{
   /* Not implemented yet */
   return 0;
}

int pcnet_ups_program_eeprom(UPSINFO *ups, int command, char *data)
{
   /* Unsupported */
   return 0;
}

int pcnet_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   /* Unsupported */
   return 0;
}
