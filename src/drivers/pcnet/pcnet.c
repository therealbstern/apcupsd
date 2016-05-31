/*
 * pcnet.c
 *
 * Driver for PowerChute Network Shutdown protocol.
 */

/*
 * Copyright (C) 2006 Adam Kropelin
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
#include "pcnet.h"
#include <sys/socket.h>
#include <netinet/in.h>

/* UPS broadcasts status packets to UDP port 3052 */
#define PCNET_DEFAULT_PORT 3052

/*
 * Number of seconds with no data before we declare COMMLOST.
 * UPS should report in every 25 seconds. We allow 2 missing
 * reports plus a fudge factor.
 */
#define COMMLOST_TIMEOUT   55

/* Win32 needs a special close for sockets */
#ifdef HAVE_MINGW
#define close(fd) closesocket(fd)
#endif

/* Convert UPS response to enum and string */
SelfTestResult PcnetUpsDriver::decode_testresult(const char* str)
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
LastXferCause PcnetUpsDriver::decode_lastxfer(const char *str)
{
   Dmsg(80, "Transfer reason: %c\n", *str);

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

PcnetUpsDriver::PcnetUpsDriver(UPSINFO *ups) :
   UpsDriver(ups),
   _ipaddr(NULL),
   _user(NULL),
   _pass(NULL),
   _auth(false),
   _uptime(0),
   _reboots(0),
   _datatime(0),
   _runtimeInSeconds(false),
   _fd(INVALID_SOCKET)
{
   memset(_device, 0, sizeof(_device));
}

bool PcnetUpsDriver::pcnet_process_data(const char *key, const char *value)
{
   unsigned long cmd;
   int ci;
   bool ret;
   double tmp;

   /* Make sure we have a value */
   if (*value == '\0')
      return false;

   /* Detect remote shutdown command */
   if (strcmp(key, "SD") == 0)
   {
      cmd = strtoul(value, NULL, 10);
      switch (cmd)
      {
      case 0:
         Dmsg(80, "SD: The UPS is NOT shutting down\n");
         _ups->clear_shut_remote();
         break;

      case 1:
         Dmsg(80, "SD: The UPS is shutting down\n");
         _ups->set_shut_remote();
         break;

      default:
         Dmsg(80, "Unrecognized SD value %s!\n", value);
         break;
      }
 
      return true;
   }

   /* Key must be 2 hex digits */
   if (!isxdigit(key[0]) || !isxdigit(key[1]))
      return false;

   /* Convert command to CI */
   cmd = strtoul(key, NULL, 16);
   for (ci=0; ci<CI_MAXCI; ci++)
      if (_ups->UPS_Cmd[ci] == cmd)
         break;

   /* No match? */
   if (ci == CI_MAXCI)
      return false;

   /* Mark this CI as available */
   _ups->UPS_Cap[ci] = true;

   /* Handle the data */
   ret = true;
   switch (ci) {
      /*
       * VOLATILE DATA
       */
   case CI_STATUS:
      Dmsg(80, "Got CI_STATUS: %s\n", value);
      _ups->Status &= ~0xFF;        /* clear APC byte */
      _ups->Status |= strtoul(value, NULL, 16) & 0xFF;  /* set APC byte */
      break;
   case CI_LQUAL:
      Dmsg(80, "Got CI_LQUAL: %s\n", value);
      strlcpy(_ups->linequal, value, sizeof(_ups->linequal));
      break;
   case CI_WHY_BATT:
      Dmsg(80, "Got CI_WHY_BATT: %s\n", value);
      _ups->lastxfer = decode_lastxfer(value);
      break;
   case CI_ST_STAT:
      Dmsg(80, "Got CI_ST_STAT: %s\n", value);
      _ups->testresult = decode_testresult(value);
      break;
   case CI_VLINE:
      Dmsg(80, "Got CI_VLINE: %s\n", value);
      _ups->LineVoltage = atof(value);
      break;
   case CI_VMIN:
      Dmsg(80, "Got CI_VMIN: %s\n", value);
      _ups->LineMin = atof(value);
      break;
   case CI_VMAX:
      Dmsg(80, "Got CI_VMAX: %s\n", value);
      _ups->LineMax = atof(value);
      break;
   case CI_VOUT:
      Dmsg(80, "Got CI_VOUT: %s\n", value);
      _ups->OutputVoltage = atof(value);
      break;
   case CI_BATTLEV:
      Dmsg(80, "Got CI_BATTLEV: %s\n", value);
      _ups->BattChg = atof(value);
      break;
   case CI_VBATT:
      Dmsg(80, "Got CI_VBATT: %s\n", value);
      _ups->BattVoltage = atof(value);
      break;
   case CI_LOAD:
      Dmsg(80, "Got CI_LOAD: %s\n", value);
      _ups->UPSLoad = atof(value);
      break;
   case CI_FREQ:
      Dmsg(80, "Got CI_FREQ: %s\n", value);
      _ups->LineFreq = atof(value);
      break;
   case CI_RUNTIM:
      Dmsg(80, "Got CI_RUNTIM: %s\n", value);
      tmp = atof(value);
      _ups->TimeLeft = _runtimeInSeconds ? tmp/60 : tmp;
      break;
   case CI_ITEMP:
      Dmsg(80, "Got CI_ITEMP: %s\n", value);
      _ups->UPSTemp = atof(value);
      break;
   case CI_DIPSW:
      Dmsg(80, "Got CI_DIPSW: %s\n", value);
      _ups->dipsw = strtoul(value, NULL, 16);
      break;
   case CI_REG1:
      Dmsg(80, "Got CI_REG1: %s\n", value);
      _ups->reg1 = strtoul(value, NULL, 16);
      break;
   case CI_REG2:
      Dmsg(80, "Got CI_REG2: %s\n", value);
      _ups->reg2 = strtoul(value, NULL, 16);
      _ups->set_battpresent(!(_ups->reg2 & 0x20));
      break;
   case CI_REG3:
      Dmsg(80, "Got CI_REG3: %s\n", value);
      _ups->reg3 = strtoul(value, NULL, 16);
      break;
   case CI_HUMID:
      Dmsg(80, "Got CI_HUMID: %s\n", value);
      _ups->humidity = atof(value);
      break;
   case CI_ATEMP:
      Dmsg(80, "Got CI_ATEMP: %s\n", value);
      _ups->ambtemp = atof(value);
      break;
   case CI_ST_TIME:
      Dmsg(80, "Got CI_ST_TIME: %s\n", value);
      _ups->LastSTTime = atof(value);
      break;
      
      /*
       * STATIC DATA
       */
   case CI_SENS:
      Dmsg(80, "Got CI_SENS: %s\n", value);
      strlcpy(_ups->sensitivity, value, sizeof(_ups->sensitivity));
      break;
   case CI_DWAKE:
      Dmsg(80, "Got CI_DWAKE: %s\n", value);
      _ups->dwake = (int)atof(value);
      break;
   case CI_DSHUTD:
      Dmsg(80, "Got CI_DSHUTD: %s\n", value);
      _ups->dshutd = (int)atof(value);
      break;
   case CI_LTRANS:
      Dmsg(80, "Got CI_LTRANS: %s\n", value);
      _ups->lotrans = (int)atof(value);
      break;
   case CI_HTRANS:
      Dmsg(80, "Got CI_HTRANS: %s\n", value);
      _ups->hitrans = (int)atof(value);
      break;
   case CI_RETPCT:
      Dmsg(80, "Got CI_RETPCT: %s\n", value);
      _ups->rtnpct = (int)atof(value);
      break;
   case CI_DALARM:
      Dmsg(80, "Got CI_DALARM: %s\n", value);
      strlcpy(_ups->beepstate, value, sizeof(_ups->beepstate));
      break;
   case CI_DLBATT:
      Dmsg(80, "Got CI_DLBATT: %s\n", value);
      _ups->dlowbatt = (int)atof(value);
      break;
   case CI_IDEN:
      Dmsg(80, "Got CI_IDEN: %s\n", value);
      if (_ups->upsname[0] == 0)
         strlcpy(_ups->upsname, value, sizeof(_ups->upsname));
      break;
   case CI_STESTI:
      Dmsg(80, "Got CI_STESTI: %s\n", value);
      strlcpy(_ups->selftest, value, sizeof(_ups->selftest));
      break;
   case CI_MANDAT:
      Dmsg(80, "Got CI_MANDAT: %s\n", value);
      strlcpy(_ups->birth, value, sizeof(_ups->birth));
      break;
   case CI_SERNO:
      Dmsg(80, "Got CI_SERNO: %s\n", value);
      strlcpy(_ups->serial, value, sizeof(_ups->serial));
      break;
   case CI_BATTDAT:
      Dmsg(80, "Got CI_BATTDAT: %s\n", value);
      strlcpy(_ups->battdat, value, sizeof(_ups->battdat));
      break;
   case CI_NOMOUTV:
      Dmsg(80, "Got CI_NOMOUTV: %s\n", value);
      _ups->NomOutputVoltage = (int)atof(value);
      break;
   case CI_NOMBATTV:
      Dmsg(80, "Got CI_NOMBATTV: %s\n", value);
      _ups->nombattv = atof(value);
      break;
   case CI_REVNO:
      Dmsg(80, "Got CI_REVNO: %s\n", value);
      strlcpy(_ups->firmrev, value, sizeof(_ups->firmrev));
      break;
   case CI_EXTBATTS:
      Dmsg(80, "Got CI_EXTBATTS: %s\n", value);
      _ups->extbatts = (int)atof(value);
      break;
   case CI_BADBATTS:
      Dmsg(80, "Got CI_BADBATTS: %s\n", value);
      _ups->badbatts = (int)atof(value);
      break;
   case CI_UPSMODEL:
      Dmsg(80, "Got CI_UPSMODEL: %s\n", value);
      strlcpy(_ups->upsmodel, value, sizeof(_ups->upsmodel));
      break;
   case CI_EPROM:
      Dmsg(80, "Got CI_EPROM: %s\n", value);
      strlcpy(_ups->eprom, value, sizeof(_ups->eprom));
      break;
   default:
      Dmsg(100, "Unknown CI (%d)\n", ci);
      ret = false;
      break;
   }
   
   return ret;
}

char *PcnetUpsDriver::digest2ascii(md5_byte_t *digest)
{
   static char ascii[33];
   char byte[3];
   int idx;

   /* Convert binary digest to ascii */
   ascii[0] = '\0';
   for (idx=0; idx<16; idx++) {
      snprintf(byte, sizeof(byte), "%02x", (unsigned char)digest[idx]);
      strlcat(ascii, byte, sizeof(ascii));
   }

   return ascii;
}

struct pair {
   const char* key;
   const char* value;
};

#define MAX_PAIRS 256

const char *PcnetUpsDriver::lookup_key(const char *key, struct pair table[])
{
   int idx;
   const char *ret = NULL;

   for (idx=0; table[idx].key; idx++) {
      if (strcmp(key, table[idx].key) == 0) {
         ret = table[idx].value;
         break;
      }
   }

   return ret;
}

struct pair *PcnetUpsDriver::auth_and_map_packet(char *buf, int len)
{
   char *key, *end, *ptr, *value;
   const char *val, *hash=NULL;
   static struct pair pairs[MAX_PAIRS+1];
   md5_state_t ms;
   md5_byte_t digest[16];
   unsigned int idx;
   unsigned long uptime, reboots;

   /* If there's no MD= field, drop the packet */
   if ((ptr = strstr(buf, "MD=")) == NULL || ptr == buf)
      return NULL;

   if (_auth) {
      /* Calculate the MD5 of the packet before messing with it */
      md5_init(&ms);
      md5_append(&ms, (md5_byte_t*)buf, ptr-buf);
      md5_append(&ms, (md5_byte_t*)_user, strlen(_user));
      md5_append(&ms, (md5_byte_t*)_pass, strlen(_pass));
      md5_finish(&ms, digest);

      /* Convert binary digest to ascii */
      hash = digest2ascii(digest);
   }

   /* Build a table of pointers to key/value pairs */
   memset(pairs, 0, sizeof(pairs));
   ptr = buf;
   idx = 0;
   while (*ptr && idx < MAX_PAIRS) {
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

      Dmsg(300, "process_packet: line='%s'\n", key);

      /* Split the string */
      if ((value = strchr(key, '=')) == NULL)
         continue;
      *value++ = '\0';

      Dmsg(300, "process_packet: key='%s' value='%s'\n",
         key, value);

      /* Save key/value in table */
      pairs[idx].key = key;
      pairs[idx].value = value;
      idx++;
   }

   if (_auth) {
      /* Check calculated hash vs received */
      Dmsg(200, "process_packet: calculated=%s\n", hash);
      val = lookup_key("MD", pairs);
      if (!val || strcmp(hash, val)) {
         Dmsg(200, "process_packet: message hash failed\n");
         return NULL;
      }
      Dmsg(200, "process_packet: message hash passed\n", val);

      /* Check management card IP address */
      val = lookup_key("PC", pairs);
      if (!val) {
         Dmsg(200, "process_packet: Missing PC field\n");
         return NULL;
      }
      Dmsg(200, "process_packet: Expected IP=%s\n", _ipaddr);
      Dmsg(200, "process_packet: Received IP=%s\n", val);
      if (strcmp(val, _ipaddr)) {
         Dmsg(200, "process_packet: IP address mismatch\n",
            _ipaddr, val);
         return NULL;
      }
   }

   /*
    * Check that uptime and/or reboots have advanced. If not,
    * this packet could be out of order, or an attacker may
    * be trying to replay an old packet.
    */
   val = lookup_key("SR", pairs);
   if (!val) {
      Dmsg(200, "process_packet: Missing SR field\n");
      return NULL;
   }
   reboots = strtoul(val, NULL, 16);

   val = lookup_key("SU", pairs);
   if (!val) {
      Dmsg(200, "process_packet: Missing SU field\n");
      return NULL;
   }
   uptime = strtoul(val, NULL, 16);

   Dmsg(200, "process_packet: Our reboots=%d\n", _reboots);
   Dmsg(200, "process_packet: UPS reboots=%d\n", reboots);
   Dmsg(200, "process_packet: Our uptime=%d\n", _uptime);
   Dmsg(200, "process_packet: UPS uptime=%d\n", uptime);

   if ((reboots == _reboots && uptime <= _uptime) ||
       (reboots < _reboots)) {
      Dmsg(200, "process_packet: Packet is out of order or replayed\n");
      return NULL;
   }

   _reboots = reboots;
   _uptime = uptime;
   return pairs;
}

int PcnetUpsDriver::wait_for_data(int wait_time)
{
   struct timeval tv, now, exit;
   fd_set rfds;
   bool done = false;
   struct sockaddr_in from;
   socklen_t fromlen;
   int retval;
   char buf[4096];
   struct pair *map;
   int idx;

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += wait_time;

   while (!done) {

      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_usec >= exit.tv_usec)) {
         /* Done already? How time flies... */
         break;
      }

      tv.tv_sec = exit.tv_sec - now.tv_sec;
      tv.tv_usec = exit.tv_usec - now.tv_usec;
      if (tv.tv_usec < 0) {
         tv.tv_sec--;              /* Normalize */
         tv.tv_usec += 1000000;
      }

      Dmsg(100, "Waiting for %d.%d\n", tv.tv_sec, tv.tv_usec);
      FD_ZERO(&rfds);
      FD_SET(_fd, &rfds);

      retval = select(_fd + 1, &rfds, NULL, NULL, &tv);

      if (retval == 0) {
         /* No chars available in TIMER seconds. */
         break;
      } else if (retval == -1) {
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;
         Dmsg(200, "select error: ERR=%s\n", strerror(errno));
         return 0;
      }

      do {
         fromlen = sizeof(from);
         retval = recvfrom(_fd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

      if (retval < 0) {            /* error */
         Dmsg(200, "recvfrom error: ERR=%s\n", strerror(errno));
//         usb_link_check(ups);      /* notify that link is down, wait */
         break;
      }

      Dmsg(200, "Packet from: %d.%d.%d.%d\n",
         (ntohl(from.sin_addr.s_addr) >> 24) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 16) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 8) & 0xff,
         ntohl(from.sin_addr.s_addr) & 0xff);

      /* Ensure the packet is nul-terminated */
      buf[retval] = '\0';

      hex_dump(300, buf, retval);

      map = auth_and_map_packet(buf, retval);
      if (map == NULL)
         continue;

      write_lock(_ups);

      for (idx=0; map[idx].key; idx++)
         done |= pcnet_process_data(map[idx].key, map[idx].value);

      write_unlock(_ups);
   }

   /* If we successfully received a data packet, update timer. */
   if (done) {
      time(&_datatime);
      Dmsg(100, "Valid data at time_t=%d\n", _datatime);
   }

   return done;
}

