/*
 * options.c
 *
 * Command line options parsing routine for apcupsd.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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

static char *const shortoptions = "b?RtTVcf:ud:npP:k";

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
   OPT_PIDFILE
};

static const struct option longoptions[] = {
   {"help",                no_argument,       NULL, OPT_HELP},
   {"version",             no_argument,       NULL, OPT_VERSION},
   {"config-file",         required_argument, NULL, OPT_CFGFILE},
   {"configure",           no_argument,       NULL, OPT_CONFIG},
   {"update-battery-date", no_argument,       NULL, OPT_BATTERY},
   {"debug",               required_argument, NULL, OPT_DEBUG},
   {"rename-ups",          no_argument,       NULL, OPT_RENAME},
   {"killpower",           no_argument,       NULL, OPT_KILLPWR},
   {"term-on-powerfail",   no_argument,       NULL, OPT_TERMONPWRFAIL},
   {"kill-on-powerfail",   no_argument,       NULL, OPT_KILLONPWRFAIL},
   {"pid-file",            required_argument, NULL, OPT_PIDFILE},
   {0,                     no_argument,       NULL, OPT_NOARG}
};

/* Globals used by command line options. */
int show_version = FALSE;
char *cfgfile = NULL;
int configure_ups = FALSE;
int update_battery_date = FALSE;
int rename_ups = FALSE;
int terminate_on_powerfail = FALSE;
int kill_on_powerfail = FALSE;
int kill_ups_power = FALSE;
int dumb_mode_test = FALSE;        /* for testing dumb mode */
int go_background = TRUE;
char *pidfile = APCPID;
extern bool trace;

static void print_usage(char *argv[])
{
   printf(_("usage: apcupsd [options]\n"
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
         "  -T                        send debug to ./apcupsd.trace\n"
         "  -u, --update-battery-date update UPS battery date [*]\n"
         "  -V, --version             display version info\n"
         "  -?, --help                display this help\n"
         "\n"
         "  [*] Only one parameter of this kind and apcupsd must not already be running.\n"
         "\n"
         "Copyright (C) 1999-2005 Kern Sibbald\n"
         "Copyright (C) 1996-1999 Andre Hedrick\n"
         "Copyright (C) 1999-2001 Riccardo Facchetti\n"
         "apcupsd is free software and comes with ABSOLUTELY NO WARRANTY\n"
         "under the terms of the GNU General Public License\n"
         "\n"
         "Report bugs to apcupsd Support Center:\n"
         "  apcupsd-users@lists.sourceforge.net\n"));
}

int parse_options(int argc, char *argv[])
{
   int options = 0;
   int oneshot = FALSE;
   int option_index;
   int errflag = 0;
   int c;

   while (!errflag &&
          (c = getopt_long(argc, argv, shortoptions,
            longoptions, &option_index)) != -1) {
      options++;

      switch (c) {
      case 'b':                   /* do not become daemon */
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
            /* Following option: no argument set for -f */
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
      case 'P':
      case OPT_PIDFILE:
         if (optarg[0] == '-') {
            /* Following option: no argument set for -P */
            errflag++;
            break;
         }
         options--;
         pidfile = optarg;
         break;
      case 'u':
      case OPT_BATTERY:
         update_battery_date = TRUE;
         oneshot = TRUE;
         break;
      case 'd':
      case OPT_DEBUG:
         if (optarg[0] == '-') {
            /* Following option: no argument set for -d */
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
      case 'T':
         trace = true;
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
