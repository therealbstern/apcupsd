/*
 * Copyright (C) 2007 Adam Kropelin
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
#include "statmgr.h"
#include <stdarg.h>

StatMgr::StatMgr(const char *host, unsigned short port)
   : m_host(host),
     m_port(port),
     m_socket(-1)
{
   memset(m_stats, 0, sizeof(m_stats));
   m_mutex = CreateMutex(NULL, false, NULL);
}

StatMgr::~StatMgr()
{
   lock();
   close();
   CloseHandle(m_mutex);
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

std::string StatMgr::Get(const char* key)
{
   std::string ret;

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++) {
      if (strcmp(key, m_stats[idx].key) == 0) {
         if (m_stats[idx].value)
            ret = m_stats[idx].value;
         break;
      }
   }
   unlock();

   return ret;
}

bool StatMgr::GetAll(std::vector<std::string> &status)
{
   status.clear();

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++) {
      char buffer[1024];
      asnprintf(buffer, sizeof(buffer), "%-9s: %s",
                m_stats[idx].key, m_stats[idx].value);
      status.push_back(buffer);
   }
   unlock();

   return true;
}

bool StatMgr::GetEvents(std::vector<std::string> &events)
{
   lock();

   if (m_socket == -1 && !open()) {
      unlock();
      return false;
   }

   if (net_send(m_socket, "events", 6) != 6) {
      close();
      unlock();
      return false;
   }

   events.clear();

   char temp[1024];
   int len;
   while ((len = net_recv(m_socket, temp, sizeof(temp)-1)) > 0)
   {
      temp[len] = '\0';
      rtrim(temp);
      events.push_back(temp);
   }

   if (len == -1)
      close();

   unlock();
   return true;
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

   m_socket = net_open((char*)m_host, NULL, m_port);
   return m_socket != -1;
}

void StatMgr::close()
{
   if (m_socket != -1) {
      net_close(m_socket);
      m_socket = -1;
   }
}
