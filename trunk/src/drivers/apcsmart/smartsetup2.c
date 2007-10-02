/*
 * smartsetup2.c
 *
 * UPS capability discovery for SmartUPS models.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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
char *ApcSmartDriver::get_apc_model_V_codes(const char *s)
{
   switch (s[0]) {
   case '0':
      return ("APC Matrix-UPS 3000");
   case '2':
      return ("APC Smart-UPS 250");
   case '3':
      return ("APC Smart-UPS 370ci");
   case '4':
      return ("APC Smart-UPS 400");
   case '5':
      return ("APC Matrix-UPS 5000");
   case '6':
      return ("APC Smart-UPS 600");
   case '7':
      return ("APC Smart-UPS 900");
   case '8':
      return ("APC Smart-UPS 1250");
   case '9':
      return ("APC Smart-UPS 2000");
   case 'A':
      return ("APC Smart-UPS 1400");
   case 'B':
      return ("APC Smart-UPS 1000");
   case 'C':
      return ("APC Smart-UPS 650");
   case 'D':
      return ("APC Smart-UPS 420");
   case 'E':
      return ("APC Smart-UPS 280");
   case 'F':
      return ("APC Smart-UPS 450");
   case 'G':
      return ("APC Smart-UPS 700");
   case 'H':
      return ("APC Smart-UPS 700 XL");
   case 'I':
      return ("APC Smart-UPS 1000");
   case 'J':
      return ("APC Smart-UPS 1000 XL");
   case 'K':
      return ("APC Smart-UPS 1400");
   case 'L':
      return ("APC Smart-UPS 1400 XL");
   case 'M':
      return ("APC Smart-UPS 2200");
   case 'N':
      return ("APC Smart-UPS 2200 XL");
   case 'P':
      return ("APC Smart-UPS 3000");
   case 'O':
      return ("APC Smart-UPS 5000");
   default:
      break;
   }

   return (_ups->mode.long_name);
}

/*********************************************************************/
char *ApcSmartDriver::get_apc_model_b_codes(const char *s)
{
   /*
    * New Firmware revison and model ID String in NN.M.D format,
    * where...
    *
    *    NN is UPS ID Code:
    *        5 == Back-UPS 350
    *        6 == Back-UPS 500
    *       12 == Back-UPS Pro 650
    *       13 == Back-UPS Pro 1000
    *       52 == Smart-UPS 700
    *       60 == Smart-UPS 1000
    *       72 == Smart-UPS 1400
    *
    *    M is Possible Case Style, unknown???
    *       1 == Stand Alone
    *       2 == Plastic
    *       8 == Rack Mount
    *       9 == Rack Mount
    *
    *    D == Domestic; I == International; ...
    */
   return _ups->mode.long_name;
}

/*********************************************************************/
void ApcSmartDriver::get_apc_model()
{
   char response[32];
   char *cp;
   unsigned char b;
   int i;

   response[0] = '\0';

   for (i = 0; i < 4; i++) {
      b = 0x01;
      write(_ups->fd, &b, 1);
      sleep(1);
   }
   getline(response, sizeof response);

   if (strlen(response)) {
      _ups->mode.long_name[0] = '\0';
      asnprintf(_ups->mode.long_name, sizeof(_ups->mode.long_name), "%s", response);
      return;
   }

   response[0] = '\0';
   astrncpy(response, smart_poll(_ups->UPS_Cmd[CI_UPSMODEL]), sizeof(response));

   if (strlen(response)) {
      cp = get_apc_model_V_codes(response);
      if (cp != _ups->mode.long_name)
         asnprintf(_ups->mode.long_name, sizeof(_ups->mode.long_name), "%s", cp);
      return;
   }

   response[0] = '\0';
   astrncpy(response, smart_poll(_ups->UPS_Cmd[CI_REVNO]), sizeof(response));

   if (strlen(response)) {
      fprintf(stderr, "\n%s: 'b' %s", argvalue,
         get_apc_model_b_codes(response));
   }
}


/*
 * This subroutine polls the APC Smart UPS to find out 
 * what capabilities it has.          
 */
bool ApcSmartDriver::GetCapabilities()
{
   char answer[1000];              /* keep this big to handle big string */
   char caps[1000], *cmds, *p;
   int i;

   /* Get UPS capabilities string */
   astrncpy(caps, smart_poll(_ups->UPS_Cmd[CI_UPS_CAPS]), sizeof(caps));
   if (strlen(caps) && (strcmp(caps, "NA") != 0)) {
      _ups->UPS_Cap[CI_UPS_CAPS] = TRUE;

      /* skip version */
      for (p = caps; *p && *p != '.'; p++)
         ;

      /* skip alert characters */
      for (; *p && *p != '.'; p++)
         ;

      cmds = p;                    /* point to commands */
      if (!*cmds) {                /* oops, none */
         cmds = NULL;
         _ups->UPS_Cap[CI_UPS_CAPS] = FALSE;
      }
   } else {
      cmds = NULL;                 /* No commands string */
   }

   /*
    * Try all the possible UPS capabilities and mark the ones supported.
    * If we can get the eprom caps list, use them to jump over the
    * unsupported caps, if we can't get the cap list try every known
    * capability.
    */
   for (i = 0; i <= CI_MAX_CAPS; i++) {
      if (_ups->UPS_Cmd[i] == 0)
         continue;
      if (!cmds || strchr(cmds, _ups->UPS_Cmd[i]) != NULL) {
         astrncpy(answer, smart_poll(_ups->UPS_Cmd[i]), sizeof(answer));
         if (*answer && (strcmp(answer, "NA") != 0)) {
            _ups->UPS_Cap[i] = true;
         }
      }
   }

   return 1;
}
