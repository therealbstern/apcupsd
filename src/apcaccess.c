/*
 * apcaccess.c
 *
 * Text based IPC management tool for apcupsd package.
 */

/*
 * Copyright (C) 2000-2006 Kern Sibbald
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#include "apc.h"

/* These are all the possible unit labels generated in src/lib/apcstatus.c
 * -u removes them. (To make life easier for scripts.)
 */
const char *const units[] = {
  " Minutes\n",
  " Seconds\n",
  " Percent\n",
  " Volts\n",
  " Watts\n",
  " Hz\n",
  " C\n",
};

/* Behavior modifying flags */
#define NO_UNITS 0x1

/* Get and print status from apcupsd NIS server */
static int do_pthreads_status(const char *host, int port, const char *par, int flags)
{
   sock_t sockfd;
   int n;
   char recvline[MAXSTRING + 1];
   char *line;

   if ((sockfd = net_open(host, NULL, port)) < 0) {
      fprintf(stderr, "Error contacting apcupsd @ %s:%d: %s\n",
         host, port, strerror(-sockfd));
      return 1;
   }

   net_send(sockfd, "status", 6);

   while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) {
      recvline[n] = 0;
      line = recvline;
      if (par) { /* Check for match against parameter name */
         char *r = NULL;
         char *var;

         var = strtok_r(recvline, ":", &r);
         if (!var)   // No : separator? Skip it.
            continue;
         line = recvline + strlen(var) + 1;
         if ((r = strchr(var, ' ')))
            *r = '\0';
         if (strcmp(par, var)) // Doesn't match parameter? Skip it.
            continue;
         while (*line == ' ')
            line++;
      }
      if (flags & NO_UNITS) { /* Remove units labels */
         size_t i;
         for (i = 0; i < sizeof units / sizeof units[0]; i++) {
            const char * const u = units[i];
            size_t ulen = strlen(u);
            size_t llen = strlen(line);

            if (llen >= ulen && !strcmp (line + llen - ulen, u)) {
               line[llen - ulen] = '\n';
               line[llen+1 - ulen] = '\0';
               break;
            }
         }
      }
      fputs(line, stdout);
      // If we had a param to match and we got this far we must have
      // matched it. Clear par to indicate success and bail out.
      if (par) {
         par = NULL;
         break;
      }
   }

   if (n < 0) {
      fprintf(stderr, "Error reading status from apcupsd @ %s:%d: %s\n",
         host, port, strerror(-n));
      net_close(sockfd);
      return 1;
   }

   net_close(sockfd);
   return par ? 2 : 0;
}

/*********************************************************************/

#if defined(HAVE_MINGW)
#undef main
#endif

void usage()
{
   fprintf(stderr, 
      "Usage: apcaccess [-f <config-file>] [-h <host>[:<port>]] "
                       "[-p <parameter-name>] [-u] [<command>] [<host>[:<port>]]\n"
      "\n"
      " -f  Load default host,port from given conf file (default: %s)\n"
      " -h  Connect to host and port (supercedes conf file)\n"
      " -p  Return only the value of the named parameter rather than all parameters and values\n"
      " -u  Strip unit labels\n"
      "\n"
      "Supported commands: 'status' (default)\n"
      "Trailing host/port spec overrides -h and conf file.\n"
      , APCCONF);
}

int main(int argc, char **argv)
{
   const char *par = NULL;
   char *cfgfile = NULL;
   char DEFAULT_HOST[] = "localhost";
   char *host = NULL;
   const char *cmd = "status";
   int port = NISPORT;
   int flags = 0;
   FILE *cfg;
   UPSINFO ups;

   // Process standard options
   int ch;
   while ((ch = getopt(argc, argv, "f:h:p:u")) != -1)
   {
      switch (ch)
      {
      case 'f':
         cfgfile = optarg;
         break;
      case 'h':
         host = optarg;
         break;
      case 'p':
         par = optarg;
         break;
      case 'u':
         flags |= NO_UNITS;
         break;
      case '?':
      default:
         usage();
         return 1;
      }
   }

   // Default cfgfile if not provided on command line
   // Remember if we defaulted so we know later if conf failure is fatal
   bool fatal = cfgfile != NULL;
   if (!cfgfile)
      cfgfile = APCCONF;

   // Parse conf file for defaults
   if ((cfg = fopen(cfgfile, "r")))
   {
      fclose(cfg);
      memset(&ups, 0, sizeof(UPSINFO));
      init_ups_struct(&ups);
      check_for_config(&ups, cfgfile);
      port = ups.statusport;
      if (!host) // Don't override command line -h
         host = ups.nisip;
   }
   else if (fatal)
   {
      // Failure to find explicitly specified conf file is fatal
      fprintf(stderr, "Unable to open config file '%s'\n", cfgfile);
      return 2;
   }

   // Remaining non-option arguments are optional command and host:port
   // These are from legacy apcaccess syntax.  They take priority over
   // the default from the conf file and/or switch values.
   int optleft = argc - optind;
   if (optleft >= 1)
      cmd = argv[optind];
   if (optleft >= 2)
      host = argv[optind + 1];

   // If still no host, use default
   if (!host || !*host)
      host = DEFAULT_HOST;

   // Separate host and port
   char *p = strchr(host, ':');
   if (p) {
      *p++ = 0;
      port = atoi(p);
   }

   // Translate host of 0.0.0.0 to localhost
   // This is due to NISIP in apcupsd.conf being 0.0.0.0 for listening on all
   // interfaces. In that case just use loopback.
   if (!strcmp(host, "0.0.0.0"))
      host = DEFAULT_HOST;

   if (!strcmp(cmd, "status"))
   {
      return do_pthreads_status(host, port, par, flags);
   }
   else
   {
      fprintf(stderr, "Unknown command %s\n", cmd);
      usage();
      return 1;
   }
}