/*
 * Read UPS events. I.e. state changes.
 */
bool PcnetUpsDriver::check_state()
{
   return wait_for_data(_ups->wait_time);
}

bool PcnetUpsDriver::Open()
{
   struct sockaddr_in addr;
   char *ptr;

   write_lock(_ups);

   unsigned short port = PCNET_DEFAULT_PORT;
   if (_ups->device[0] != '\0') {
      _auth = true;

      strlcpy(_device, _ups->device, sizeof(_device));
      ptr = _device;

      _ipaddr = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';
      
      _user = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';

      _pass = ptr;
      if (*ptr == '\0')
         Error_abort("Malformed DEVICE [ip:user:pass]\n");

      // Last segment is optional port number
      ptr = strchr(ptr, ':');
      if (ptr)
      {
         *ptr++ = '\0';
         port = atoi(ptr);
         if (port == 0)
            port = PCNET_DEFAULT_PORT;
      }
   }

   _fd = socket_cloexec(PF_INET, SOCK_DGRAM, 0);
   if (_fd == INVALID_SOCKET)
      Error_abort("Cannot create socket (%d)\n", errno);

   // Although SO_BROADCAST is typically described as enabling broadcast
   // *transmission* (which is not what we want) on some systems it appears to
   // be needed for broadcast reception as well. We will attempt to set it
   // everywhere and not worry if it fails.
   int enable = 1;
   (void)setsockopt(_fd, SOL_SOCKET, SO_BROADCAST, 
      (const char*)&enable, sizeof(enable));

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(_fd);
      Error_abort("Cannot bind socket (%d)\n", errno);
   }

   /* Reset datatime to now */
   time(&_datatime);

   /*
    * Note, we set _ups->fd here so the "core" of apcupsd doesn't
    * think we are a slave, which is what happens when it is -1.
    * (ADK: Actually this only appears to be true for apctest as
    * apcupsd proper uses the UPS_slave flag.)
    * Internally, we use the fd in our own private space
    */
   _ups->fd = 1;

   write_unlock(_ups);
   return 1;
}

