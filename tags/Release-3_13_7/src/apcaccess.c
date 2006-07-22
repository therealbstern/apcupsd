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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"

extern char *net_errmsg;

#define BUFFER_SIZE PIPE_BUF

struct passwd *pwentry;

char msg[100];
time_t nowtime;
char m_char;
char argvalue[MAXSTRING];
int shm_OK = 0;

/* Default values for contacting daemon */
static char *host = "localhost";
static int port = NISPORT;

struct s_mydata {
   char apcmagic[APC_MAGIC_SIZE];
   int update_master_config;
   int get_master_status;
   int slave_status;
   int call_master_shutdown;
   char accessmagic[ACCESS_MAGIC_SIZE];
} myDATA = {
   "\0",               /* data->apcmagic             */
   0,                  /* data->update_master_config */
   0,                  /* data->get_master_status    */
   -1,                 /* data->slave_status         */
   0,                  /* data->call_master_shutdown */
   "\0"                /* data->accessmagic          */
};

struct configinfo {
   int new_annoy;
   int new_maxtime;
   int new_delay;
#ifdef __NOLOGIN
   int new_nologin;
#endif  /* __NOLOGIN */
   int new_stattime;
   int new_datatime;
   int new_nettime;
   int new_percent;
} myCONFIG;

/*
 * EPROM commands and their values as parsed from the
 * ^Z eprom string returned by the UPS.
 */
static struct {
   char cmd;
   char size;
   char num;
   char cmdvals[50];
} cmd[15];

/* Total number of EPROM commands */
static int ncmd = 0;

UPSINFO eeprom_ups;
UPSINFO *core_ups = &eeprom_ups;

/*
 * Table of the UPS command, the apcupsd configuration directive,
 * and an explanation of what the command sets in the EPROM.
 */
static const struct {
   const char cmd;
   const char *config_directive;
   const char *descript;
   const char type;
   const int *data;
} cmd_table[] = {
   {'u', "HITRANSFER",   "Upper transfer voltage",  'i', &eeprom_ups.hitrans},
   {'l', "LOTRANSFER",   "Lower transfer voltage",  'i', &eeprom_ups.lotrans},
   {'e', "RETURNCHARGE", "Return threshold",        'i', &eeprom_ups.rtnpct},
   {'o', "OUTPUTVOLTS",  "Output voltage on batts", 'i', &eeprom_ups.NomOutputVoltage},
   {'s', "SENSITIVITY",  "Sensitivity",             'c', (int *)eeprom_ups.sensitivity},
   {'q', "LOWBATT",      "Low battery warning",     'i', &eeprom_ups.dlowbatt},
   {'p', "SLEEP",        "Shutdown grace delay",    'i', &eeprom_ups.dshutd},
   {'k', "BEEPSTATE",    "Alarm delay",             'c', (int *)eeprom_ups.beepstate},
   {'r', "WAKEUP",       "Wakeup delay",            'i', &eeprom_ups.dwake},
   {'E', "SELFTEST",     "Self test interval",      'c', (int *)eeprom_ups.selftest},
   {0,   NULL,           NULL,                      'x', NULL}  /* Last entry */
};

void print_valid_eprom_values(UPSINFO *ups)
{
   int i, j, k, l;
   char *p;
   char val[10];

   printf("\nValid EPROM values for the %s\n\n", ups->mode.long_name);

   memcpy(&eeprom_ups, ups, sizeof(UPSINFO));

   printf("%-24s %-12s  %-6s  %s\n", "           ",
      "Config", "Current",  "Permitted");
   printf("%-24s %-12s  %-6s  %s\n", "Description",
      "Directive", "Value  ", "Values");
   printf("===================================================================\n");

   for (i = 0; i < ncmd; i++) {
      for (j = 0; cmd_table[j].cmd; j++) {
         if (cmd[i].cmd == cmd_table[j].cmd) {
            if (cmd_table[j].type == 'c')
               asnprintf(val, sizeof(val), "%s", (char *)cmd_table[j].data);
            else
               asnprintf(val, sizeof(val), "%d", *cmd_table[j].data);

            printf("%-24s %-12s  %-6s   ", cmd_table[j].descript,
               cmd_table[j].config_directive, val);

            p = cmd[i].cmdvals;
            for (k = cmd[i].num; k; k--) {
               for (l = cmd[i].size; l; l--)
                  printf("%c", *p++);
               printf(" ");
            }

            printf("\n");
            break;
         }
      }
   }

   printf("\n");
}

