/*
 * apcconfig.c
 *
 * Config file parser.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 * Copyright (C) Jonathan H N Chin <jc254@newton.cam.ac.uk>
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

/*
 * Global variables -- note, we really should have a separate
 * global file for these. For example, apcglobals.c.  
 */
int slave_count = 0;
SLAVEINFO slaves[MAXSLAVES];       /* Slaves description */

/* ---------------------------------------------------------------------- */

static HANDLER match_int, match_range, match_slave, match_str,
   match_facility, match_index;
static HANDLER obsolete;

#ifdef UNSUPPORTED_CODE
static HANDLER start_ups, end_ups;
#endif

/* ---------------------------------------------------------------------- */

static const GENINFO accesses[] = {
   { "false", "false",                 FALSE }, /* must come first */
   { "true",  "true",                  TRUE },
   { NULL,    "*invalid-access-mode*", FALSE },
};

static const GENINFO onoroff[] = {
   { "off", "off",                     FALSE },
   { "on",  "on",                      TRUE },
   { NULL,  "value must be ON or OFF", FALSE },
};

static const GENINFO cables[] = {
   { "simple",         "Custom Cable Simple",  CUSTOM_SIMPLE },
   { "smart",          "Custom Cable Smart",   CUSTOM_SMART },
   { "ether",          "Ethernet Link",        APC_NET },
   { "940-0119A",      "APC Cable 940-0119A",  APC_940_0119A },
   { "940-0127A",      "APC Cable 940-0127A",  APC_940_0127A },
   { "940-0128A",      "APC Cable 940-0128A",  APC_940_0128A },
   { "940-0020B",      "APC Cable 940-0020B",  APC_940_0020B },
   { "940-0020C",      "APC Cable 940-0020C",  APC_940_0020C },
   { "940-0023A",      "APC Cable 940-0023A",  APC_940_0023A },
   { "940-0024B",      "APC Cable 940-0024B",  APC_940_0024B },
   { "940-0024C",      "APC Cable 940-0024C",  APC_940_0024C },
   { "940-1524C",      "APC Cable 940-1524C",  APC_940_1524C },
   { "940-0024G",      "APC Cable 940-0024G",  APC_940_0024G },
   { "940-0095A",      "APC Cable 940-0095A",  APC_940_0095A },
   { "940-0095B",      "APC Cable 940-0095B",  APC_940_0095B },
   { "940-0095C",      "APC Cable 940-0095C",  APC_940_0095C },
   { "MAM-04-02-2000", "MAM Cable 04-02-2000", MAM_CABLE},
   { "usb",            "USB Cable",            USB_CABLE     },
   { NULL,             "*invalid-cable*",      NO_CABLE  },
};

static const GENINFO upsclasses[] = {
   { "standalone",     "Stand Alone",           STANDALONE },
   { "shareslave",     "ShareUPS Slave",        SHARESLAVE },
   { "sharemaster",    "ShareUPS Master",       SHAREMASTER },
   { "netslave",       "Net Slave",             NETSLAVE },
   { "netmaster",      "Net Master",            NETMASTER },
   { "sharenetmaster", "ShareUPS & Net Master", SHARENETMASTER },
   { NULL,             "*invalid-ups-class*",   NO_CLASS },
};

static const GENINFO logins[] = {
   { "always",  "always",               ALWAYS }, /* must come first */
   { "disable", "disable",              NEVER },
   { "timeout", "timeout",              TIMEOUT },
   { "percent", "percent",              PERCENT },
   { "minutes", "minutes",              MINUTES },
   { NULL,      "*invalid-login-mode*", NO_LOGON },
};

static const GENINFO modes[] = {
   { "disable",  "Network & ShareUPS Disabled", DISABLE },
   { "share",    "ShareUPS",                    SHARE },
   { "net",      "NetworkUPS",                  NET },
   { "sharenet", "Network & ShareUPS",          SHARENET },
   { NULL,       "*invalid-ups-mode*",          NO_SHARE_NET },
};

