/*
 * apcdevice.c
 *
 * Generic device handling
 *
 * Written by Riccardo Fachetti 2000 from Kern's design
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
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

/*    
 * This is a sort of helper routine between apcupsd core and the drivers.
 * The raw calls into the drivers are through function pointers in the 
 * ups device structure, and are #defined to the following:
 *
 *    device_open(ups)
 *    device_setup(ups)
 *    device_close(ups)
 *    device_kill_power(ups)
 *    device_shutdown(ups)
 *    device_read_static_data(ups)
 *    device_read_volatile_data(ups)
 *    device_get_capabilities(ups) 
 *    device_check_state(ups)
 *    device_program_eeprom(ups)
 *    device_entry_point(ups, command, data)
 * 
 * see include/apc_drivers.h for more details on each routine.
 */

#include "apc.h"

/* Forward referenced function */
static int device_wait_time(UPSINFO *ups);

/*********************************************************************/
bool setup_device(UPSINFO *ups)
{
   return device_open(ups) &&
          device_setup(ups) &&
          device_get_capabilities(ups);
}

/*********************************************************************/
void initiate_hibernate(UPSINFO *ups)
{
   int pwdf;
   int killcount;

   if (ups->mode.type == DUMB_UPS) {
      /* Make sure we are on battery */
      for (killcount = 0; killcount < 3; killcount++)
         device_read_volatile_data(ups);
   }

   /*
    * ****FIXME*****  This big if is BROKEN in the
    *                 case that there is no real power failure.
    *
    * We really need to find out if we are on batteries
    * and if not, delete the PWRFAIL file.  Note, the code
    * above only tests UPS_onbatt flag for dumb UPSes.
    */

   pwdf = open(ups->pwrfailpath, O_RDONLY|O_CLOEXEC);
   if ((pwdf == -1 && ups->mode.type != DUMB_UPS) ||
       (pwdf == -1 && ups->is_onbatt() && ups->mode.type == DUMB_UPS)) {

      /*                                                  
       * At this point, we really should not be attempting
       * a kill power since either the powerfail file is
       * not defined, or we are not on batteries.
       */

      /* Now complain */
      log_event(ups, LOG_WARNING,
         "Cannot find %s file.\n Killpower requested in "
           "non-power fail condition or bug.\n Killpower request "
           "ignored at %s:%d\n", ups->pwrfailpath, __FILE__, __LINE__);
      Error_abort(
         "Cannot find %s file.\n Killpower requested in "
           "non-power fail condition or bug.\n Killpower request "
           "ignored at %s:%d\n", ups->pwrfailpath, __FILE__, __LINE__);
   } else {
      /* We are on batteries, so do the kill_power */
      if (ups->upsclass.type == SHAREMASTER) {
         log_event(ups, LOG_WARNING,
            "Waiting 30 seconds for slave(s) to shutdown.");
         sleep(30);
      }

      /* close the powerfail file */
      if (pwdf != -1)
         close(pwdf);

      log_event(ups, LOG_WARNING, "Attempting to kill the UPS power!");

      if (ups->upsclass.type == SHARESLAVE) {
         sleep(10);
         log_event(ups, LOG_WARNING, "Waiting For ShareUPS Master to shutdown");
         sleep(60);
         log_event(ups, LOG_WARNING, "Failed to have power killed by Master!");

         /*
          * ***FIXME*** this really should not do a reboot here,
          * but rather a halt or nothing -- KES
          */
         /* generate_event(ups, CMDDOREBOOT); */

         log_event(ups, LOG_WARNING, "Perform CPU-reset or power-off");
         return;
      } else {
         /* it must be a SmartUPS or BackUPS */
         device_kill_power(ups);
      }
   }
}

/*********************************************************************/
void initiate_shutdown(UPSINFO *ups)
{
   log_event(ups, LOG_WARNING, "Attempting to shutdown the UPS!");
   device_shutdown(ups);
}

/*
 * After the device is initialized, we come here
 * to read all the information we can about the UPS.
 */
void prep_device(UPSINFO *ups)
{
   device_read_static_data(ups);

   /* If no UPS name found, use hostname, or "default" */
   if (ups->upsname[0] == 0) {     /* no name given */
      gethostname(ups->upsname, sizeof(ups->upsname) - 1);
      if (ups->upsname[0] == 0)    /* error */
         strlcpy(ups->upsname, "default", sizeof(ups->upsname));
   }

   /* Strip unprintable characters from UPS model */
   for (char *ptr = ups->upsmodel; *ptr; ptr++) 
   {
      if (!isprint(*ptr))
         *ptr = ' ';
   }
}

