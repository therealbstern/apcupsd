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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "pcnet.h"
#include <sys/socket.h>
#include <netinet/in.h>

/* UPS broadcasts status packets to UDP port 3052 */
#define PCNET_PORT   3052

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

/* Constructor */
PcnetDriver::PcnetDriver(UPSINFO *ups)
   : UpsDriver(ups, "pcnet")
{
   _cmdmap = GetSmartCmdMap();

   // Fixup CI_UPSMODEL to match PCNET
   // Other commands map directly.
   _cmdmap[CI_UPSMODEL] = 0x01;
}

/* Destructor */
PcnetDriver::~PcnetDriver()
{
   delete [] _cmdmap;
}

/* Convert UPS response to enum and string */
SelfTestResult PcnetDriver::decode_testresult(const char* str)
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
LastXferCause PcnetDriver::decode_lastxfer(const char *str)
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

bool PcnetDriver::process_data(const char *key, const char *value)
{
   unsigned long cmd;
   int ci;
   bool ret;

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
         Dmsg0(80, "SD: The UPS is NOT shutting down\n");
         _ups->clear_shut_remote();
         break;

      case 1:
         Dmsg0(80, "SD: The UPS is shutting down\n");
         _ups->set_shut_remote();
         break;

      default:
         Dmsg1(80, "Unrecognized SD value %s!\n", value);
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
      if (_cmdmap[ci] == (int)cmd)
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
      Dmsg1(80, "Got CI_STATUS: %s\n", value);
      _ups->Status &= ~0xFF;        /* clear APC byte */
      _ups->Status |= strtoul(value, NULL, 16) & 0xFF;  /* set APC byte */
      break;
   case CI_LQUAL:
      Dmsg1(80, "Got CI_LQUAL: %s\n", value);
      strncpy(_ups->linequal, value, sizeof(_ups->linequal));
      break;
   case CI_WHY_BATT:
      Dmsg1(80, "Got CI_WHY_BATT: %s\n", value);
      _ups->lastxfer = decode_lastxfer(value);
      break;
   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %s\n", value);
      _ups->testresult = decode_testresult(value);
      break;
   case CI_VLINE:
      Dmsg1(80, "Got CI_VLINE: %s\n", value);
      _ups->LineVoltage = atof(value);
      break;
   case CI_VMIN:
      Dmsg1(80, "Got CI_VMIN: %s\n", value);
      _ups->LineMin = atof(value);
      break;
   case CI_VMAX:
      Dmsg1(80, "Got CI_VMAX: %s\n", value);
      _ups->LineMax = atof(value);
      break;
   case CI_VOUT:
      Dmsg1(80, "Got CI_VOUT: %s\n", value);
      _ups->OutputVoltage = atof(value);
      break;
   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %s\n", value);
      _ups->BattChg = atof(value);
      break;
   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %s\n", value);
      _ups->BattVoltage = atof(value);
      break;
   case CI_LOAD:
      Dmsg1(80, "Got CI_LOAD: %s\n", value);
      _ups->UPSLoad = atof(value);
      break;
   case CI_FREQ:
      Dmsg1(80, "Got CI_FREQ: %s\n", value);
      _ups->LineFreq = atof(value);
      break;
   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %s\n", value);
      _ups->TimeLeft = atof(value);
      break;
   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %s\n", value);
      _ups->UPSTemp = atof(value);
      break;
   case CI_DIPSW:
      Dmsg1(80, "Got CI_DIPSW: %s\n", value);
      _ups->dipsw = strtoul(value, NULL, 16);
      break;
   case CI_REG1:
      Dmsg1(80, "Got CI_REG1: %s\n", value);
      _ups->reg1 = strtoul(value, NULL, 16);
      break;
   case CI_REG2:
      _ups->reg2 = strtoul(value, NULL, 16);
      _ups->set_battpresent(!(_ups->reg2 & 0x20));
      break;
   case CI_REG3:
      Dmsg1(80, "Got CI_REG3: %s\n", value);
      _ups->reg3 = strtoul(value, NULL, 16);
      break;
   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %s\n", value);
      _ups->humidity = atof(value);
      break;
   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %s\n", value);
      _ups->ambtemp = atof(value);
      break;
   case CI_ST_TIME:
      Dmsg1(80, "Got CI_ST_TIME: %s\n", value);
      _ups->LastSTTime = atof(value);
      break;
      
      /*
       * STATIC DATA
       */
   case CI_SENS:
      Dmsg1(80, "Got CI_SENS: %s\n", value);
      strncpy(_ups->sensitivity, value, sizeof(_ups->sensitivity));
      break;
   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %s\n", value);
      _ups->dwake = (int)atof(value);
      break;
   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %s\n", value);
      _ups->dshutd = (int)atof(value);
      break;
   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %s\n", value);
      _ups->lotrans = (int)atof(value);
      break;
   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %s\n", value);
      _ups->hitrans = (int)atof(value);
      break;
   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %s\n", value);
      _ups->rtnpct = (int)atof(value);
      break;
   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %s\n", value);
      strncpy(_ups->beepstate, value, sizeof(_ups->beepstate));
      break;
   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %s\n", value);
      _ups->dlowbatt = (int)atof(value);
      break;
   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", value);
      if (_ups->upsname[0] == 0)
         strncpy(_ups->upsname, value, sizeof(_ups->upsname));
      break;
   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %s\n", value);
      strncpy(_ups->selftest, value, sizeof(_ups->selftest));
      break;
   case CI_MANDAT:
      Dmsg1(80, "Got CI_MANDAT: %s\n", value);
      strncpy(_ups->birth, value, sizeof(_ups->birth));
      break;
   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", value);
      strncpy(_ups->serial, value, sizeof(_ups->serial));
      break;
   case CI_BATTDAT:
      Dmsg1(80, "Got CI_BATTDAT: %s\n", value);
      strncpy(_ups->battdat, value, sizeof(_ups->battdat));
      break;
   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %s\n", value);
      _ups->NomOutputVoltage = (int)atof(value);
      break;
   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %s\n", value);
      _ups->nombattv = atof(value);
      break;
   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", value);
      strncpy(_ups->firmrev, value, sizeof(_ups->firmrev));
      break;
   case CI_EXTBATTS:
      Dmsg1(80, "Got CI_EXTBATTS: %s\n", value);
      _ups->extbatts = (int)atof(value);
      break;
   case CI_BADBATTS:
      Dmsg1(80, "Got CI_BADBATTS: %s\n", value);
      _ups->badbatts = (int)atof(value);
      break;
   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", value);
      strncpy(_ups->upsmodel, value, sizeof(_ups->upsmodel));
      break;
   case CI_EPROM:
      Dmsg1(80, "Got CI_EPROM: %s\n", value);
      strncpy(_ups->eprom, value, sizeof(_ups->eprom));
      break;
   default:
      Dmsg1(100, "Unknown CI (%d)\n", ci);
      ret = false;
      break;
   }
   
   return ret;
}

