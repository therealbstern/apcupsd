/*
 * pcnet.h
 *
 * Public header file for the pcnet driver.
 */

#ifndef _PCNET_H
#define _PCNET_H

#include "md5.h"

class PcnetUpsDriver: public UpsDriver
{
public:
   PcnetUpsDriver(UPSINFO *ups);
   virtual ~PcnetUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new PcnetUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();
   virtual bool entry_point(int command, void *data);

private:

   bool pcnet_process_data(const char *key, const char *value);
   struct pair *auth_and_map_packet(char *buf, int len);
   int wait_for_data(int wait_time);

   static SelfTestResult decode_testresult(const char* str);
   static LastXferCause decode_lastxfer(const char *str);
   static char *digest2ascii(md5_byte_t *digest);
   static const char *lookup_key(const char *key, struct pair table[]);

   char _device[MAXSTRING];             /* Copy of ups->device */
   char *_ipaddr;                       /* IP address of UPS */
   char *_user;                         /* Username */
   char *_pass;                         /* Pass phrase */
   bool _auth;                          /* Authenticate? */
   unsigned long _uptime;               /* UPS uptime counter */
   unsigned long _reboots;              /* UPS reboot counter */
   time_t _datatime;                    /* Last time we got valid data */
   bool _runtimeInSeconds;              /* UPS reports runtime in seconds */
   sock_t _fd;                          /* Socket connection */
};

#endif   /* _PCNET_H */