static const GENINFO types[] = {
   /* FIXME (adk): It has been long enough...time to kill these */
   { "backups",       "BackUPS",                   BK },
   { "sharebasic",    "ShareUPS Basic Port",       SHAREBASIC },
   { "netups",        "NetUPS Virtual Basic Port", NETUPS },
   { "backupspro",    "BackUPS Pro",               BKPRO },
   { "smartvsups",    "SmartUPS VS",               VS },
   { "newbackupspro", "Smarter BackUPS Pro",       NBKPRO },
   { "backupspropnp", "Smarter BackUPS Pro",       NBKPRO },
   { "smartups",      "SmartUPS",                  SMART },
   { "matrixups",     "MatrixUPS",                 MATRIX },
   { "sharesmart",    "ShareUPS Advanced Port",    SHARESMART },

   /*
    * Below are the new "drivers" entries. Entries above with time (long)
    * will go away.
    */
   { "dumb",     "DUMB UPS Driver",     DUMB_UPS },
   { "apcsmart", "APC Smart UPS (any)", APCSMART_UPS },
   { "usb",      "USB UPS Driver",      USB_UPS },
   { "snmp",     "SNMP UPS Driver",     SNMP_UPS },
   { "net",      "NETWORK UPS Driver",  NETWORK_UPS },
   { "test",     "TEST UPS Driver",     TEST_UPS },
   { NULL,       "*invalid-ups-type*",  NO_UPS },
};

static const PAIRS table[] = {

   /* General parameters */

#ifdef UNSUPPORTED_CODE
   {"UPSSTART",     start_ups,   WHERE(upsname),      SIZE(upsname),
      "UPS configuration id: start entry"},
   {"UPSEND",       end_ups,     0,                   0,
      "UPS configuration id: end entry"},
   {"POWEREDBYUPS", match_index, WHERE(PoweredByUPS), onoroff,
      "This computer is power by the UPS, there must be only one"},
#endif

   {"UPSNAME",  match_str,   WHERE(upsname),  SIZE(upsname),
      "UPS name"},
   {"UPSCABLE", match_range, WHERE(cable),    cables,
      "UPS cable type"},
   {"UPSTYPE",  match_range, WHERE(mode),     types,
      "UPS type"},
   {"DEVICE",   match_str,   WHERE(device),   SIZE(device),
      "Serial device to which the UPS is attached"},
   {"LOCKFILE", match_str,   WHERE(lockpath), SIZE(lockpath),
      "Lock file directory"},

   /* Configuration parameters used during power failures */
   {"ANNOY",          match_int,   WHERE(annoy),       0,
      "Time in seconds between messages requesting users to logoff"},
   {"ANNOYDELAY",     match_int,   WHERE(annoydelay),  0,
      "Initial delay in seconds before telling users to get off the system"},
   {"ONBATTERYDELAY", match_int,   WHERE(onbattdelay), 0,
      "Initial delay in seconds before reacting to a power failure"},
   {"TIMEOUT",        match_int,   WHERE(maxtime),     0,
      "Max. time in seconds to run on batteries"},
   {"NOLOGON",        match_range, WHERE(nologin),     logins,
      "Nologin policy"},
   {"BATTERYLEVEL",   match_int,   WHERE(percent),     0,
      "Min. battery percentage when to shutdown"},
   {"MINUTES",        match_int,   WHERE(runtime),     0,
      "Min. battery time in seconds remaining when to shutdown"},
   {"KILLDELAY",      match_int,   WHERE(killdelay),   0,
      "Delay in seconds after power failure before shutting down UPS power"},

   /* Configuration parmeters for network information server */
   {"NETSTATUS",  match_index, WHERE(netstats),   onoroff,
      "Send status information over the network"},      /* to be deleted */
   {"NETSERVER",  match_index, WHERE(netstats),   onoroff,
      "Become a server for STATUS and EVENTS data on the network"},
   {"NISIP",      match_str,   WHERE(nisip),      SIZE(nisip),
      "TCP IP for NIS communications"},
   {"NISPORT",    match_int,   WHERE(statusport), 0,
      "TCP port for Network Information Server communications"},
   {"SERVERPORT", match_int,   WHERE(statusport), 0,
      "TCP port for NIS communications"},

   /* Configuration parameters for event logging */
   {"EVENTFILE",     match_str, WHERE(eventfile),    SIZE(eventfile),
      "Location of temporary events file"},
   {"EVENTSFILE",    match_str, WHERE(eventfile),    SIZE(eventfile),
      "Location of temporary events file"},
   {"EVENTFILEMAX",  match_int, WHERE(eventfilemax), 0,
      "Maximum size of the events file in kilobytes"},
   {"EVENTSFILEMAX", match_int, WHERE(eventfilemax), 0,
      "Maximum size of the events file in kilobytes"},

   /* Configuration parameters to control system logging */
   {"FACILITY", match_facility, 0,               0,
      "log_event facility"},
   {"STATFILE", match_str,      WHERE(statfile), SIZE(statfile),
      "Location of status file"},
   {"LOGSTATS", match_index,    WHERE(logstats), onoroff,
      "Log status information"},
   {"STATTIME", match_int,      WHERE(stattime), 0,
      "Time between status file updates"},
   {"DATATIME", match_int,      WHERE(datatime), 0,
      "Time between syslog logging events (0=disable"},

   /* Values used to set UPS EPROM for --configure */
   {"SELFTEST",     match_str, WHERE(selftest),         SIZE(selftest),
      "Define hours between automatic self tests"},
   {"HITRANSFER",   match_int, WHERE(hitrans),          0,
      "High voltage transfer to UPS batteries"},
   {"LOTRANSFER",   match_int, WHERE(lotrans),          0,
      "Low voltage transfer to UPS batteries"},
   {"LOWBATT",      match_int, WHERE(dlowbatt),         0,
      "Low battery warning in minutes"},
   {"WAKEUP",       match_int, WHERE(dwake),            0,
      "Wake up delay in seconds"},
   {"RETURNCHARGE", match_int, WHERE(rtnpct),           0,
      "Percent charge required after power fail to return online"},
   {"OUTPUTVOLTS",  match_int, WHERE(NomOutputVoltage), 0,
      "Output Voltage when on batteries"},
   {"SLEEP",        match_int, WHERE(dshutd),           0,
      "Shutdown delay in seconds"},
   {"BEEPSTATE",    match_str, WHERE(beepstate),        SIZE(beepstate),
      "When to sound alarm after power failure"},
   {"BATTDATE",     match_str, WHERE(battdat),          SIZE(battdat),
      "Date of last battery replacement"},
   {"SENSITIVITY",  match_str, WHERE(sensitivity),      SIZE(sensitivity), 
      "Sensitivity of UPS to line voltage fluxuations"},

   /* Configuration statements for network sharing of the UPS */
   {"MASTER",    match_str,   WHERE(master_name), SIZE(master_name),
      "Master network machine"},
   {"USERMAGIC", match_str,   WHERE(usermagic),   SIZE(usermagic),
      "Id string for network security"},
   {"UPSCLASS",  match_range, WHERE(upsclass),    upsclasses,
      "UPS class"},
   {"UPSMODE",   match_range, WHERE(sharenet),    modes,
      "UPS mode"},
   {"SLAVE",     match_slave, 0,                  0,
      "Slave network machine"},
   {"NETPORT",   match_int,   WHERE(NetUpsPort),  0,
      "TCP socket for netups communications"},
   {"NETTIME",   match_int,   WHERE(nettime),     0,
      "Time between network updates"},

   /*
    * FIXME (adk): These look totally broken; they should just be removed.
    *
    * Obsolete configuration options: to be removed in the future.
    * The warning string is passed into the GENINFO *field since it is
    * not used any more for obsoleted options: we are only interested in
    * printing the message.
    * There is a new meaning for offset field too. If TRUE will bail out,
    * if FALSE it will continue to run apcupsd. This way we can bail out
    * if an obsolete option is too sensible to continue running apcupsd.
    * -RF
    *
    * An example entry may be:
    *
    *   { "CONTROL",   obsolete, TRUE, (GENINFO *)
    *      "CONTROL config option is obsolete, use /usr/lib/apcupsd/ "
    *      "scripts instead." },
    */
   {"CONTROL", obsolete, TRUE,
      (GENINFO *)"CONTROL config directive is obsolete"},

   {"NETACCESS", match_range, WHERE(enable_access), accesses,
      "NETACCESS config directive is obsolete"},

   /* must be last */
   {NULL, 0, 0, 0, NULL}
};