char *PcnetDriver::digest2ascii(md5_byte_t *digest)
{
   static char ascii[33];
   char *ptr;
   int idx;

   /* Convert binary digest to ascii */
   ptr = ascii;
   for (idx=0; idx<16; idx++) {
      sprintf(ptr, "%02x", (unsigned char)digest[idx]);
      ptr += 2;
   }

   return ascii;
}

const char *PcnetDriver::lookup_key(const char *key, amap<astring, astring> *map)
{
   const char *ret = NULL;
   amap<astring, astring>::iterator iter;
   iter = map->find(key);
   if (iter != map->end())
      ret = iter.value();

   return ret;
}

amap<astring, astring> *PcnetDriver::auth_and_map_packet(char *buf, int len)
{
   char *key, *end, *ptr, *value;
   const char *val, *hash=NULL;
   amap<astring, astring> *map;
   md5_state_t ms;
   md5_byte_t digest[16];
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
   map = new amap<astring, astring>();
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

      Dmsg1(300, "process_packet: line='%s'\n", key);

      /* Split the string */
      if ((value = strchr(key, '=')) == NULL)
         continue;
      *value++ = '\0';

      Dmsg2(300, "process_packet: key='%s' value='%s'\n",
         key, value);

      /* Save key/value in map */
      (*map)[key] = value;
   }

   if (_auth) {
      /* Check calculated hash vs received */
      Dmsg1(200, "process_packet: calculated=%s\n", hash);
      val = lookup_key("MD", map);
      if (!val || strcmp(hash, val)) {
         Dmsg0(200, "process_packet: message hash failed\n");
         delete map;
         return NULL;
      }
      Dmsg1(200, "process_packet: message hash passed\n", val);

      /* Check management card IP address */
      val = lookup_key("PC", map);
      if (!val) {
         Dmsg0(200, "process_packet: Missing PC field\n");
         delete map;
         return NULL;
      }
      Dmsg1(200, "process_packet: Expected IP=%s\n", _ipaddr);
      Dmsg1(200, "process_packet: Received IP=%s\n", val);
      if (strcmp(val, _ipaddr)) {
         Dmsg2(200, "process_packet: IP address mismatch\n",
            _ipaddr, val);
         delete map;
         return NULL;
      }
   }

   /*
    * Check that uptime and/or reboots have advanced. If not,
    * this packet could be out of order, or an attacker may
    * be trying to replay an old packet.
    */
   val = lookup_key("SR", map);
   if (!val) {
      Dmsg0(200, "process_packet: Missing SR field\n");
      delete map;
      return NULL;
   }
   reboots = strtoul(val, NULL, 16);

   val = lookup_key("SU", map);
   if (!val) {
      Dmsg0(200, "process_packet: Missing SU field\n");
      delete map;
      return NULL;
   }
   uptime = strtoul(val, NULL, 16);

   Dmsg1(200, "process_packet: Our reboots=%d\n", _reboots);
   Dmsg1(200, "process_packet: UPS reboots=%d\n", reboots);
   Dmsg1(200, "process_packet: Our uptime=%d\n", _uptime);
   Dmsg1(200, "process_packet: UPS uptime=%d\n", uptime);

   if ((reboots == _reboots && uptime <= _uptime) ||
       (reboots < _reboots)) {
      Dmsg0(200, "process_packet: Packet is out of order or replayed\n");
      delete map;
      return NULL;
   }

   _reboots = reboots;
   _uptime = uptime;
   return map;
}

