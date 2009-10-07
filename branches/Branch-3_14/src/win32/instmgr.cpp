/*
 * Copyright (C) 2009 Adam Kropelin
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

#include "instmgr.h"
#include "wintray.h"
#include <stdio.h>

const char *InstanceManager::DEFAULT_HOST = "127.0.0.1";
const unsigned int InstanceManager::DEFAULT_PORT = 3551;
const unsigned int InstanceManager::DEFAULT_REFRESH = 5;
const char *InstanceManager::INSTANCES_KEY =
   "Software\\Apcupsd\\Apctray\\instances";

InstanceManager::InstanceManager(HINSTANCE appinst) :
   _appinst(appinst)
{
}

InstanceManager::~InstanceManager()
{
}

void InstanceManager::CreateMonitors()
{
   alist<InstanceConfig> unsorted;
   InstanceConfig config;

   // Open registry key apctray
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, INSTANCES_KEY, 0, KEY_READ, &apctray)
         == ERROR_SUCCESS)
   {
      // Iterate though all apctray instance keys, reading the config for each 
      // instance into a list.
      int i = 0;
      char name[128];
      DWORD len = sizeof(name);
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS)
      {
         unsorted.append(ReadConfig(apctray, name));
         len = sizeof(name);
      }
      RegCloseKey(apctray);

      // Now that we've read all instance configs, place them in a sorted list.
      // This is a really inefficient algorithm, but even O(N^2) is tolerable
      // for small N.
      alist<InstanceConfig>::iterator iter, lowest;
      while (!unsorted.empty())
      {
         lowest = unsorted.begin();
         for (iter = unsorted.begin(); iter != unsorted.end(); ++iter)
         {
            if ((*iter).order < (*lowest).order)
               lowest = iter;
         }

         _instances.append(*lowest);
         unsorted.remove(lowest);
      }
   }

   // If no instances were found, create a default one
   if (_instances.empty())
   {
      InstanceConfig config;
      config.host = DEFAULT_HOST;
      config.port = DEFAULT_PORT;
      config.refresh = DEFAULT_REFRESH;
      config.id = CreateId();
      config.order = 0;
      _instances.append(config);
      Write();
   }

   // Loop thru sorted instance list and create an upsMenu for each
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
   {
      (*iter).menu = new upsMenu(
         _appinst, (*iter).host, (*iter).port, (*iter).refresh, 
         &_balmgr, (*iter).id);
   }
}

InstanceConfig InstanceManager::ReadConfig(HKEY key, const char *id)
{
   InstanceConfig config;
   config.id = id;

   // Read instance config from registry
   HKEY subkey;
   if (RegOpenKeyEx(key, id, 0, KEY_READ, &subkey) == ERROR_SUCCESS)
   {
      config.host = RegQueryString(subkey, "host");
      config.port = RegQueryDWORD(subkey, "port");
      config.refresh = RegQueryDWORD(subkey, "refresh");
      config.order = RegQueryDWORD(subkey, "order");
      RegCloseKey(subkey);
   }

   // Apply defaults as necessary
   if (config.host.empty()) config.host = DEFAULT_HOST;
   if (config.port < 1)     config.port = DEFAULT_PORT;
   if (config.refresh < 1)  config.refresh = DEFAULT_REFRESH;

   return config;
}

void InstanceManager::Write()
{
   // Remove all existing instances from the registry
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, INSTANCES_KEY,
                    0, KEY_READ|KEY_WRITE, &apctray) == ERROR_SUCCESS)
   {
      // Iterate though all apctray keys locating instance keys of the form
      // "instanceN". Add the id of each key to a list. We don't want to
      // remove keys while we're enumerating them since Win32 docs say to not
      // change the key while enumerating it.
      int i = 0;
      char name[128];
      DWORD len = sizeof(name);
      alist<astring> ids;
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS)
      {
         if (len && strncasecmp(name, "instance", 8) == 0)
            ids.append(name);
         len = sizeof(name);
      }

      // Now that we have a list of all ids, loop thru and delete each one
      alist<astring>::iterator iter;
      for (iter = ids.begin(); iter != ids.end(); ++iter)
         RegDeleteKey(apctray, *iter);
   }
   else
   {
      // No apctray\instances key (and therefore no instances) yet
      if (RegCreateKey(HKEY_LOCAL_MACHINE, INSTANCES_KEY, &apctray) 
            != ERROR_SUCCESS)
         return;
   }

   // Write out instances. Our instance list is in sorted order but may
   // have gaps in the numbering, so regenerate 'order' value, ignoring what's
   // in the InstanceConfig.order field.
   HKEY instkey;
   int count = 0;
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
   {
      // No apctray key (and therefore no instances) yet. Create the key.
      if (RegCreateKey(apctray, (*iter).id, &instkey) == ERROR_SUCCESS)
      {
         RegSetString(instkey, "host", (*iter).host);
         RegSetDWORD(instkey, "port", (*iter).port);
         RegSetDWORD(instkey, "refresh", (*iter).refresh);
         RegSetDWORD(instkey, "order", count);
         RegCloseKey(instkey);
         count++;
      }
   }
   RegCloseKey(apctray);
}

alist<InstanceConfig>::iterator InstanceManager::FindInstance(const char *id)
{
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
   {
      if ((*iter).id == id)
         return iter;
   }
   return _instances.end();
}

void InstanceManager::RemoveInstance(const char *id)
{
   alist<InstanceConfig>::iterator inst = FindInstance(id);
   if (inst != _instances.end())
   {
      (*inst).menu->Destroy();
      delete (*inst).menu;
      _instances.remove(inst);
      Write();
   }
}

void InstanceManager::AddInstance()
{
   InstanceConfig config;
   config.host = DEFAULT_HOST;
   config.port = DEFAULT_PORT;
   config.refresh = DEFAULT_REFRESH;
   config.id = CreateId();
   config.order = 0;
   config.menu = new upsMenu(
      _appinst, config.host, config.port, config.refresh, 
      &_balmgr, config.id);
   _instances.append(config);
   Write();
}

void InstanceManager::UpdateInstance(
   const char *id, 
   const char *host, 
   int port, 
   int refresh)
{
   alist<InstanceConfig>::iterator inst = FindInstance(id);
   if (inst != _instances.end())
   {
      (*inst).host = host;
      (*inst).port = port;
      (*inst).refresh = refresh;
      Write();
   }
}

astring InstanceManager::CreateId()
{
   // Create binary UUID
   UUID uuid;
   UuidCreate(&uuid);

   // Convert binary UUID to RPC string
   unsigned char *tmpstr;
   UuidToString(&uuid, &tmpstr);   

   // Copy string UUID to astring and free RPC version
   astring uuidstr((char*)tmpstr);
   RpcStringFree(&tmpstr);

   return uuidstr;
}

DWORD InstanceManager::RegQueryDWORD(HKEY hkey, const char *name)
{
   DWORD result;
   DWORD len = sizeof(result);
   DWORD type;

   // Retrieve DWORD
   if (RegQueryValueEx(hkey, name, NULL, &type, (BYTE*)&result, &len)
         != ERROR_SUCCESS || type != REG_DWORD)
   {
      result = 0;
   }

   return result;
}

astring InstanceManager::RegQueryString(HKEY hkey, const char *name)
{
   char data[512]; // Arbitrary max
   DWORD len = sizeof(data);
   astring result;
   DWORD type;

   // Retrieve string
   if (RegQueryValueEx(hkey, name, NULL, &type, (BYTE*)data, &len)
         == ERROR_SUCCESS && type == REG_SZ)
   {
      result = data;
   }

   return result;
}

void InstanceManager::RegSetDWORD(HKEY hkey, const char *name, DWORD value)
{
   RegSetValueEx(hkey, name, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
}

void InstanceManager::RegSetString(HKEY hkey, const char *name, const char *value)
{
   RegSetValueEx(hkey, name, 0, REG_SZ, (BYTE*)value, strlen(value)+1);
}
