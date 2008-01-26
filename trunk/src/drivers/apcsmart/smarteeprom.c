/*
 * apceeprom.c
 *
 * Do APC EEPROM changes.
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "apcsmart.h"


/*********************************************************************/
bool ApcSmartDriver::ProgramEeprom(int command, const char *data)
{
   char setting[20];

   setup_device(_ups);
   GetCapabilities();

   switch (command) {
   case CI_BATTDAT:               /* change battery date */
      if (_ups->UPS_Cap[CI_BATTDAT]) {
         printf(_("Attempting to update UPS battery date ...\n"));
         change_ups_battery_date(data);
      } else {
         printf(_("UPS battery date configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

   case CI_IDEN:
      if (_ups->UPS_Cap[CI_IDEN]) {
         printf(_("Attempting to rename UPS ...\n"));
         change_ups_name(data);
      } else {
         printf(_("UPS name configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* SENSITIVITY */
   case CI_SENS:
      if (_ups->UPS_Cap[CI_SENS]) {
         asnprintf(setting, sizeof(setting), "%.1s", data);
         change_ups_eeprom_item("sensitivity", _cmdmap[CI_SENS], setting);
      } else {
         printf(_("UPS sensitivity configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* ALARM_STATUS */
   case CI_DALARM:
      if (_ups->UPS_Cap[CI_DALARM]) {
         asnprintf(setting, sizeof(setting), "%.1s", data);
         change_ups_eeprom_item("alarm status", _cmdmap[CI_DALARM],
            setting);
      } else {
         printf(_("UPS alarm status configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* LOWBATT_SHUTDOWN_LEVEL */
   case CI_DLBATT:
      if (_ups->UPS_Cap[CI_DLBATT]) {
         asnprintf(setting, sizeof(setting), "%02d", (int)atoi(data));
         change_ups_eeprom_item("low battery warning delay",
            _cmdmap[CI_DLBATT], setting);
      } else {
         printf(
            _("UPS low battery warning configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* WAKEUP_DELAY */
   case CI_DWAKE:
      if (_ups->UPS_Cap[CI_DWAKE]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item("wakeup delay", _cmdmap[CI_DWAKE],
            setting);
      } else {
         printf(_("UPS wakeup delay configuration not supported by this UPS.\n"));
         return 0;
      }
      break;


      /* SLEEP_DELAY */
   case CI_DSHUTD:
      if (_ups->UPS_Cap[CI_DSHUTD]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item("shutdown delay", _cmdmap[CI_DSHUTD],
            setting);
      } else {
         printf(_("UPS shutdown delay configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* LOW_TRANSFER_LEVEL */
   case CI_LTRANS:
      if (_ups->UPS_Cap[CI_LTRANS]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item("lower transfer voltage",
            _cmdmap[CI_LTRANS], setting);
      } else {
         printf(
            _("UPS low transfer voltage configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* HIGH_TRANSFER_LEVEL */
   case CI_HTRANS:
      if (_ups->UPS_Cap[CI_HTRANS]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item("high transfer voltage",
            _cmdmap[CI_HTRANS], setting);
      } else {
         printf(
            _("UPS high transfer voltage configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* UPS_BATT_CAP_RETURN */
   case CI_RETPCT:
      if (_ups->UPS_Cap[CI_RETPCT]) {
         asnprintf(setting, sizeof(setting), "%02d", (int)atoi(data));
         change_ups_eeprom_item("return threshold percent",
            _cmdmap[CI_RETPCT], setting);
      } else {
         printf(
            _("UPS return threshold configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* UPS_SELFTEST */
   case CI_STESTI:
      if (_ups->UPS_Cap[CI_STESTI]) {
         asnprintf(setting, sizeof(setting), "%.3s", data);
         /* Make sure "ON" is 3 characters */
         if (setting[2] == 0) {
            setting[2] = ' ';
            setting[3] = 0;
         }
         change_ups_eeprom_item("self test interval", _cmdmap[CI_STESTI],
            setting);
      } else {
         printf(
            _("UPS self test interval configuration not supported by this UPS.\n"));
         return 0;
      }
      break;

      /* OUTPUT_VOLTAGE */
   case CI_NOMOUTV:
      if (_ups->UPS_Cap[CI_NOMOUTV]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item("output voltage on batteries",
            _cmdmap[CI_NOMOUTV], setting);
      } else {
         printf(
            _("UPS output voltage on batteries configuration not supported by this UPS.\n"));
         return 0;
      }
      break;


   case -1:                       /* old style from .conf file */

      printf(_("Attempting to configure UPS ...\n"));
      change_extended();        /* set new values in UPS */

      printf("\nReading updated UPS configuration ...\n\n");
      ReadVolatileData();
      ReadStaticData();

      /* Print report of status */
      output_status(_ups, 0, stat_open, stat_print, stat_close);
      break;

   default:
      printf(_("Ignoring unknown config request command=%d\n"), command);
      return 0;
      break;
   }

   return 1;
}

/*********************************************************************/
void ApcSmartDriver::change_ups_name(const char *newname)
{
   char *n;
   char response[32];
   char name[10];
   char a = _cmdmap[CI_CYCLE_EPROM];
   char c = _cmdmap[CI_IDEN];
   int i;
   int j = strlen(newname);

   name[0] = '\0';

   if (j == 0) {
      fprintf(stderr, "Error, new name of zero length.\n");
      return;
   } else if (j > 8) {
      j = 8;                       /* maximum size */
   }

   strncpy(name, newname, 9);

   /* blank fill to 8 chars */
   while (j < 8) {
      name[j] = ' ';
      j++;
   }

   /* Ask for name */
   write(_ups->fd, &c, 1);          /* c = 'c' */
   getline(response, sizeof(response));
   fprintf(stderr, "The old UPS name is: %s\n", response);

   /* Tell UPS we will change name */
   write(_ups->fd, &a, 1);          /* a = '-' */
   sleep(1);

   n = name;
   for (i = 0; i < 8; i++) {
      write(_ups->fd, n++, 1);
      sleep(1);
   }

   /* Expect OK after successful name change */
   *response = 0;
   getline(response, sizeof(response));
   if (strcmp(response, "OK") != 0) {
      fprintf(stderr, "\nError changing UPS name\n");
   }

   _ups->upsname[0] = '\0';
   smart_poll(_cmdmap[CI_IDEN]);
   astrncpy(_ups->upsname, smart_poll(_cmdmap[CI_IDEN]),
      sizeof(_ups->upsname));

   fprintf(stderr, "The new UPS name is: %s\n", _ups->upsname);
}

/*
 * Update date battery replaced
 */
void ApcSmartDriver::change_ups_battery_date(const char *newdate)
{
   char *n;
   char response[32];
   char battdat[9];
   char a = _cmdmap[CI_CYCLE_EPROM];
   char c = _cmdmap[CI_BATTDAT];
   int i;
   int j = strlen(newdate);

   battdat[0] = '\0';

   if (j != 8) {
      fprintf(stderr, "Error, new battery date must be 8 characters long.\n");
      return;
   }

   astrncpy(battdat, newdate, sizeof(battdat));

   /* Ask for battdat */
   write(_ups->fd, &c, 1);          /* c = 'x' */
   getline(response, sizeof(response));
   fprintf(stderr, "The old UPS battery date is: %s\n", response);

   /* Tell UPS we will change battdat */
   write(_ups->fd, &a, 1);          /* a = '-' */
   sleep(1);

   n = battdat;
   for (i = 0; i < 8; i++) {
      write(_ups->fd, n++, 1);
      sleep(1);
   }

   /* Expect OK after successful battdat change */
   *response = 0;
   getline(response, sizeof(response));
   if (strcmp(response, "OK") != 0) {
      fprintf(stderr, "\nError changing UPS battery date\n");
   }
#if 0
   _ups->battdat[0] = '\0';
   smart_poll(_cmdmap[CI_BATTDAT]);
   astrncpy(_ups->battdat, smart_poll(_cmdmap[CI_BATTDAT]),
      sizeof(_ups->battdat));

   fprintf(stderr, "The new UPS battery date is: %s\n", _ups->battdat);
#endif
}

/*********************************************************************/
int ApcSmartDriver::change_ups_eeprom_item(
   const char *title, char cmd, const char *setting)
{
   char response[32];
   char response1[32];
   char oldvalue[32];
   char lastvalue[32];
   char allvalues[256];
   char a = _cmdmap[CI_CYCLE_EPROM];
   int i;

   /* Ask for old value */
   write(_ups->fd, &cmd, 1);
   if (getline(oldvalue, sizeof(oldvalue)) == FAILURE) {
      fprintf(stderr, "Could not get old value of %s.\n", title);
      return FAILURE;
   }

   if (strcmp(oldvalue, setting) == 0) {
      fprintf(stderr, "The UPS %s remains unchanged as: %s\n", title, oldvalue);
      return SUCCESS;
   }

   fprintf(stderr, "The old UPS %s is: %s\n", title, oldvalue);
   astrncpy(allvalues, oldvalue, sizeof(allvalues));
   astrncat(allvalues, " ", sizeof(allvalues));
   astrncpy(lastvalue, oldvalue, sizeof(lastvalue));

   /* Try a second time to ensure that it is a stable value */
   write(_ups->fd, &cmd, 1);
   *response = 0;
   getline(response, sizeof(response));
   if (strcmp(oldvalue, response) != 0) {
      fprintf(stderr, "\nEEPROM value of %s is not stable\n", title);
      return FAILURE;
   }

   /*
    * Just before entering this loop, the last command sent
    * to the UPS MUST be to query the old value.   
    */
   for (i = 0; i < 10; i++) {
      write(_ups->fd, &cmd, 1);
      getline(response1, sizeof(response1));

      /* Tell UPS to cycle to next value */
      write(_ups->fd, &a, 1);       /* a = '-' */

      /* Expect OK after successful change */
      *response = 0;
      getline(response, sizeof(response));
      if (strcmp(response, "OK") != 0) {
         fprintf(stderr, "\nError changing UPS %s\n", title);
         fprintf(stderr, "Got %s instead of OK\n\n", response);
         sleep(10);
         return FAILURE;
      }

      /* get cycled value */
      write(_ups->fd, &cmd, 1);
      getline(response1, sizeof(response1));

      /* get cycled value again */
      write(_ups->fd, &cmd, 1);
      if (getline(response, sizeof(response)) == FAILURE ||
         strcmp(response1, response) != 0) {
         fprintf(stderr, "Error cycling values.\n");
         getline(response, sizeof(response));      /* eat any garbage */
         return FAILURE;
      }
      if (strcmp(setting, response) == 0) {
         fprintf(stderr, "The new UPS %s is: %s\n", title, response);
         sleep(10);                /* allow things to settle down */
         return SUCCESS;
      }

      /*
       * Check if we cycled back to the same value, but permit
       * a duplicate because the L for sensitivy appears
       * twice in a row, i.e. H M L L.
       */
      if (strcmp(oldvalue, response) == 0 && i > 0)
         break;
      if (strcmp(lastvalue, response) != 0) {
         astrncat(allvalues, response, sizeof(allvalues));
         astrncat(allvalues, " ", sizeof(allvalues));
         astrncpy(lastvalue, response, sizeof(lastvalue));
      }
      sleep(5);                    /* don't cycle too fast */
   }

   fprintf(stderr, "Unable to change %s to: %s\n", title, setting);
   fprintf(stderr, "Permitted values are: %s\n", allvalues);
   getline(response, sizeof(response));    /* eat any garbage */

   return FAILURE;
}


/*
 * Set new values in EEPROM memmory.  Change the UPS EEPROM.
 */
void ApcSmartDriver::change_extended()
{
   char setting[20];

   GetCapabilities();

   /*
    * Note, a value of -1 in the variable at the beginning
    * means that the user did not put a configuration directive
    * in /etc/apcupsd/apcupsd.conf. Consequently, if no value
    * was given, we won't attept to change it.
    */
#if 0
   /* SENSITIVITY */
   if (_ups->UPS_Cap[CI_SENS] && strcmp(_ups->sensitivity, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.1s", _ups->sensitivity);
      change_ups_eeprom_item("sensitivity", _cmdmap[CI_SENS], setting);
   }

   /* WAKEUP_DELAY */
   if (_ups->UPS_Cap[CI_DWAKE] && _ups->dwake != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)_ups->dwake);
      change_ups_eeprom_item("wakeup delay", _cmdmap[CI_DWAKE], setting);
   }

   /* SLEEP_DELAY */
   if (_ups->UPS_Cap[CI_DSHUTD] && _ups->dshutd != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)_ups->dshutd);
      change_ups_eeprom_item("shutdown delay", _cmdmap[CI_DSHUTD],
         setting);
   }

   /* LOW_TRANSFER_LEVEL */
   if (_ups->UPS_Cap[CI_LTRANS] && _ups->lotrans != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)_ups->lotrans);
      change_ups_eeprom_item("lower transfer voltage",
         _cmdmap[CI_LTRANS], setting);
   }

   /* HIGH_TRANSFER_LEVEL */
   if (_ups->UPS_Cap[CI_HTRANS] && _ups->hitrans != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)_ups->hitrans);
      change_ups_eeprom_item("upper transfer voltage",
         _cmdmap[CI_HTRANS], setting);
   }

   /* UPS_BATT_CAP_RETURN */
   if (_ups->UPS_Cap[CI_RETPCT] && _ups->rtnpct != -1) {
      asnprintf(setting, sizeof(setting), "%02d", (int)_ups->rtnpct);
      change_ups_eeprom_item("return threshold percent",
         _cmdmap[CI_RETPCT], setting);
   }

   /* ALARM_STATUS */
   if (_ups->UPS_Cap[CI_DALARM] && strcmp(_ups->beepstate, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.1s", _ups->beepstate);
      change_ups_eeprom_item("alarm delay", _cmdmap[CI_DALARM], setting);
   }

   /* LOWBATT_SHUTDOWN_LEVEL */
   if (_ups->UPS_Cap[CI_DLBATT] && _ups->dlowbatt != -1) {
      asnprintf(setting, sizeof(setting), "%02d", (int)_ups->dlowbatt);
      change_ups_eeprom_item("low battery warning delay",
         _cmdmap[CI_DLBATT], setting);
   }

   /* UPS_SELFTEST */
   if (_ups->UPS_Cap[CI_STESTI] && strcmp(_ups->selftest, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.3s", _ups->selftest);
      /* Make sure "ON" is 3 characters */
      if (setting[2] == 0) {
         setting[2] = ' ';
         setting[3] = 0;
      }
      change_ups_eeprom_item(
         "self test interval", _cmdmap[CI_STESTI], setting);
   }

   /* OUTPUT_VOLTAGE */
   if (_ups->UPS_Cap[CI_NOMOUTV] && _ups->NomOutputVoltage != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)_ups->NomOutputVoltage);
      change_ups_eeprom_item("output voltage on batteries",
         _cmdmap[CI_NOMOUTV], setting);
   }
#endif
}