/*
 * Read UPS events. I.e. state changes.
 */
bool PcnetDriver::CheckState()
{
   struct timeval tv, now, exit;
   fd_set rfds;
   bool done = false;
   struct sockaddr_in from;
   socklen_t fromlen;
   int retval;
   char buf[4096];
   amap<astring, astring> *map;

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += _ups->wait_time;

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

      Dmsg2(100, "Waiting for %d.%d\n", tv.tv_sec, tv.tv_usec);
      FD_ZERO(&rfds);
      FD_SET(_ups->fd, &rfds);

      retval = select(_ups->fd + 1, &rfds, NULL, NULL, &tv);

      if (retval == 0) {
         /* No chars available in TIMER seconds. */
         break;
      } else if (retval == -1) {
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;
         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         return 0;
      }

      do {
         fromlen = sizeof(from);
         retval = recvfrom(_ups->fd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

      if (retval < 0) {            /* error */
         Dmsg1(200, "recvfrom error: ERR=%s\n", strerror(errno));
//         usb_link_check(ups);      /* notify that link is down, wait */
         break;
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

      map = auth_and_map_packet(buf, retval);
      if (map == NULL)
         continue;

      write_lock(_ups);

      amap<astring, astring>::iterator iter;
      for (iter = map->begin(); iter != map->end(); ++iter)
         done |= process_data(iter.key(), iter.value());

      write_unlock(_ups);

      delete map;
   }

   /* If we successfully received a data packet, update timer. */
   if (done) {
      time(&_datatime);
      Dmsg1(100, "Valid data at time_t=%d\n", _datatime);
   }

   return done;
}

bool PcnetDriver::Open()
{
   struct sockaddr_in addr;
   char *ptr;

   write_lock(_ups);

   _device[0] = '\0';
   _ipaddr = _user = _pass = NULL;
   _auth = false;
   _uptime = _reboots = 0;

   if (_ups->device[0] != '\0') {
      _auth = true;

      astrncpy(_device, _ups->device, sizeof(_device));
      ptr = _device;

      _ipaddr = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';
      
      _user = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';

      _pass = ptr;
      if (*ptr == '\0')
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");
   }

   _ups->fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (_ups->fd == -1)
      Error_abort1(_("Cannot create socket (%d)\n"), errno);

   int enable = 1;
   setsockopt(_ups->fd, SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable));

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(PCNET_PORT);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(_ups->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(_ups->fd);
      Error_abort1(_("Cannot bind socket (%d)\n"), errno);
   }

   /* Reset datatime to now */
   time(&_datatime);

   write_unlock(_ups);
   return true;
}

bool PcnetDriver::Close()
{
   write_lock(_ups);

   close(_ups->fd);
   _ups->fd = -1;

   write_unlock(_ups);
   return true;
}

/*
 * Setup capabilities structure for UPS
 */
bool PcnetDriver::GetCapabilities()
{
   /*
    * Unfortunately, we don't know capabilities until we
    * receive the first broadcast status message.
    */
   return true;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
bool PcnetDriver::ReadStaticData()
{
   /* All our data gathering is done in pcnet_ups_check_state() */
   return true;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every N seconds to get
 * a current idea of what the UPS is doing.
 */
bool PcnetDriver::ReadVolatileData()
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

   return true;
}

bool PcnetDriver::KillPower()
{
   struct sockaddr_in addr;
   char data[1024];
   int s, len=0, temp=0;
   char *start;
   const char *cs, *hash;
   amap<astring, astring> *map;
   md5_state_t ms;
   md5_byte_t digest[16];

   /* We cannot perform a killpower without authentication data */
   if (!_auth) {
      Error_abort0("Cannot perform killpower without authentication "
                   "data. Please set ip:user:pass for DEVICE in "
                   "apcupsd.conf.\n");
   }

   /* Open a TCP stream to the UPS */
   s = socket(PF_INET, SOCK_STREAM, 0);
   if (s == -1) {
      Dmsg1(100, "pcnet_ups_kill_power: Unable to open socket: %s\n",
         strerror(errno));
      return false;
   }

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(80);
   inet_pton(AF_INET, _ipaddr, &addr.sin_addr.s_addr);

   if (connect(s, (sockaddr*)&addr, sizeof(addr))) {
      Dmsg3(100, "pcnet_ups_kill_power: Unable to connect to %s:%d: %s\n",
         _ipaddr, 80, strerror(errno));
      close(s);
      return false;
   }

   /* Send a simple HTTP request for "/macontrol.htm". */
   asnprintf(data, sizeof(data),
      "GET /macontrol.htm HTTP/1.1\r\n"
      "Host: %s\r\n"
      "\r\n",
      _ipaddr);

   Dmsg1(200, "Request:\n---\n%s---\n", data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg1(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return false;
   }

   /*
    * Clear buffer and read data until we find the 0-length
    * chunk. We know that AP9617 uses chunked encoding, so we
    * can count on the 0-length chunk at the end.
    */
   memset(data, 0, sizeof(data));
   do {
      len += temp;
      temp = recv(s, data+len, sizeof(data)-len, 0);
   } while(temp > 0 && strstr(data, "\r\n0\r\n") == NULL);

   Dmsg1(200, "Response:\n---\n%s---\n", data);

   if (temp < 0) {
      Dmsg1(100, "pcnet_ups_kill_power: recv failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /*
    * Find "<html>" since that's where the real authenticated
    * data begins. Everything before that is headers. 
    */
   start = strstr(data, "<html>");
   if (start == NULL) {
      Dmsg0(100, "pcnet_ups_kill_power: Malformed data\n");
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
      return false;
   }

   /* Check that we got a challenge string. */
   cs = lookup_key("CS", map);
   if (cs == NULL) {
      Dmsg0(200, "pcnet_ups_kill_power: Missing CS field\n");
      close(s);
      delete map;
      return false;
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

   /* No longer need the map */
   delete map;

   /* Send the shutdown request */
   asnprintf(data, sizeof(data),
      "POST /Forms/macontrol1 HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: 72\r\n"
      "\r\n"
      "macontrol1%%5fcontrol%%5fshutdown%%5f1=1%%2C%s",
      _ipaddr, hash);

   Dmsg2(200, "Request: (strlen=%d)\n---\n%s---\n", strlen(data), data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg1(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return false;
   }

   /* That's it, we're done. */
   close(s);

   return true;
}

bool PcnetDriver::EntryPoint(int command, void *data)
{
   int temp;

   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg0(80, "Checking self test.\n");
      if (_ups->UPS_Cap[CI_WHY_BATT] && _ups->lastxfer == XFER_SELFTEST) {
         /*
          * set Self Test start time
          */
         _ups->SelfTest = time(NULL);
         Dmsg1(80, "Self Test time: %s", ctime(&_ups->SelfTest));
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      /*
       * This is a bit kludgy. The selftest result isn't available from
       * the UPS for about 10 seconds after the selftest completes. So we
       * invoke pcnet_ups_check_state() with a 12 second timeout, 
       * expecting that it should get a status report before then.
       */

      /* Save current _ups->wait_time and set it to 12 seconds */
      temp = _ups->wait_time;
      _ups->wait_time = 12;

      /* Let check_status wait for the result */
      write_unlock(_ups);
      CheckState();
      write_lock(_ups);

      /* Restore _ups->wait_time */
      _ups->wait_time = temp;
      break;

   default:
      return false;
   }

   return true;
}
