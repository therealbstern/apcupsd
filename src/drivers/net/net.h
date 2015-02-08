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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#ifndef _NET_H
#define _NET_H

#define BIGBUF 4096

class NetUpsDriver: public UpsDriver
{
public:
   NetUpsDriver(UPSINFO *ups);
   virtual ~NetUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new NetUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();
   virtual bool entry_point(int command, void *data);

private:

   bool getupsvar(const char *request, char *answer, int anslen);
   bool poll_ups();
   bool fill_status_buffer();
   bool get_ups_status_flag(int fill);

   static SelfTestResult decode_testresult(char* str);
   static LastXferCause decode_lastxfer(char *str);

   struct CmdTrans {
      const char *request;
      const char *upskeyword;
      int nfields;
   };
   static const CmdTrans cmdtrans[];

   char _device[MAXSTRING];
   char *_hostname;
   int _port;
   sock_t _sockfd;
   bool _got_caps;
   bool _got_static_data;
   time_t _last_fill_time;
   char _statbuf[BIGBUF];
   int _statlen;
   time_t _tlog;
   bool _comm_err;
   int _comm_loss;
};

#endif   /* _NET_H */
