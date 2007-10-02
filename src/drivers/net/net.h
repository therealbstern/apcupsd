/*
 * net.h
 *
 * Public header file for the net client driver.
 */

/*
 * Copyright (C) 2000-2006 Kern Sibbald
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

#ifndef _NET_H
#define _NET_H

#include "drivers.h"

#define BIGBUF 4096

class NetDriver: public UpsDriver
{
public:

   NetDriver(UPSINFO *ups) : UpsDriver(ups, "net") {}
   virtual ~NetDriver() {}

   // Subclasses must implement these methods
   virtual bool Open();
   virtual bool GetCapabilities();
   virtual bool ReadVolatileData();
   virtual bool ReadStaticData();
   virtual bool CheckState();
   virtual bool Close();

   // We provide default do-nothing implementations
   // for these methods since not all drivers need them.
   virtual bool EntryPoint(int command, void *data);

private:

   static SelfTestResult decode_testresult(char* str);
   static LastXferCause decode_lastxfer(char *str);

   bool initialize_device_data();
   bool getupsvar(char *request, char *answer, int anslen);
   bool poll_ups();
   bool fill_status_buffer();
   bool get_ups_status_flag(int fill);

   static const struct cmdtrans {
      const char *request;
      const char *upskeyword;
      int nfields;
   } _cmdtrans[];

   char _device[MAXSTRING];
   char *_hostname;
   int _port;
   int _sockfd;
   int _got_caps;
   int _got_static_data;
   time_t _last_fill_time;
   char _statbuf[BIGBUF];
   int _statlen;
   bool _comm_loss;
};

#endif   /* _NET_H */
