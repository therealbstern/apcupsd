/*
 *  apcsetup.c	      -- configuration parts for apcupsd package
 *
 *  apcupsd.c	     -- Simple Daemon to catch power failure signals from a
 *		     BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		  -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *			 GNU GENERAL PUBLIC LICENSE
 *			    Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			       675 Mass Ave, Cambridge, MA 02139, USA
 *  Everyone is permitted to copy and distribute verbatim copies
 *  of this license document, but changing it is not allowed.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 *  IN NO EVENT SHALL ANY AND ALL PERSONS INVOLVED IN THE DEVELOPMENT OF THIS
 *  PACKAGE, NOW REFERRED TO AS "APCUPSD-Team" BE LIABLE TO ANY PARTY FOR
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ANY OR ALL
 *  OF THE "APCUPSD-Team" HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  THE "APCUPSD-Team" SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 *  ON AN "AS IS" BASIS, AND THE "APCUPSD-Team" HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *  THE "APCUPSD-Team" HAS ABSOLUTELY NO CONNECTION WITH THE COMPANY
 *  AMERICAN POWER CONVERSION, "APCC".  THE "APCUPSD-Team" DID NOT AND
 *  HAS NOT SIGNED ANY NON-DISCLOSURE AGREEMENTS WITH "APCC".  ANY AND ALL
 *  OF THE LOOK-A-LIKE ( UPSlink(tm) Language ) WAS DERIVED FROM THE
 *  SOURCES LISTED BELOW.
 *
 */

#include "apc.h"
#include "apcsmart.h"

/*********************************************************************/
static char *get_apc_model_V_codes (const char *s, UPSINFO *ups)
{

    switch(s[0]) {
    case '0': return("APC Matrix-UPS 3000");
    case '2': return("APC Smart-UPS 250");
    case '3': return("APC Smart-UPS 370ci");
    case '4': return("APC Smart-UPS 400");
    case '5': return("APC Matrix-UPS 5000");
    case '6': return("APC Smart-UPS 600");
    case '7': return("APC Smart-UPS 900");
    case '8': return("APC Smart-UPS 1250");
    case '9': return("APC Smart-UPS 2000");
    case 'A': return("APC Smart-UPS 1400");
    case 'B': return("APC Smart-UPS 1000");
    case 'C': return("APC Smart-UPS 650");
    case 'D': return("APC Smart-UPS 420");
    case 'E': return("APC Smart-UPS 280");
    case 'F': return("APC Smart-UPS 450");
    case 'G': return("APC Smart-UPS 700");
    case 'H': return("APC Smart-UPS 700 XL");
    case 'I': return("APC Smart-UPS 1000");
    case 'J': return("APC Smart-UPS 1000 XL");
    case 'K': return("APC Smart-UPS 1400");
    case 'L': return("APC Smart-UPS 1400 XL");
    case 'M': return("APC Smart-UPS 2200");
    case 'N': return("APC Smart-UPS 2200 XL");
    case 'P': return("APC Smart-UPS 3000");
    case 'O': return("APC Smart-UPS 5000");
    default:
	break;
    }
    return(ups->mode.long_name);
}

/*********************************************************************/
static char *get_apc_model_b_codes(const char *s, UPSINFO *ups)
{
/*
 *	  New Firmware revison and model ID String in NN.M.D is the format
 *         This returns two numbers and a character "D"
 *	       where NN == UPS ID Code.
 *		    5 == Back-UPS 350
 *		    6 == Back-UPS 500
 *		   12 == Back-UPS Pro 650
 *		   13 == Back-UPS Pro 1000
 *		   52 == Smart-UPS 700
 *		   60 == Smart-UPS 1000
 *		   72 == Smart-UPS 1400
 *	       where M == Possible Case Style, unknown???
 *		   1 == Stand Alone
 *		   2 == Plastic
 *		   8 == Rack Mount
 *		   9 == Rack Mount
 *	       where D == Domestic; I == International; ...
 *
 */

/*      fprintf(stderr, "\n%s: 'b' %s", argvalue, s); */

	return ups->mode.long_name; 
}

/*********************************************************************/
void get_apc_model(UPSINFO *ups)
{
    char response[32];
    char *cp;
    unsigned char b;
    int i;
    response[0] = '\0';

    for (i=0; i<4; i++) {
	b=0x01;
	write(ups->fd, &b, 1);
	sleep(1);
    }
    getline(response, sizeof response, ups);

    if (strlen(response)) {
        ups->mode.long_name[0] = '\0';
        asnprintf(ups->mode.long_name, sizeof(ups->mode.long_name), "%s", response);
	return;
    }

    response[0] = '\0';
    astrncpy(response, smart_poll(ups->UPS_Cmd[CI_UPSMODEL], ups), sizeof(response));

    if (strlen(response)) {
	cp = get_apc_model_V_codes(response, ups);
	if (cp != ups->mode.long_name)
            asnprintf(ups->mode.long_name, sizeof(ups->mode.long_name), "%s", cp);
	return;
    }

    response[0] = '\0';
    astrncpy(response, smart_poll(ups->UPS_Cmd[CI_REVNO], ups), sizeof(response));

    if (strlen(response))
        fprintf(stderr, "\n%s: 'b' %s", argvalue,
		get_apc_model_b_codes(response, ups));

}


/********************************************************************* 
 *
 * This subroutine polls the APC Smart UPS to find out 
 *   what capabilities it has.		    
 *
 */
int apcsmart_ups_get_capabilities(UPSINFO *ups)
{
    char answer[1000];	/* keep this big to handle big string */
    char caps[1000], *cmds, *p;
    int i;

    /* Get UPS capabilities string */
    astrncpy(caps, smart_poll(ups->UPS_Cmd[CI_UPS_CAPS], ups), sizeof(caps));
    if (strlen(caps) && (strcmp(caps,"NA") != 0)) {
	ups->UPS_Cap[CI_UPS_CAPS] = TRUE;
	/* skip version */
        for (p = caps; *p && *p != '.'; p++) {
	}
	/* skip alert characters */
        for (; *p && *p != '.'; p++) {
	}
	cmds = p;		       /* point to commands */
	if (!*cmds) {		       /* oops, none */
	   cmds = NULL;
	   ups->UPS_Cap[CI_UPS_CAPS] = FALSE;
	}
    } else {
	cmds = NULL;			  /* No commands string */
    }

    /*
     * Try all the possible UPS capabilities and mark the ones supported.
     * If we can get the eprom caps list, use them to jump over the
     * unsupported caps, if we can't get the cap list try every known
     * capability.
     */
    for (i = 0; i <= CI_MAX_CAPS; i++) {
	if (!cmds || strchr(cmds, ups->UPS_Cmd[i]) != NULL) {
	    astrncpy(answer, smart_poll(ups->UPS_Cmd[i], ups), sizeof(answer));
            if (answer && *answer && (strcmp(answer, "NA") != 0)) {
		ups->UPS_Cap[i] = true;
	    }
	}
    }
    return 1;
}
