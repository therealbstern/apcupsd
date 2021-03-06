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

char argvalue[MAXSTRING];

/*
 * We use more complicated defaults for these constants in some cases,
 * so stick them in arrays that can be modified at runtime.
 */
char APCCONF[APC_FILENAME_MAX] = SYSCONFDIR APCCONF_FILE;

/* ---------------------------------------------------------------------- */

typedef int (HANDLER) (UPSINFO *, int, const GENINFO *, const char *);

static HANDLER match_int, match_range, match_str;
static HANDLER match_facility, match_index;
static HANDLER obsolete;

#ifdef UNSUPPORTED_CODE
static HANDLER start_ups, end_ups;
#endif

/* ---------------------------------------------------------------------- */

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
   { NULL,       "*invalid-ups-mode*",          NO_SHARE_NET },
};

static const GENINFO types[] = {
   /* FIXME (adk): It has been long enough...time to kill these */
   { "backups",       "BackUPS",                   BK },
   { "sharebasic",    "ShareUPS Basic Port",       SHAREBASIC },
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
   { "pcnet",    "PCNET UPS Driver",    PCNET_UPS },
   { NULL,       "*invalid-ups-type*",  NO_UPS },
};

typedef struct {
   const char *key;
   HANDLER *handler;
   size_t offset;
   const GENINFO *values;
} PAIRS;

static const PAIRS table[] = {

   /* General parameters */

   {"UPSNAME",  match_str,   WHERE(upsname),  SIZE(upsname)},
   {"UPSCABLE", match_range, WHERE(cable),    cables},
   {"UPSTYPE",  match_range, WHERE(mode),     types},
   {"DEVICE",   match_str,   WHERE(device),   SIZE(device)},
   {"POLLTIME", match_int,   WHERE(polltime), 0},

   /* Paths */
   {"LOCKFILE",   match_str, WHERE(lockpath),    SIZE(lockpath)},
   {"SCRIPTDIR",  match_str, WHERE(scriptdir),   SIZE(scriptdir)},
   {"PWRFAILDIR", match_str, WHERE(pwrfailpath), SIZE(pwrfailpath)},
   {"NOLOGINDIR", match_str, WHERE(nologinpath), SIZE(nologinpath)},

   /* Configuration parameters used during power failures */
   {"ANNOY",          match_int,   WHERE(annoy),       0},
   {"ANNOYDELAY",     match_int,   WHERE(annoydelay),  0},
   {"ONBATTERYDELAY", match_int,   WHERE(onbattdelay), 0},
   {"TIMEOUT",        match_int,   WHERE(maxtime),     0},
   {"NOLOGON",        match_range, WHERE(nologin),     logins},
   {"BATTERYLEVEL",   match_int,   WHERE(percent),     0},
   {"MINUTES",        match_int,   WHERE(runtime),     0},
   {"KILLDELAY",      match_int,   WHERE(killdelay),   0},

   /* Configuration parmeters for network information server */
   {"NETSERVER", match_index, WHERE(netstats),   onoroff},
   {"NISIP",     match_str,   WHERE(nisip),      SIZE(nisip)},
   {"NISPORT",   match_int,   WHERE(statusport), 0},

   /* Configuration parameters for event logging */
   {"EVENTSFILE",    match_str, WHERE(eventfile),    SIZE(eventfile)},
   {"EVENTSFILEMAX", match_int, WHERE(eventfilemax), 0},

   /* Configuration parameters to control system logging */
   {"FACILITY", match_facility, 0,               0},
   {"STATFILE", match_str,      WHERE(statfile), SIZE(statfile)},
   {"LOGSTATS", match_index,    WHERE(logstats), onoroff},
   {"STATTIME", match_int,      WHERE(stattime), 0},
   {"DATATIME", match_int,      WHERE(datatime), 0},

   /* Configuration statements for network sharing of the UPS */
   {"UPSCLASS",  match_range, WHERE(upsclass),    upsclasses},
   {"UPSMODE",   match_range, WHERE(sharenet),    modes     },
   {"NETTIME",   match_int,   WHERE(polltime),    0         },

   /*
    * Obsolete configuration options: To be removed in the future.
    * The warning string is passed in the GENINFO* field since it is
    * not used any more for obsoleted options: we are only interested in
    * printing the message.
    * There is a new meaning for offset field, too. If TRUE will bail out,
    * if FALSE it will continue to run apcupsd. This way we can bail out
    * if an obsolete option is too important to continue running apcupsd.
    */
   {"CONTROL",    obsolete, TRUE,  (GENINFO *)"CONTROL config directive is obsolete"   },
   {"NETACCESS",  obsolete, TRUE,  (GENINFO *)"NETACCESS config directive is obsolete" },
   {"MASTER",     obsolete, TRUE,  (GENINFO *)"MASTER config directive is obsolete"    },
   {"USERMAGIC",  obsolete, FALSE, (GENINFO *)"USERMAGIC config directive is obsolete" },
   {"SLAVE",      obsolete, FALSE, (GENINFO *)"SLAVE config directive is obsolete"     },
   {"NETPORT",    obsolete, FALSE, (GENINFO *)"NETPORT config directive is obsolete"   },
   {"NETSTATUS",  obsolete, FALSE, (GENINFO *)"NETSTATUS config directive is obsolete" },
   {"SERVERPORT", obsolete, FALSE, (GENINFO *)"SERVERPORT config directive is obsolete"},

   /* Obsolete EEPROM options */
   {"SELFTEST",     obsolete, FALSE, (GENINFO *)"SELFTEST config directive is obsolete"    },
   {"HITRANSFER",   obsolete, FALSE, (GENINFO *)"HITRANSFER config directive is obsolete"  },
   {"LOTRANSFER",   obsolete, FALSE, (GENINFO *)"LOTRANSFER config directive is obsolete"  },
   {"LOWBATT",      obsolete, FALSE, (GENINFO *)"LOWBATT config directive is obsolete"     },
   {"WAKEUP",       obsolete, FALSE, (GENINFO *)"WAKEUP config directive is obsolete"      },
   {"RETURNCHARGE", obsolete, FALSE, (GENINFO *)"RETURNCHARGE config directive is obsolete"},
   {"OUTPUTVOLTS",  obsolete, FALSE, (GENINFO *)"OUTPUTVOLTS config directive is obsolete" },
   {"SLEEP",        obsolete, FALSE, (GENINFO *)"SLEEP config directive is obsolete"       },
   {"BEEPSTATE",    obsolete, FALSE, (GENINFO *)"BEEPSTATE config directive is obsolete"   },
   {"BATTDATE",     obsolete, FALSE, (GENINFO *)"BATTDATE config directive is obsolete"    },
   {"SENSITIVITY",  obsolete, FALSE, (GENINFO *)"SENSITIVITY config directive is obsolete" },

   /* must be last */
   {NULL, 0, 0, 0}
};