void print_pairs_table(void)
{
   int i;

   printf("Valid configuration directives:\n\n");

   for (i = 0; table[i].key != NULL; i++)
      printf("%-16s%s\n", table[i].key, table[i].help);
}

static int obsolete(UPSINFO *ups, int offset, const GENINFO * junk, const char *v)
{
   char *msg = (char *)junk;

   fprintf(stderr, "%s\n", msg);
   fprintf(stderr, _("error ignored.\n"));
   return SUCCESS;
}

#ifdef UNSUPPORTED_CODE
static int start_ups(UPSINFO *ups, int offset, const GENINFO * size, const char *v)
{
   char x[MAXSTRING];

   if (!sscanf(v, "%s", x))
      return FAILURE;

   /* Verify that we don't already have an UPS with the same name. */
   for (ups = NULL; (ups = getNextUps(ups)) != NULL;)
      if (strncmp(x, ups->upsname, UPSNAMELEN) == 0) {
         fprintf(stderr, _("%s: duplicate upsname [%s] in config file.\n"),
            argvalue, ups->upsname);
         return FAILURE;
      }

   /* Here start the definition of a new UPS to add to the linked list. */
   ups = new_ups();

   if (ups == NULL) {
      Error_abort1(_("%s: not enough memory.\n"), argvalue);
      return FAILURE;
   }

   init_ups_struct(ups);


   astrncpy((char *)AT(ups, offset), x, (int)size);

   /* Terminate string */
   *((char *)AT(ups, (offset + (int)size) - 1)) = 0;
   return SUCCESS;
}

