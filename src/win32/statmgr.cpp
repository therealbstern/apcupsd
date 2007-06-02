
#include "statmgr.h"
#include "apc.h"


StatMgr::StatMgr(char *host, unsigned short port)
   : m_host(host),
     m_port(port)
{
   memset(m_stats, 0, sizeof(m_stats));
}

StatMgr::~StatMgr()
{
}

bool StatMgr::Update()
{
   int s = net_open(m_host, NULL, m_port);
   if (s == -1)
      return false;

   if (net_send(s, "status", 6) != 6) {
      net_close(s);
      return false;
   }

   lock();
   memset(m_stats, 0, sizeof(m_stats));

   int len;
   int i = 0;
   while (i < MAX_STATS &&
          (len = net_recv(s, m_stats[i].data, sizeof(m_stats[i].data)-1)) > 0)
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

   unlock();
   net_close(s);
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
      result = sprintf_realloc_append(result, "%-8s : %s\n",
                  m_stats[idx].key, m_stats[idx].value);
   }
   unlock();

   return result;
}

char* StatMgr::GetEvents()
{
   int s = net_open(m_host, NULL, m_port);
   if (s == -1)
      return NULL;

   if (net_send(s, "events", 6) != 6) {
      net_close(s);
      return NULL;
   }

   char *result = NULL;
   int len;
   char temp[1024];

   while ((len = net_recv(s, temp, sizeof(temp)-1)) > 0)
   {
      temp[len] = '\0';
      rtrim(temp);
      result = sprintf_realloc_append(result, "%s\n", temp);
   }

   net_close(s);
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
   va_start(args, format);
   
   int appendlen = vsnprintf(NULL, 0, format, args);
   int oldlen = str ? strlen(str) : 0;
   char *result = (char*)realloc(str, oldlen + appendlen + 1);
   vsprintf(result + oldlen, format, args);

   return result;
}
