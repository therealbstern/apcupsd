/*
 * smartoper.c
 *
 * Functions for SmartUPS operations
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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
#include "apcsmart.h"

bool ApcSmartDriver::KillPower()
{
   char response[32];
   int shutdown_delay = 0;
   int errflag = 0;

   response[0] = '\0';
   shutdown_delay = GetShutdownDelay();
   
   writechar('S');                 /* ask for soft shutdown */

   /*
    * Check whether the UPS has acknowledged the power-off command.
    * This might not happen in rare cases, when mains-power returns
    * just after LINUX starts with the shutdown sequence.
    * interrupt the ouput-power. So LINUX will not come up without
    * operator intervention.  w.p.
    */
   sleep(5);
   getline(response, sizeof response);
   if (strcmp(response, "OK") == 0) {
      WarnShutdown(shutdown_delay);
   } else {
      /*
       * Experiments show that the UPS needs delays between chars
       * to accept this command.
       */

      sleep(1);                    /* Shutdown now */
      writechar('@');
      sleep(1);
      writechar('0');
      sleep(1);
      writechar('0');
      sleep(1);
      writechar('0');

      getline(response, sizeof(response));
      if ((strcmp(response, "OK") == 0) || (strcmp(response, "*") == 0)) {
         WarnShutdown(shutdown_delay);
      } else {
         errflag++;
      }
   }

   if (errflag) {
      writechar('@');
      sleep(1);
      writechar('0');
      sleep(1);
      writechar('0');
      sleep(1);

      if ((_ups->mode.type == BKPRO) || (_ups->mode.type == VS)) {
         log_event(_ups, LOG_WARNING,
            _("BackUPS Pro and SmartUPS v/s sleep for 6 min"));
         writechar('1');
      } else {
         writechar('0');
      }

      getline(response, sizeof(response));
      if ((strcmp(response, "OK") == 0) || (strcmp(response, "*") == 0)) {
         WarnShutdown(shutdown_delay);
         errflag = 0;
      } else {
         errflag++;
      }
   }

   if (errflag) {
      return ShutdownWithDelay(shutdown_delay);
   }
   
   return 1;
}

bool ApcSmartDriver::Shutdown()
{
   return ShutdownWithDelay(GetShutdownDelay());
}

bool ApcSmartDriver::ShutdownWithDelay(int shutdown_delay)
{
   char response[32];
      
   /*
    * K K command
    *
    * This method should turn the UPS off completely according to this article:
    * http://nam-en.apc.com/cgi-bin/nam_en.cfg/php/enduser/std_adp.php?p_faqid=604
    */
   
   writechar('K');
   sleep(2);
   writechar('K');
   getline(response, sizeof response);
   if ((strcmp(response, "*") == 0) || (strcmp(response, "OK") == 0) ||
      (_ups->mode.type >= BKPRO)) {
      WarnShutdown(shutdown_delay);
      return 1;
   } else {
      log_event(_ups, LOG_WARNING, _("Unexpected error!\n"));
      log_event(_ups, LOG_WARNING, _("UPS in unstable state\n"));
      log_event(_ups, LOG_WARNING,
         _("You MUST power off your UPS before rebooting!!!\n"));
      return 0;
   }
}

void ApcSmartDriver::WarnShutdown(int shutdown_delay)
{
   if (shutdown_delay > 0) {
      log_event(_ups, LOG_WARNING,
         "UPS will power off after %d seconds ...\n", shutdown_delay);
   } else {
      log_event(_ups, LOG_WARNING,
         "UPS will power off after the configured delay  ...\n");
   }
   log_event(_ups, LOG_WARNING,
      _("Please power off your UPS before rebooting your computer ...\n"));
}

int ApcSmartDriver::GetShutdownDelay()
{
   char response[32];

   writechar(_cmdmap[CI_DSHUTD]);
   getline(response, sizeof(response));
   return (int)atof(response);
}

bool ApcSmartDriver::CheckState()
{
   return getline(NULL, 0) == SUCCESS;
}
