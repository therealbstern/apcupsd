/*
 *  apcoptd.c -- command line options parsing routine for apcupsd.
 *
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *		 BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *	      -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
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

static char *shortoptions = "b?RtVcf:ud:npk";

enum {
    OPT_NOARG,
    OPT_HELP,
    OPT_VERSION,
    OPT_CFGFILE,
    OPT_CONFIG,
    OPT_BATTERY,
    OPT_DEBUG,
    OPT_RENAME,
    OPT_KILLPWR,
    OPT_TERMONPWRFAIL,
    OPT_KILLONPWRFAIL,
};

static struct option longoptions[] = {
    { "help",                       no_argument,            NULL,   OPT_HELP        },
    { "version",                    no_argument,            NULL,   OPT_VERSION     },
    { "config-file",                required_argument,      NULL,   OPT_CFGFILE     },
    { "configure",                  no_argument,            NULL,   OPT_CONFIG      },
    { "update-battery-date",        no_argument,            NULL,   OPT_BATTERY     },
    { "debug",                      required_argument,      NULL,   OPT_DEBUG       },
    { "rename-ups",                 no_argument,            NULL,   OPT_RENAME      },
    { "killpower",                  no_argument,            NULL,   OPT_KILLPWR     },
    { "term-on-powerfail",          no_argument,            NULL,   OPT_TERMONPWRFAIL },
    { "kill-on-powerfail",          no_argument,            NULL,   OPT_KILLONPWRFAIL },
    { 0,			    no_argument,	    NULL,   OPT_NOARG	    }
};

/*
 * Globals used by command line options.
 */
int show_version = FALSE;
char *cfgfile = NULL;
int configure_ups = FALSE;
int update_battery_date = FALSE;
int rename_ups = FALSE;
int terminate_on_powerfail = FALSE;
int kill_on_powerfail = FALSE;
int kill_ups_power = FALSE;
int dumb_mode_test = FALSE;		/* for testing dumb mode */
int go_background = TRUE;

extern int debug_level;

static void print_usage(char *argv[]) {
    /*
     * I don't like fprintf when I try to get some help from a program.
     * Doing `... 2> pippo' and then `less pippo' just because some
     * `kind' person [ass] has decided to output the help screen to stderr
     * makes me _very_ angry.
     */
    printf(_(
            "usage: apcupsd [options]\n"
            "  Options are as follows:\n"
            "  -b,                       don't go into background\n"
            "  -c, --configure           attempt to configure UPS [*]\n"
            "  -d, --debug <level>       set debug level (>0)\n"
            "  -f, --config-file <file>  load specified config file\n"
            "  -k, --killpower           attempt to power down UPS [*]\n"
            "  -n, --rename-ups          attempt to rename UPS [*]\n"
            "  -p, --kill-on-powerfail   power down UPS on powerfail\n"
            "  -R,                       put SmartUPS into dumb mode\n"
            "  -t, --term-on-powerfail   terminate when battery power fails\n"
            "  -u, --update-battery-date update UPS battery date [*]\n"
            "  -V, --version             display version info\n"
            "  -?, --help                display this help\n"
            "\n"
            "  [*] Only one parameter of this kind and apcupsd must not already be running.\n"
            "\n"
            "Copyright (C) 1999-2003 Kern Sibbald\n"
            "Copyright (C) 1996-1999 Andre Hedrick\n"
            "Copyright (C) 1999-2001 Riccardo Facchetti\n"
            "apcupsd is free software and comes with ABSOLUTELY NO WARRANTY\n"
            "under the terms of the GNU General Public License\n"
            "\n"
            "Report bugs to apcupsd Support Center:\n"
            "  apcupsd-users@lists.sourceforge.net\n"));
}

int parse_options(int argc, char *argv[]) {
    int options = 0;
    int oneshot = FALSE;
    int option_index;
    int errflag = 0;
    int c;

    while(!errflag &&
	       (c = getopt_long(argc, argv, shortoptions,
						    longoptions, &option_index)) != -1) {

    options++;

    switch(c) {
        case 'b':                     /* do not become daemon */
	    go_background = FALSE;
	    break;
        case 'R':
	    dumb_mode_test = TRUE;
	    break;
        case 'V':
	case OPT_VERSION:
	    show_version = TRUE;
	    oneshot = TRUE;
	    break;
        case 'c':
	case OPT_CONFIG:
	    configure_ups = TRUE;
	    oneshot = TRUE;
	    break;
        case 'f':
	case OPT_CFGFILE:
            if (optarg[0] == '-') {
		    /*
		     * Following option: no argument set for -f
		     */
		    errflag++;
		    break;
	    }
	    /*
	     * The reason for this is that loading a different config file
             * is not something dangerous when we are doing `oneshot' things
	     * and we may even want to load a different config file during
             * `oneshot's.
	     */
	    options--;
	    cfgfile = optarg;
	    break;
        case 'u':
	case OPT_BATTERY:
	    update_battery_date = TRUE;
	    oneshot = TRUE;
	    break;
        case 'd':
	case OPT_DEBUG:
            if (optarg[0] == '-') {
		    /*
		     * Following option: no argument set for -d
		     */
		    errflag++;
		    break;
	    }
	    /*
	     * The reason of this, is that enabling debugging is
             * not something dangerous when we are doing `oneshot's
	     */
	    options--;
	    debug_level = atoi(optarg);
	    break;
        case 'n':
	case OPT_RENAME:
	    rename_ups = TRUE;
	    oneshot = TRUE;
	    break;
        case 'p':
	case OPT_KILLONPWRFAIL:
	    kill_on_powerfail = TRUE;
	    break;
        case 't':
	case OPT_TERMONPWRFAIL:
	    terminate_on_powerfail = TRUE;
	    break;
        case 'k':
	case OPT_KILLPWR:
	    kill_ups_power = TRUE;
	    oneshot = TRUE;
	    break;
        case '?':
	case OPT_HELP:
	default:
	    errflag++;
	}
    }

    if ((oneshot == TRUE) && options > 1) {
        fprintf(stderr, _("\nError: too many arguments.\n\n"));
	errflag++;
    }

    if (errflag)
	print_usage(argv);

    return errflag;
}