static int end_ups(UPSINFO *ups, int offset, const GENINFO * size, const char *v)
{
   char x[MAXSTRING];

   if (!sscanf(v, "%s", x))
      return FAILURE;

   if (ups == NULL) {
      fprintf(stderr, _("%s: upsname [%s] mismatch in config file.\n"), argvalue, x);
      return FAILURE;
   }

   if (strncmp(x, ups->upsname, UPSNAMELEN) != 0) {
      fprintf(stderr, _("%s: upsname [%s] mismatch in config file.\n"),
         argvalue, ups->upsname);
      return FAILURE;
   }

   insertUps(ups);

   return SUCCESS;
}
#endif


static int match_int(UPSINFO *ups, int offset, const GENINFO * junk, const char *v)
{
   int x;

   if (sscanf(v, "%d", &x)) {
      *(int *)AT(ups, offset) = x;
      return SUCCESS;
   }
   return FAILURE;
}

static int match_range(UPSINFO *ups, int offset, const GENINFO * vs, const char *v)
{
   char x[MAXSTRING];
   INTERNALGENINFO *t;

   if (!vs) {
      /* Shouldn't ever happen so abort if it ever does. */
      Error_abort1(_("%s: Bogus configuration table! Fix and recompile.\n"),
         argvalue);
   }

   if (!sscanf(v, "%s", x))
      return FAILURE;

   for (; vs->name; vs++)
      if (!strcmp(x, vs->name))
         break;

   t = (INTERNALGENINFO *) AT(ups, offset);

   /* Copy the structure */
   if (vs->name == NULL) {
      /*
       * A NULL here means that the config value is bogus, so
       * print error message (and log it).
       */
      log_event(ups, LOG_WARNING,
         _("%s: Bogus configuration value (%s)\n"), argvalue, vs->long_name);
      fprintf(stderr,
         _("%s: Bogus configuration value (%s)\n"), argvalue, vs->long_name);
      return FAILURE;

   }

   /*
    * THIS IS VERY UGLY. If some idiot like myself doesn't
    * know enough to define all the appropriate variables
    * with all the correct ordering and sizes, this will
    * overwrite memory.
    */
   t->type = vs->type;
   astrncpy(t->name, vs->name, sizeof(t->name));
   astrncpy(t->long_name, vs->long_name, sizeof(t->long_name));
   return SUCCESS;
}

/*
 * Similar to match range except that it only returns the index of the
 * item.
 */
static int match_index(UPSINFO *ups, int offset, const GENINFO * vs, const char *v)
{
   char x[MAXSTRING];

   if (!vs) {
      /* Shouldn't ever happen so abort if it ever does. */
      Error_abort1(_("%s: Bogus configuration table! Fix and recompile.\n"),
         argvalue);
   }

   if (!sscanf(v, "%s", x))
      return FAILURE;

   for (; vs->name; vs++)
      if (!strcmp(x, vs->name))
         break;

   if (vs->name == NULL) {
      /*
       * A NULL here means that the config value is bogus, so
       * print error message (and log it).
       */
      log_event(ups, LOG_WARNING,
         _("%s: Bogus configuration value (%s)\n"), argvalue, vs->long_name);
      fprintf(stderr,
         _("%s: Bogus configuration value (%s)\n"), argvalue, vs->long_name);
      return FAILURE;

   }
   /* put it in ups buffer */
   *(int *)AT(ups, offset) = vs->type;
   return SUCCESS;
}


static int match_slave(UPSINFO *ups, int offset,
   const GENINFO *junk, const char *v)
{
   char x[MAXSTRING];

   if (!sscanf(v, "%s", x))
      return FAILURE;

   if (slave_count >= MAXSLAVES) {
      fprintf(stderr, _("%s: Exceeded max slaves number (%d)\n"),
         argvalue, MAXSLAVES);
      return FAILURE;
   }

   astrncpy(slaves[slave_count].name, x, sizeof(slaves[0].name));
   slave_count++;

   return SUCCESS;
}

/*
 * FIXME (remove/replace this comment once fixed)
 *
 * Do we ever want str to contain whitespace?
 * If so, we can't use sscanf(3)
 */