/*
 * Parse EPROM command string returned by a ^Z command. We
 * pull out only entries that correspond to our UPS (locale).
 */
void parse_eprom_cmds(char *eprom, char locale)
{
   char *p = eprom;
   char c, l, n, s;

   for (;;) {
      c = *p++;

      if (c == 0)
         break;

      if (c == '#')
         continue;

      l = *p++;                    /* get locale */
      n = *p++ - '0';              /* get number of commands */
      s = *p++ - '0';              /* get character size */

      if (l != '4' && l != locale) {    /* skip this locale */
         p += n * s;
         continue;
      }

      cmd[ncmd].cmd = c;           /* store command */
      cmd[ncmd].size = s;          /* chare length of each value */
      cmd[ncmd].num = n;           /* number of values */

      strncpy(cmd[ncmd].cmdvals, p, n * s);     /* save values */
      p += n * s;
      ncmd++;
   }

#ifdef debuggggggg
   printf("\n");
   for (i = 0; i < ncmd; i++) {
      printf("cmd=%c len=%d nvals=%d vals=%s\n", cmd[i].cmd,
         cmd[i].size, cmd[i].num, cmd[i].cmdvals);
   }
#endif
}

static void get_raw_upsinfo(UPSINFO *ups, char *host, int port);

/*********************************************************************/
void do_eprom(UPSINFO *ups)
{
   char locale, locale1, locale2;

   get_raw_upsinfo(ups, host, port);

   if (!ups->UPS_Cap[CI_EPROM])
      Error_abort0("Your model does not support EPROM programming.\n");

   if (ups->UPS_Cap[CI_REVNO])
      locale1 = *(ups->firmrev + strlen(ups->firmrev) - 1);
   else
      locale1 = 0;

   if (ups->UPS_Cap[CI_UPSMODEL])
      locale2 = *(ups->upsmodel + strlen(ups->upsmodel) - 1);
   else
      locale2 = 0;

   if (locale1 == locale2 && locale1 == 0)
      Error_abort0("Your model does not support EPROM programming.\n");

   if (locale1 == locale2)
      locale = locale1;

   if (locale1 == 0)
      locale = locale2;
   else
      locale = locale1;

   parse_eprom_cmds(ups->eprom, locale);
   print_valid_eprom_values(ups);
}



/*
 * In a non-shared memory environment, we use the network
 * to get a raw upsinfo buffer.  This code does not support
 * mixing machines of different types.
 */
static void get_raw_upsinfo(UPSINFO *ups, char *host, int port)
{
   int sockfd, n;

   if ((sockfd = net_open(host, NULL, port)) < 0)
      Error_abort0(net_errmsg);

   net_send(sockfd, "rawupsinfo", 10);

   n = net_recv(sockfd, (char *)ups, sizeof(UPSINFO));
   if (n != sizeof(UPSINFO)) {
      Error_abort2("net_recv of UPSINFO returned %d bytes, wanted %d\n",
         n, sizeof(UPSINFO));
   }

   net_close(sockfd);
}

/* Get and print status from apcupsd NIS server */
static void do_pthreads_status(UPSINFO *ups, char *host, int port)
{
   int sockfd, n;
   char recvline[MAXSTRING + 1];

   if ((sockfd = net_open(host, NULL, port)) < 0)
      Error_abort0(net_errmsg);

   net_send(sockfd, "status", 6);

   while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) {
      recvline[n] = 0;
      fputs(recvline, stdout);
   }

   if (n < 0)
      Error_abort0(net_errmsg);

   net_close(sockfd);
}

