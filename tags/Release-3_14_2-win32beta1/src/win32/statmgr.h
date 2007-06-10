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