static int match_str(UPSINFO *ups, int offset, const GENINFO * gen, const char *v)
{
   char x[MAXSTRING];
   long size = (long)gen;

   /*
    * Needed if string is empty or all whitespace; sscanf will return EOF 
    * without modifying the destination buffer.
    */
   x[0] = '\0';

   if (!sscanf(v, "%s", x))
      return FAILURE;

   astrncpy((char *)AT(ups, offset), x, (int)size);
   *((char *)AT(ups, (offset + (int)size) - 1)) = 0;    /* terminate string */
   return SUCCESS;
}

static int match_facility(UPSINFO *ups, int offset,
   const GENINFO *junk, const char *v)
{
   const struct {
      char *fn;
      int fi;
   } facnames[] = {
      {"daemon", LOG_DAEMON},
      {"local0", LOG_LOCAL0},
      {"local1", LOG_LOCAL1},
      {"local2", LOG_LOCAL2},
      {"local3", LOG_LOCAL3},
      {"local4", LOG_LOCAL4},
      {"local5", LOG_LOCAL5},
      {"local6", LOG_LOCAL6},
      {"local7", LOG_LOCAL7},
      {"lpr", LOG_LPR},
      {"mail", LOG_MAIL},
      {"news", LOG_NEWS},
      {"uucp", LOG_UUCP},
      {"user", LOG_USER},
      {NULL, -1}
   };
   int i;
   char x[MAXSTRING];
   int oldfac;

   if (!sscanf(v, "%s", x))
      return FAILURE;

   oldfac = ups->sysfac;
   for (i = 0; facnames[i].fn; i++) {
      if (!(strcasecmp(facnames[i].fn, x))) {
         ups->sysfac = facnames[i].fi;
         /* if it changed, close log file and reopen it */
         if (ups->sysfac != oldfac) {
            closelog();
            openlog("apcupsd", LOG_CONS | LOG_PID, ups->sysfac);
         }
         return SUCCESS;
      }
   }

   return FAILURE;
}

/* ---------------------------------------------------------------------- */

/**
 ** This function has been so buggy, I thought commentary was needed.
 ** Be careful about changing anything. Make sure you understand what is
 ** going on first. Even the people who wrote it kept messing it up... 8^)
 **/
static int ParseConfig(UPSINFO *ups, char *line)
{
   const PAIRS *p;
   char *key, *value;

/**
 ** Hopefully my notation is obvious enough:
 **   (a) : a
 **   a|b : a or b
 **    a? : zero or one a
 **    a* : zero or more a
 **    a+ : one or more a
 **
 ** On entry, situation is expected to be:
 **
 ** line  = WS* ((KEY (WS+ VALUE)* WS* (WS+ COMMENT)?) | COMMENT?) (EOL|EOF)
 ** key   = undef
 ** value = undef
 **
 ** if line is not of this form we will eventually return an error
 ** line, key, value point to null-terminated strings
 ** EOF may be end of file, or if it occurs alone signifies null string
 **
 **/
   /* skip initial whitespace */
   for (key = line; *key && isspace(*key); key++)
      ;

/**
 ** key   = ((KEY (WS+ VALUE)* WS* (WS+ COMMENT)?) | COMMENT?) (EOL|EOF)
 ** value = undef
 **/
   /* catch EOF (might actually be only an empty line) and comments */
   if (!*key || (*key == '#'))
      return (SUCCESS);

/**
 ** key   = (KEY (WS+ VALUE)* WS* (WS+ COMMENT)? (EOL|EOF)
 ** value = undef
 **/
   /* convert key to UPPERCASE */
   for (value = key; *value && !isspace(*value); value++)
      *value = toupper(*value);

/**
 ** key   = KEY (WS+ VALUE)* WS* (WS+ COMMENT)? (EOL|EOF)
 ** value = (WS+ VALUE)* WS* (WS+ COMMENT)? (EOL|EOF)
 **/
   /* null out whitespace in the string */
   for (; *value && isspace(*value); value++)
      *value = '\0';

/**
 ** key   = KEY
 ** value = (VALUE (WS+ VALUE)* WS* (WS+ COMMENT)? (EOL|EOF))
 **          | (COMMENT (EOL|EOF))
 **          | EOF
 **
 ** key is now fully `normalised' (no leading or trailing garbage)
 ** value contains zero or more whitespace delimited elements and there
 ** may be a trailing comment.
 **/
   /* look for key in table */
   for (p = table; p->key && strcmp(p->key, key); p++)
      ;

/**
 ** It is *not* the responsibility of a dispatcher (ie. ParseConfig())
 ** to parse `value'.
 ** Parsing `value' is the responsibility of the handler (ie. p->handler()).
 ** (Although one could conceive of the case where a ParseConfig()-alike
 ** function were invoked recursively as the handler...!)
 **
 ** Currently all the handler functions (match_int(), match_str(), etc)
 ** just use sscanf() to pull out the bits they want out of `value'.
 **
 ** In particular, if "%s" is used, leading/trailing whitespace
 ** is automatically removed.
 **
 ** In addition, unused bits of value are simply discarded if the handler
 ** does not use them. So trailing garbage on lines is unimportant.
 **/
   if (p->key) {
      if (p->handler)
         return (p->handler(ups, p->offset, p->values, value));
      else
         return (SUCCESS);
   } else {
      return (FAILURE);
   }
}