/*********************************************************************/

#if defined(HAVE_CYGWIN) || defined(HAVE_MINGW)
#undef main
#endif

int main(int argc, char **argv)
{
   int mode = 0;
   UPSINFO *ups = NULL;

   char *cfgfile = APCCONF;
   struct stat cfgstat;

#ifdef HAVE_MINGW
   WSA_Init();                   /* init MS networking */
#endif

   astrncpy(argvalue, argv[0], sizeof(argvalue) - 1);

   ups = attach_ups(ups);
   if (!ups)
      Error_abort0(_("Cannot attach SYSV IPC.\n"));

   if (argc < 2) {
      /* Assume user wants "status" */
      mode = 2;
      myDATA.update_master_config = FALSE;
      myDATA.get_master_status = TRUE;
      myDATA.slave_status = -1;
      myDATA.call_master_shutdown = FALSE;
   } else {
      if (strcmp(argv[1], "reconfig") == 0) {
         mode = 1;
         myDATA.update_master_config = TRUE;
         myDATA.get_master_status = FALSE;
         myDATA.slave_status = -1;
         myDATA.call_master_shutdown = FALSE;
         Error_abort1(_("(ipc) <%s> : is not functional yet.\n"), argv[1]);
      } else if (strcmp(argv[1], "status") == 0) {
         mode = 2;
         myDATA.update_master_config = FALSE;
         myDATA.get_master_status = TRUE;
         myDATA.slave_status = -1;
         myDATA.call_master_shutdown = FALSE;
      } else if (strcmp(argv[1], "slave") == 0) {
         mode = 3;
         myDATA.update_master_config = FALSE;
         myDATA.get_master_status = FALSE;
         myDATA.slave_status = TRUE;
         myDATA.call_master_shutdown = FALSE;
         Error_abort1(_("(ipc) <%s> : is not functional yet.\n"), argv[1]);
      } else if (strcmp(argv[1], "shutdown") == 0) {
         mode = 4;
         myDATA.update_master_config = FALSE;
         myDATA.get_master_status = FALSE;
         myDATA.slave_status = -1;
         myDATA.call_master_shutdown = TRUE;
         fprintf(stderr, _("%s (ipc) <%s> : is not functional yet.\n"),
            argv[0], argv[1]);
      } else if (strcmp(argv[1], "eprom") == 0 || strcmp(argv[1], "eeprom") == 0) {
         mode = 5;
      } else {
         Error_abort1(_("Unknown command %s\n"), argv[1]);
      }
   }

   if (argc > 2) {                 /* assume host:port */
      char *p;

      host = argv[2];
      p = strchr(host, ':');
      if (p) {
         *p++ = 0;
         port = atoi(p);
      }
   } else {
      /* check configuration so local NISIP and NISPORT variables can be used as defaults */
      if (!stat(cfgfile, &cfgstat)) {
         check_for_config(ups, cfgfile);

         if (ups) {
            if (ups->nisip && ups->nisip[0])
               host = ups->nisip;

            if (ups->statusport)
               port = ups->statusport;
         }
      }
   }

   astrncpy(myDATA.apcmagic, APC_MAGIC, sizeof(myDATA.apcmagic));
   astrncpy(myDATA.accessmagic, ACCESS_MAGIC, sizeof(myDATA.accessmagic));

   if (!*host || strcmp(host, "0.0.0.0") == 0)
      host = "localhost";

   switch (mode) {
   case 1:
      break;

   case 2:       /* status */
      do_pthreads_status(ups, host, port);
      break;

   case 3:
      break;

   case 4:      /* shutdown */
      Error_abort0(_("shutdown not implemented.\n"));
      break;

   case 5:
      do_eprom(ups);
      break;

   default:
      Error_abort0(_("Strange mode\n"));
   }

   detach_ups(ups);

   return 0;
}