static int obsolete(UPSINFO *ups, int offset, const GENINFO * junk, const char *v)
{
   char *msg = (char *)junk;

   fprintf(stderr, "%s\n", msg);
   if (!offset) {
      fprintf(stderr, _("error ignored.\n"));
      return SUCCESS;
   }
   
   return FAILURE;
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
      if (!strcasecmp(x, vs->name))
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
      if (!strcasecmp(x, vs->name))
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


/*
 * We accept strings containing internal whitespace (in order to support
 * paths with spaces in them) but truncate after '#' since we assume it 
 * marks a comment.
 */
static int match_str(UPSINFO *ups, int offset, const GENINFO * gen, const char *v)
{
   char x[MAXSTRING];
   long size = (long)gen;

   /* Copy the string so we can edit it in place */
   astrncpy(x, v, sizeof(x));

   /* Remove trailing comment, if there is one */
   char *ptr = strchr(x, '#');
   if (ptr)
      *ptr = '\0';

   /* Remove any trailing whitespace */
   ptr = x + strlen(x) - 1;
   while (ptr >= x && isspace(*ptr))
      *ptr-- = '\0';

   astrncpy((char *)AT(ups, offset), x, (int)size);
   return SUCCESS;
}

static int match_facility(UPSINFO *ups, int offset,
   const GENINFO *junk, const char *v)
{
   const struct {
      const char *fn;
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
 *      pthreads structures initialized.
 */
void init_ups_struct(UPSINFO *ups)
{
   ups->fd = -1;

   ups->set_plugged();

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
   ups->polltime = 60;
   ups->percent = 10;
   ups->runtime = 5;
   ups->netstats = TRUE;
   ups->statusport = NISPORT;
   ups->nisip[0] = 0;              /* no nis IP file as default */

   ups->lockfile = -1;

   ups->clear_shut_load();
   ups->clear_shut_btime();
   ups->clear_shut_ltime();
   ups->clear_shut_emerg();
   ups->clear_shut_remote();

   ups->sysfac = LOG_DAEMON;

   ups->statfile[0] = 0;           /* no stats file default */
   ups->eventfile[0] = 0;          /* no events file as default */
   ups->eventfilemax = 10;         /* trim the events file at 10K as default */
   ups->event_fd = -1;             /* no file open */

   /* Default paths */
   astrncpy(ups->scriptdir, SYSCONFDIR, sizeof(ups->scriptdir));
   astrncpy(ups->pwrfailpath, PWRFAILDIR, sizeof(ups->pwrfailpath));
   astrncpy(ups->nologinpath, NOLOGDIR, sizeof(ups->nologinpath));
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
         Dmsg1(100, "%s\n", line);
         Dmsg2(100, _("Parsing error at line %d of config file %s.\n"), erpos, cfgfile);
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

   /*
    * If annoy time is greater than initial delay, don't bother about
    * initial delay and set it to 0.
    */
   if (ups->annoy >= ups->annoydelay)
      ups->annoydelay = 0;

   if (ups->sharenet.type == SHAREMASTER) {
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

   /* Append filenames to paths */
   Dmsg1(200, "After config scriptdir: \"%s\"\n", ups->scriptdir);
   Dmsg1(200, "After config pwrfailpath: \"%s\"\n", ups->pwrfailpath);
   Dmsg1(200, "After config nologinpath: \"%s\"\n", ups->nologinpath);
   astrncat(ups->nologinpath, NOLOGIN_FILE, sizeof(ups->nologinpath));
   astrncat(ups->pwrfailpath, PWRFAIL_FILE, sizeof(ups->pwrfailpath));

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

bail_out:
   if (errors)
      error_exit(_("Terminating due to configuration file errors.\n"));

   return;
}