bool PcnetUpsDriver::Close()
{
   write_lock(_ups);
   
   close(_fd);
   _fd = INVALID_SOCKET;

   write_unlock(_ups);
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
bool PcnetUpsDriver::get_capabilities()
{
   /*
    * Unfortunately, we don't know capabilities until we
    * receive the first broadcast status message.
    */

   int rc = wait_for_data(COMMLOST_TIMEOUT);
   if (rc)
   {
// Disable workaround ... recent Smart-UPS RT 5000 XL don't have this issue
// and workaround is causing bad readings
#if 0
      /*
       * Check for quirk where UPS reports runtime remaining in seconds
       * instead of the usual minutes. So far this has been reported on
       * "Smart-UPS RT 5000 XL" and "Smart-UPS X 3000". We will assume it
       * affects all RT and X series models.
       */
      if (_ups->UPS_Cap[CI_UPSMODEL] && 
          (!strncmp(_ups->upsmodel, "Smart-UPS X", 11) ||
           !strncmp(_ups->upsmodel, "Smart-UPS RT", 12)))
      {
         Dmsg(50, "Enabling runtime-in-seconds quirk [%s]\n", _ups->upsmodel);
         _runtimeInSeconds = true;
         if (_ups->UPS_Cap[CI_RUNTIM])
            _ups->TimeLeft /= 60; // Adjust initial value
      }
#endif
   }

   return _ups->UPS_Cap[CI_STATUS];
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
bool PcnetUpsDriver::read_static_data()
{
   /*
    * First set of data was gathered already in pcnet_ups_get_capabilities().
    * All additional data gathering is done in pcnet_ups_check_state()
    */
   return 1;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every N seconds to get
 * a current idea of what the UPS is doing.
 */
bool PcnetUpsDriver::read_volatile_data()
{
   time_t now, diff;
   
   /*
    * All our data gathering is done in pcnet_ups_check_state().
    * But we do use this function to check our commlost state.
    */

   time(&now);
   diff = now - _datatime;

   if (_ups->is_commlost()) {
      if (diff < COMMLOST_TIMEOUT) {
         generate_event(_ups, CMDCOMMOK);
         _ups->clear_commlost();
      }
   } else {
      if (diff >= COMMLOST_TIMEOUT) {
         generate_event(_ups, CMDCOMMFAILURE);
         _ups->set_commlost();
      }
   }

   return 1;
}

bool PcnetUpsDriver::kill_power()
{
   struct sockaddr_in addr;
   char data[1024];
   sock_t s;
   int len=0, temp=0;
   char *start;
   const char *cs, *hash;
   struct pair *map;
   md5_state_t ms;
   md5_byte_t digest[16];

   /* We cannot perform a killpower without authentication data */
   if (!_auth) {
      Error_abort("Cannot perform killpower without authentication "
                   "data. Please set ip:user:pass for DEVICE in "
                   "apcupsd.conf.\n");
   }

   /* Open a TCP stream to the UPS */
   s = socket_cloexec(PF_INET, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET) {
      Dmsg(100, "pcnet_ups_kill_power: Unable to open socket: %s\n",
         strerror(errno));
      return 0;
   }

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(80);
   inet_pton(AF_INET, _ipaddr, &addr.sin_addr.s_addr);

   if (connect(s, (sockaddr*)&addr, sizeof(addr))) {
      Dmsg(100, "pcnet_ups_kill_power: Unable to connect to %s:%d: %s\n",
         _ipaddr, 80, strerror(errno));
      close(s);
      return 0;
   }

   /* Send a simple HTTP request for "/macontrol.htm". */
   asnprintf(data, sizeof(data),
      "GET /macontrol.htm HTTP/1.1\r\n"
      "Host: %s\r\n"
      "\r\n",
      _ipaddr);

   Dmsg(200, "Request:\n---\n%s---\n", data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /*
    * Read data until we find the 0-length chunk. 
    * We know that AP9617 uses chunked encoding, so we
    * can count on the 0-length chunk at the end.
    */
   do {
      len += temp;
      temp = recv(s, data+len, sizeof(data)-len-1, 0);
      if (temp >= 0)
         data[len+temp] = '\0';
   } while(temp > 0 && strstr(data, "\r\n0\r\n") == NULL);

   Dmsg(200, "Response:\n---\n%s---\n", data);

   if (temp < 0) {
      Dmsg(100, "pcnet_ups_kill_power: recv failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /*
    * Find "<html>" since that's where the real authenticated
    * data begins. Everything before that is headers. 
    */
   start = strstr(data, "<html>");
   if (start == NULL) {
      Dmsg(100, "pcnet_ups_kill_power: Malformed data\n");
      close(s);
      return 0;
   }

   /*
    * Authenticate and map the packet contents. This will
    * extract all key/value pairs and ensure the packet 
    * authentication hash is valid.
    */
   map = auth_and_map_packet(start, strlen(start));
   if (map == NULL) {
      close(s);
      return 0;
   }

   /* Check that we got a challenge string. */
   cs = lookup_key("CS", map);
   if (cs == NULL) {
      Dmsg(200, "pcnet_ups_kill_power: Missing CS field\n");
      close(s);
      return 0;
   }

   /*
    * Now construct the hash of the packet we're about to
    * send using the challenge string from the packet we
    * just received, plus our username and passphrase.
    */
   md5_init(&ms);
   md5_append(&ms, (md5_byte_t*)"macontrol1_control_shutdown_1=1,", 32);
   md5_append(&ms, (md5_byte_t*)cs, strlen(cs));
   md5_append(&ms, (md5_byte_t*)_user, strlen(_user));
   md5_append(&ms, (md5_byte_t*)_pass, strlen(_pass));
   md5_finish(&ms, digest);
   hash = digest2ascii(digest);

   /* Send the shutdown request */
   asnprintf(data, sizeof(data),
      "POST /Forms/macontrol1 HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: 72\r\n"
      "\r\n"
      "macontrol1%%5fcontrol%%5fshutdown%%5f1=1%%2C%s",
      _ipaddr, hash);

   Dmsg(200, "Request: (strlen=%d)\n---\n%s---\n", strlen(data), data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /* That's it, we're done. */
   close(s);

   return 1;
}

bool PcnetUpsDriver::entry_point(int command, void *data)
{
   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg(80, "Checking self test.\n");
      if (_ups->UPS_Cap[CI_WHY_BATT] && _ups->lastxfer == XFER_SELFTEST) {
         /*
          * set Self Test start time
          */
         _ups->SelfTest = time(NULL);
         Dmsg(80, "Self Test time: %s", ctime(&_ups->SelfTest));
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      /*
       * This is a bit kludgy. The selftest result isn't available from
       * the UPS for about 10 seconds after the selftest completes. So we
       * invoke pcnet_ups_check_state() with a 12 second timeout, 
       * expecting that it should get a status report before then.
       */
      
      /* Let check_status wait for the result */
      write_unlock(_ups);
      wait_for_data(12);
      write_lock(_ups);
      break;

   default:
      return FAILURE;
   }

   return SUCCESS;
}
