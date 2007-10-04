/*
 * pcnet.h
 *
 * Public header file for the pcnet driver.
 */

#ifndef _PCNET_H
#define _PCNET_H

#include "drivers.h"
#include "md5.h"

class PcnetDriver: public UpsDriver
{
public:

   PcnetDriver(UPSINFO *ups);
   virtual ~PcnetDriver();

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState();
   virtual bool Close();

   // We provide default do-nothing implementations
   // for these methods since not all drivers need them.
   virtual bool KillPower();
   virtual bool EntryPoint(int command, void *data);

private:

   struct pair {
      const char* key;
      const char* value;
   };

   static SelfTestResult decode_testresult(const char* str);
   static LastXferCause decode_lastxfer(const char *str);
   static char *digest2ascii(md5_byte_t *digest);
   static const char *lookup_key(const char *key, struct pair table[]);

   bool process_data(const char *key, const char *value);
   pair *auth_and_map_packet(char *buf, int len);

   char _device[MAXSTRING];            /* Copy of ups->device */
   char *_ipaddr;                      /* IP address of UPS */
   char *_user;                        /* Username */
   char *_pass;                        /* Pass phrase */
   bool _auth;                         /* Authenticate? */
   unsigned long _uptime;              /* UPS uptime counter */
   unsigned long _reboots;             /* UPS reboot counter */
   time_t _datatime;                   /* Last time we got valid data */
   int *_cmdmap;                       /* Map of CI to command code */
};

#endif   /* _PCNET_H */
