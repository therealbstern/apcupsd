
#include "statmgr.h"
#include "apc.h"
#include <stdarg.h>


StatMgr::StatMgr(char *host, unsigned short port)
   : m_host(host),
     m_port(port),
     m_socket(-1)
{
   memset(m_stats, 0, sizeof(m_stats));
   m_mutex = CreateMutex(NULL, false, NULL);
}

StatMgr::~StatMgr()
{
   close();
}

bool StatMgr::Update()
{
   lock();

   if (m_socket == -1 && !open()) {
      unlock();
      return false;
   }

   if (net_send(m_socket, "status", 6) != 6) {
      close();
      unlock();
      return false;
   }

   memset(m_stats, 0, sizeof(m_stats));

   int len;
   int i = 0;
   while (i < MAX_STATS &&
          (len = net_recv(m_socket, m_stats[i].data, sizeof(m_stats[i].data)-1)) > 0)
   {
      char *key, *value;

      // NUL-terminate the string
      m_stats[i].data[len] = '\0';

      // Find separator
      value = strchr(m_stats[i].data, ':');

      // Trim whitespace from value
      if (value) {
         *value++ = '\0';
         value = trim(value);
      }
 
      // Trim whitespace from key;
      key = trim(m_stats[i].data);

      m_stats[i].key = key;
      m_stats[i].value = value;
      i++;
   }

   if (len == -1) {
      close();
      unlock();
      return false;
   }

   unlock();
   return true;
}

char* StatMgr::Get(const char* key)
{
   char *ret = NULL;

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++) {
      if (strcmp(key, m_stats[idx].key) == 0) {
         if (m_stats[idx].value)
            ret = strdup(m_stats[idx].value);
         break;
      }
   }
   unlock();

   return ret;
}

char* StatMgr::GetAll()
{
   char *result = NULL;

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++) {
      result = sprintf_realloc_append(result, "%-9s: %s\n",
                  m_stats[idx].key, m_stats[idx].value);
   }
   unlock();

   return result;
}

char* StatMgr::GetEvents()
{
   lock();

   if (m_socket == -1 && !open()) {
      unlock();
      return NULL;
   }

   if (net_send(m_socket, "events", 6) != 6) {
      close();
      unlock();
      return NULL;
   }

   char *result = NULL;
   int len;
   char temp[1024];

   while ((len = net_recv(m_socket, temp, sizeof(temp)-1)) > 0)
   {
      temp[len] = '\0';
      rtrim(temp);
      result = sprintf_realloc_append(result, "%s\n", temp);
   }

   if (len == -1) {
      close();
      free(result);
      result = NULL;
   }

   unlock();
   return result;
}

char *StatMgr::ltrim(char *str)
{
   while(isspace(*str))
      *str++ = '\0';

   return str;
}

void StatMgr::rtrim(char *str)
{
   char *tmp = str + strlen(str) - 1;

   while (tmp >= str && isspace(*tmp))
      *tmp-- = '\0';
}

char *StatMgr::trim(char *str)
{
   str = ltrim(str);
   rtrim(str);
   return str;
}

char *StatMgr::sprintf_realloc_append(char *str, const char *format, ...)
{
   va_list args;
   char tmp[1024];

   va_start(args, format);
   int rc = avsnprintf(tmp, sizeof(tmp), format, args);
   va_end(args);

   // Win98 vsnprintf fails with -1 if buffer is not large enough.
   if (rc < 0)
      return str;

   int appendlen = strlen(tmp);
   int oldlen = str ? strlen(str) : 0;
   char *result = (char*)realloc(str, oldlen + appendlen + 1);

   memcpy(result + oldlen, tmp, appendlen+1);
   return result;
}

void StatMgr::lock()
{
   WaitForSingleObject(m_mutex, INFINITE);
}

void StatMgr::unlock()
{
   ReleaseMutex(m_mutex);
}

bool StatMgr::open()
{
   if (m_socket != -1)
      close();

   m_socket = net_open(m_host, NULL, m_port);
   return m_socket != -1;
}

void StatMgr::close()
{
   if (m_socket != -1) {
      net_close(m_socket);
      m_socket = -1;
   }
}