/* Called once every 5 seconds to read all UPS info */
int fillUPS(UPSINFO *ups)
{
   device_read_volatile_data(ups);

   return 0;
}

static void open_ups(UPSINFO *ups)
{
   // Don't issue a COMMLOST event until we've been running for a little while
   static const time_t COMMLOST_EVENT_GRACE_PERIOD = 60;

   time_t event_time = 0;

   while (!device_open(ups))
   {
      // Failed to communicate with UPS: we're COMMLOST now
      ups->set_commlost();

      // Do not generate COMMLOST event until we've retried a few times
      time_t now = time(NULL);
      if (now - ups->start_time >= COMMLOST_EVENT_GRACE_PERIOD)
      {
         // Generate an event once
         if (event_time == 0)
         {
            generate_event(ups, CMDCOMMFAILURE);
            event_time = now;
         }

         // Log every 10 minutes thereafter
         if ((now - event_time) >= 10*60)
         {
            event_time = now;
            log_event(ups, event_msg[CMDCOMMFAILURE].level,
               event_msg[CMDCOMMFAILURE].msg);            
         }
      }

      sleep(5);
   }

   // If we were commlost, we're not any more
   if (ups->is_commlost())
   {
      ups->clear_commlost();
      if (event_time)
         generate_event(ups, CMDCOMMOK);
   }

   // Complete remainder of UPS setup
   device_setup(ups);
   device_get_capabilities(ups);
   prep_device(ups);
}

/* NOTE! This is the starting point for a separate process (thread). */
void do_device(UPSINFO *ups)
{
   /* Open the UPS device and ensure we can talk to it. This does not return
      until the UPS is successfully contacted */
   open_ups(ups);

   /* get all data so apcaccess is happy */
   fillUPS(ups);

   while(1)
   {
      /* compute appropriate wait time */
      ups->wait_time = device_wait_time(ups);

      Dmsg(70, "Before do_action: 0x%x (OB:%d).\n",
         ups->Status, ups->is_onbatt());

      /* take event actions */
      do_action(ups);

      Dmsg(70, "Before fillUPS: 0x%x (OB:%d).\n",
         ups->Status, ups->is_onbatt());

      /* Get all info available from UPS by asking it questions */
      fillUPS(ups);

      Dmsg(70, "Before do_action: 0x%x (OB:%d).\n",
         ups->Status, ups->is_onbatt());

      /* take event actions */
      do_action(ups);

      Dmsg(70, "Before do_reports: 0x%x (OB:%d).\n",
         ups->Status, ups->is_onbatt());

      do_reports(ups);

      /* compute appropriate wait time */
      ups->wait_time = device_wait_time(ups);

      Dmsg(70, "Before device_check_state: 0x%x (OB:%d).\n",
         ups->Status, ups->is_onbatt());

      /*
       * Check the UPS to see if has changed state.
       * This routine waits a reasonable time to prevent
       * consuming too much CPU time.
       */
      device_check_state(ups);
   }
}

/* 
 * Each device handler when called at device_check_state() waits
 * a specified time for the next event. In general, we want this
 * to be as long as possible (i.e. about a minute) to prevent
 * excessive "polling" because when device_check_state() returns,
 * repoll the UPS (fillUPS). 
 * This routine attempts to determine a reasonable "sleep" time
 * for the device.
 */
static int device_wait_time(UPSINFO *ups)
{
   int wait_time;

   if (ups->is_fastpoll() || !ups->is_battpresent())
      wait_time = TIMER_FAST;
   else if (ups->is_commlost())
      wait_time = TIMER_FAST*5;
   else
      wait_time = ups->polltime;    /* normally 60 seconds */

   /* Make sure we do data and stats when asked */
   if (ups->datatime && ups->datatime < wait_time)
      wait_time = ups->datatime;

   if (ups->stattime && ups->stattime < wait_time)
      wait_time = ups->stattime;

   /* Sanity check */
   if (wait_time < TIMER_FAST)
      wait_time = TIMER_FAST;

   return wait_time;
}
