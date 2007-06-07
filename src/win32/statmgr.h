#ifndef STATMGR_H
#define STATMGR_H

#include "windows.h"

#define MAX_STATS 256
#define MAX_DATA  100


class StatMgr
{
public:

   StatMgr(char *host, unsigned short port);
   ~StatMgr();

   bool Update();
   char* Get(const char* key);
   char* GetAll();
   char* GetEvents();

private:

   bool open();
   void close();

   char *ltrim(char *str);
   void rtrim(char *str);
   char *trim(char *str);

   void lock();
   void unlock();

   char *sprintf_realloc_append(char *str, const char *format, ...);

   struct keyval {
      const char *key;
      const char *value;
      char data[MAX_DATA];
   };

   keyval          m_stats[MAX_STATS];
   char           *m_host;
   unsigned short  m_port;
   int             m_socket;
   HANDLE          m_mutex;
};

#endif // STATMGR_H