/* ---------------------------------------------------------------------- */

#define BUF_SIZE 1000

/*
 * Setup general defaults for the ups structure.
 * N.B. Do not zero the structure because it already has
 *      pthreads sturctures or shared memory/semaphore
 *      structures initialized.
 */
void init_ups_struct(UPSINFO *ups)
{
   ups->buf = (char *)malloc(BUF_SIZE);
   if (ups->buf)
      ups->buf_len = BUF_SIZE;

   /* put some basic information for sanity checks */
   astrncpy(ups->id, UPSINFO_ID, sizeof(ups->id));
   ups->version = UPSINFO_VERSION;
   ups->size = sizeof(UPSINFO);
   astrncpy(ups->release, APCUPSD_RELEASE, sizeof(ups->release));

   ups->fd = -1;

   set_ups(UPS_PLUGGED);

   astrncpy(ups->enable_access.name, accesses[0].name,
      sizeof(ups->enable_access.name));
   astrncpy(ups->enable_access.long_name, accesses[0].long_name,
      sizeof(ups->enable_access.long_name));
   ups->enable_access.type = accesses[0].type;

   astrncpy(ups->nologin.name, logins[0].name, sizeof(ups->nologin.name));
   astrncpy(ups->nologin.long_name, logins[0].long_name,
      sizeof(ups->nologin.long_name));
   ups->nologin.type = logins[0].type;

   ups->annoy = 5 * 60;            /* annoy every 5 mins */
   ups->annoydelay = 60;           /* must be > than annoy to work, why???? */
   ups->onbattdelay = 6;
   ups->maxtime = 0;
   ups->nologin_time = 0;
   ups->nologin_file = FALSE;

   ups->stattime = 0;
   ups->datatime = 0;
   ups->reports = FALSE;
   ups->nettime = 60;
   ups->percent = 10;
   ups->runtime = 5;
   ups->netstats = TRUE;
   ups->statusport = 7000;
   ups->upsmodel[0] = 0;           /* end of string */


   /* EPROM values that can be changed with config directives */

   astrncpy(ups->sensitivity, "-1", sizeof(ups->sensitivity));  /* no value */
   ups->dwake = -1;
   ups->dshutd = -1;
   astrncpy(ups->selftest, "-1", sizeof(ups->selftest));        /* no value */
   ups->lotrans = -1;
   ups->hitrans = -1;
   ups->rtnpct = -1;
   ups->dlowbatt = -1;
   ups->NomOutputVoltage = -1;
   astrncpy(ups->beepstate, "-1", sizeof(ups->beepstate));      /* no value */

   ups->nisip[0] = 0;              /* no nis IP file as default */
   ups->NetUpsPort = 0;

   ups->lockfile = -1;

   clear_ups(UPS_SHUT_LOAD);
   clear_ups(UPS_SHUT_BTIME);
   clear_ups(UPS_SHUT_LTIME);
   clear_ups(UPS_SHUT_EMERG);
   clear_ups(UPS_SHUT_REMOTE);
   ups->remote_state = TRUE;

   ups->sysfac = LOG_DAEMON;

   ups->statfile[0] = 0;           /* no stats file default */
   ups->eventfile[0] = 0;          /* no events file as default */
   ups->eventfilemax = 10;         /* trim the events file at 10K as default */
   ups->event_fd = -1;             /* no file open */

   /* Initialize UPS function codes */
   ups->UPS_Cmd[CI_STATUS] = APC_CMD_STATUS;
   ups->UPS_Cmd[CI_LQUAL] = APC_CMD_LQUAL;
   ups->UPS_Cmd[CI_WHY_BATT] = APC_CMD_WHY_BATT;
   ups->UPS_Cmd[CI_ST_STAT] = APC_CMD_ST_STAT;
   ups->UPS_Cmd[CI_VLINE] = APC_CMD_VLINE;
   ups->UPS_Cmd[CI_VMAX] = APC_CMD_VMAX;
   ups->UPS_Cmd[CI_VMIN] = APC_CMD_VMIN;
   ups->UPS_Cmd[CI_VOUT] = APC_CMD_VOUT;
   ups->UPS_Cmd[CI_BATTLEV] = APC_CMD_BATTLEV;
   ups->UPS_Cmd[CI_VBATT] = APC_CMD_VBATT;
   ups->UPS_Cmd[CI_LOAD] = APC_CMD_LOAD;
   ups->UPS_Cmd[CI_FREQ] = APC_CMD_FREQ;
   ups->UPS_Cmd[CI_RUNTIM] = APC_CMD_RUNTIM;
   ups->UPS_Cmd[CI_ITEMP] = APC_CMD_ITEMP;
   ups->UPS_Cmd[CI_DIPSW] = APC_CMD_DIPSW;
   ups->UPS_Cmd[CI_SENS] = APC_CMD_SENS;
   ups->UPS_Cmd[CI_DWAKE] = APC_CMD_DWAKE;
   ups->UPS_Cmd[CI_DSHUTD] = APC_CMD_DSHUTD;
   ups->UPS_Cmd[CI_LTRANS] = APC_CMD_LTRANS;
   ups->UPS_Cmd[CI_HTRANS] = APC_CMD_HTRANS;
   ups->UPS_Cmd[CI_RETPCT] = APC_CMD_RETPCT;
   ups->UPS_Cmd[CI_DALARM] = APC_CMD_DALARM;
   ups->UPS_Cmd[CI_DLBATT] = APC_CMD_DLBATT;
   ups->UPS_Cmd[CI_IDEN] = APC_CMD_IDEN;
   ups->UPS_Cmd[CI_STESTI] = APC_CMD_STESTI;
   ups->UPS_Cmd[CI_MANDAT] = APC_CMD_MANDAT;
   ups->UPS_Cmd[CI_SERNO] = APC_CMD_SERNO;
   ups->UPS_Cmd[CI_BATTDAT] = APC_CMD_BATTDAT;
   ups->UPS_Cmd[CI_NOMBATTV] = APC_CMD_NOMBATTV;
   ups->UPS_Cmd[CI_HUMID] = APC_CMD_HUMID;
   ups->UPS_Cmd[CI_REVNO] = APC_CMD_REVNO;
   ups->UPS_Cmd[CI_REG1] = APC_CMD_REG1;
   ups->UPS_Cmd[CI_REG2] = APC_CMD_REG2;
   ups->UPS_Cmd[CI_REG3] = APC_CMD_REG3;
   ups->UPS_Cmd[CI_EXTBATTS] = APC_CMD_EXTBATTS;
   ups->UPS_Cmd[CI_ATEMP] = APC_CMD_ATEMP;
   ups->UPS_Cmd[CI_UPSMODEL] = APC_CMD_UPSMODEL;
   ups->UPS_Cmd[CI_NOMOUTV] = APC_CMD_NOMOUTV;
   ups->UPS_Cmd[CI_BADBATTS] = APC_CMD_BADBATTS;
   ups->UPS_Cmd[CI_EPROM] = APC_CMD_EPROM;
   ups->UPS_Cmd[CI_ST_TIME] = APC_CMD_ST_TIME;
   ups->UPS_Cmd[CI_CYCLE_EPROM] = APC_CMD_CYCLE_EPROM;
   ups->UPS_Cmd[CI_UPS_CAPS] = APC_CMD_UPS_CAPS;
}

