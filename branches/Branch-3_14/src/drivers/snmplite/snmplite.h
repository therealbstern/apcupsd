/*
 * snmplite.h
 *
 * Public header for the SNMP Lite UPS driver
 */

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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#ifndef _SNMPLITE_H
#define _SNMPLITE_H

//#include "snmp.h"

namespace Snmp
{
   class SnmpEngine;
   struct Variable;
};

// Forward declarations
struct MibStrategy;

class SnmpLiteUpsDriver: public UpsDriver
{
public:
   SnmpLiteUpsDriver(UPSINFO *ups);
   virtual ~SnmpLiteUpsDriver() {}

   static UpsDriver *Factory(UPSINFO *ups)
      { return new SnmpLiteUpsDriver(ups); }

   virtual bool get_capabilities();
   virtual bool read_volatile_data();
   virtual bool read_static_data();
   virtual bool kill_power();
   virtual bool check_state();
   virtual bool Open();
   virtual bool Close();
   virtual bool entry_point(int command, void *data);
   virtual bool shutdown();

private:

   const char *snmplite_probe_community();
   const MibStrategy *snmplite_probe_mib();
   bool update_cis(bool dynamic);

   static bool check_ci(int ci, Snmp::Variable &data);

   char _device[MAXSTRING];       /* Copy of ups->device */
   const char *_host;             /* hostname|IP of peer */
   unsigned short _port;          /* Remote port, usually 161 */
   const char *_vendor;           /* SNMP vendor: APC or APC_NOTRAP */
   const char *_community;        /* Community name */
   Snmp::SnmpEngine *_snmp;       /* SNMP engine instance */
   int _error_count;              /* Number of consecutive SNMP network errors */
   time_t _commlost_time;         /* Time at which we declared COMMLOST */
   const MibStrategy *_strategy;  /* MIB strategy to use */
   bool _traps;                   /* true if catching SNMP traps */
};


/*********************************************************************/
/* Public Function ProtoTypes                                        */
/*********************************************************************/

extern int snmplite_ups_get_capabilities(UPSINFO *ups);
extern int snmplite_ups_read_volatile_data(UPSINFO *ups);
extern int snmplite_ups_read_static_data(UPSINFO *ups);
extern int snmplite_ups_kill_power(UPSINFO *ups);
extern int snmplite_ups_shutdown(UPSINFO *ups);
extern int snmplite_ups_check_state(UPSINFO *ups);
extern int snmplite_ups_open(UPSINFO *ups);
extern int snmplite_ups_close(UPSINFO *ups);
extern int snmplite_ups_setup(UPSINFO *ups);
extern int snmplite_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int snmplite_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _SNMPLITE_H */