void check_for_config(UPSINFO *ups, char *cfgfile)
{
   FILE *apcconf;
   char line[MAXSTRING];
   int errors = 0;
   int erpos = 0;

   if ((apcconf = fopen(cfgfile, "r")) == NULL) {
      Error_abort2(_("Error opening configuration file (%s): %s\n"),
         cfgfile, strerror(errno));
   }
   astrncpy(ups->configfile, cfgfile, sizeof(ups->configfile));

   /* Check configuration file format is a suitable version */
   if (fgets(line, sizeof(line), apcconf) != NULL) {
      /*
       * The -1 in sizeof is there because of the last character
       * to be checked. In line is '\n' and in APC... is '\0'.
       * They never match so don't even compare them.
       */
      if (strncmp(line, APC_CONFIG_MAGIC, sizeof(APC_CONFIG_MAGIC) - 1) != 0) {
         fprintf(stderr, _("%s: Warning: old configuration file found.\n\n"
               "%s: Expected: \"%s\"\n"
               "%s: Found:    \"%s\"\n\n"
               "%s: Please check new file format and\n"
               "%s: modify accordingly the first line\n"
               "%s: of config file.\n\n"
               "%s: Processing config file anyway.\n"),
            argvalue,
            argvalue, APC_CONFIG_MAGIC,
            argvalue, line, argvalue, argvalue, argvalue, argvalue);
      }

      /*
       * Here we have read alredy the first line of configuration
       * so there may be the case where the first line is a config
       * parameter and we must not discard it reading another line.
       * Jump into the reading configuration loop.
       */
      goto jump_into_the_loop;
   }

   while (fgets(line, sizeof(line), apcconf) != NULL) {
jump_into_the_loop:
      erpos++;

      if (ParseConfig(ups, line)) {
         errors++;
         printf("%s\n", line);
         printf(_("Parsing error at line %d of config file %s.\n"), erpos, cfgfile);
      }
   }

   fclose(apcconf);

   /*
    * The next step will need a good ups struct.
    * Of course if here we have errors, the apc struct is not good
    * so don't bother to post-process it.
    */
   if (errors)
      goto bail_out;

   /* post-process the configuration stored in the ups structure */

   if (ups->upsclass.type != NETSLAVE)
      ups->usermagic[0] = '\0';

   /*
    * If annoy time is greater than initial delay, don't bother about
    * initial delay and set it to 0.
    */
   if (ups->annoy >= ups->annoydelay)
      ups->annoydelay = 0;

   if ((ups->sharenet.type == SHAREMASTER) || (ups->sharenet.type == SHARENETMASTER)) {
      ups->maxtime = 0;
      ups->percent = 10;
      ups->runtime = 5;
   }

   if ((ups->cable.type < CUSTOM_SMART) && ups->mode.type >= BKPRO) {
      fprintf(stderr, _("%s: Error :: Changing UPSTYPE from %s "),
         argvalue, ups->mode.long_name);

      /* No errors expected from this operation. */
      errors += match_range(ups, WHERE(mode), types, "backups");

      fprintf(stderr, _("to %s due wrong Cable of Smart Signals.\n\a"),
         ups->mode.long_name);

      if (errors)
         Error_abort0(_("Lookup operation failed: bad 'types' table\n"));
   }

   /*
    * apcupsd _must_ have a lock file, mainly for security reasons.
    * If apcupsd is running and a lock is not there the admin could
    * mistakenly open a serial device with minicom for using it as a
    * modem or other device. Think about the implications of sending
    * extraneous characters to the UPS: a wrong char and the machine
    * can be powered down.
    *
    * No thanks.
    *
    * On the other hand if a lock file is there, minicom (like any
    * other serious serial program) will refuse to open the device.
    *
    * About "/var/lock//LCK..." nicety ... the right word IMHO is
    * simplify, so don't bother.
    */
   if (ups->lockpath[0] == '\0')
      astrncpy(ups->lockpath, LOCK_DEFAULT, sizeof(ups->lockpath));

   /* If APC_NET, the lockfile is not needed. */
   if (ups->cable.type != APC_NET) {
      char *dev = strrchr(ups->device, '/');

      astrncat(ups->lockpath, APC_LOCK_PREFIX, sizeof(ups->lockpath));
      astrncat(ups->lockpath, dev ? ++dev : ups->device, sizeof(ups->lockpath));
   } else {
      ups->lockpath[0] = 0;
      ups->lockfile = -1;
   }

   if ((slave_count > 0) && ups->master_name[0])
      error_exit(_("I can't be both MASTER and SLAVE\n"));

   switch (ups->nologin.type) {
   case TIMEOUT:
      if (ups->maxtime != 0)
         ups->nologin_time = (int)(ups->maxtime * 0.9);
      break;
   case PERCENT:
      ups->nologin_time = (int)(ups->percent * 1.1);
      if (ups->nologin_time == ups->percent)
         ups->nologin_time++;
      break;
   case MINUTES:
      ups->nologin_time = (int)(ups->runtime * 1.1);
      if (ups->nologin_time == ups->runtime)
         ups->nologin_time++;
      break;
   default:
      break;
   }

   if (ups->PoweredByUPS)
      set_ups(UPS_PLUGGED);
   else
      clear_ups(UPS_PLUGGED);

bail_out:
   if (errors)
      error_exit(_("Terminating due to configuration file errors.\n"));

   return;
}
