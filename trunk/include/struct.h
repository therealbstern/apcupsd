/*
 * struct.h
 *
 * Common apcupsd structures.
 */

/*
 * Copyright (C) 2000-2005 Kern Sibbald
 * Copyright (C) 1996-1999 Andre M. Hedrick <andre@suse.com>
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

#ifndef _STRUCT_H
#define _STRUCT_H

typedef enum {
   NO_CABLE = 0,        /* Default Disable            */
   CUSTOM_SIMPLE,       /* SIMPLE cable simple        */
   APC_940_0119A,       /* APC cable number 940-0119A */
   APC_940_0127A,       /* APC cable number 940-0127A */
   APC_940_0128A,       /* APC cable number 940-0128A */
   APC_940_0020B,       /* APC cable number 940-0020B */
   APC_940_0020C,       /* APC cable number 940-0020C identical to 20B */
   APC_940_0023A,       /* APC cable number 940-0023A */
   MAM_CABLE,           /* MAM cable for Alfatronic SPS500X */
   CUSTOM_SMART,        /* SMART cable smart          */
   APC_940_0024B,       /* APC cable number 940-0024B */
   APC_940_0024C,       /* APC cable number 940-0024C */
   APC_940_1524C,       /* APC cable number 940-1524C */
   APC_940_0024G,       /* APC cable number 940-0024G */
   APC_940_0095A,       /* APC cable number 940-0095A */
   APC_940_0095B,       /* APC cable number 940-0095B */
   APC_940_0095C,       /* APC cable number 940-0095C */
   APC_NET,             /* Ethernet Link              */
   USB_CABLE,           /* USB cable */
   APC_940_00XXX        /* APC cable number UNKNOWN   */
} UpsCable;

/* The order of these UpsModes is important!! */
typedef enum {
   NO_UPS = 0,          /* Default Disable      */
   DUMB_UPS,            /* Dumb UPS driver      */
   BK,                  /* Simple Signal        */
   SHAREBASIC,          /* Simple Signal, Share */
   NETUPS,              /*                      */
   BKPRO,               /* SubSet Smart Signal  */
   VS,                  /* SubSet Smart Signal  */
   NBKPRO,              /* Smarter BKPRO Signal */
   SMART,               /* Smart Signal         */
   MATRIX,              /* Smart Signal         */
   SHARESMART,          /* Smart Signal, Share  */
   APCSMART_UPS,        /* APC Smart UPS (any) */
   USB_UPS,             /* USB UPS driver       */
   SNMP_UPS,            /* SNMP UPS driver      */
   NETWORK_UPS,         /* NETWORK UPS driver   */
   TEST_UPS             /* TEST UPS Driver      */
} UpsMode;

typedef enum {
   NO_CLASS = 0,
   STANDALONE,
   SHARESLAVE,
   NETSLAVE,
   SHAREMASTER,
   NETMASTER,
   SHARENETMASTER
} ClassMode;

typedef enum {
   NO_SHARE_NET = 0,
   DISABLE,             /* Disable Share or Net UPS  */
   SHARE,               /* ShareUPS Internal         */
   NET,                 /* NetUPS                    */
   SHARENET             /* Share and Net, Master     */
} ShareNetMode;

typedef enum {
   NO_LOGON = 0,
   NEVER,               /* Disable Setting NoLogon               */
   TIMEOUT,             /* Based on TIMEOUT + 10 percent         */
   PERCENT,             /* Based on PERCENT + 10 percent         */
   MINUTES,             /* Based on MINUTES + 10 percent         */
   ALWAYS               /* Stop All New Login Attempts. but ROOT */
} NoLoginMode;

/* List all the internal self tests allowed. */
typedef enum {
   SMART_TEST_LEDS = 0,
   SMART_TEST_SELFTEST,
   SMART_TEST_POWERFAIL,
   SMART_TEST_CALIBRATION,
   SMART_CHANGE_NAME,
   SMART_CHANGE_BATTDATE,
   SMART_CHANGE_EPROM,
   SMART_TEST_MAX
} SelfTests;

typedef enum {
   XFER_NA = 0,          /* Not supported by this UPS */
   XFER_NONE,            /* No xfer since power on */
   XFER_OVERVOLT,        /* Utility voltage too high */
   XFER_UNDERVOLT,       /* Utility voltage too low */
   XFER_NOTCHSPIKE,      /* Line voltage notch or spike */
   XFER_RIPPLE,          /* Excessive utility voltage rate of change */
   XFER_SELFTEST,        /* Auto or manual self test */
   XFER_FORCED,          /* Forced onto battery by sw command */
   XFER_FREQ,            /* Input frequency out of range */
   XFER_UNKNOWN
} LastXferCause;

typedef enum {
   TEST_NA = 0,          /* Not supported by this UPS */
   TEST_NONE,            /* No self test result available */
   TEST_FAILED,          /* Test failed (reason unknown) */
   TEST_WARNING,         /* Test passed with warning */
   TEST_INPROGRESS,      /* Test currently in progress */
   TEST_PASSED,          /* Test passed */
   TEST_FAILCAP,         /* Test failed due to insufficient capacity */
   TEST_FAILLOAD,        /* Test failed due to overload */
   TEST_UNKNOWN
} SelfTestResult;

/*
 * Internal selftest structure.
 * This structure is made by two variables, one for client side
 * and one for server side.
 * "activate" is the client side flag to activate a particular self
 * test. The client switch this flag to TRUE and then starts monitoring
 * the value of "status". The server (apcupsd) monitors the value of
 * "activate" and when it's true, enters the selftest state machine,
 * updating the value of "status" as the selftest proceed.
 * At the end of the selftest, apcupsd resets the value of "activate" to false
 * and leave the value of "status" to one of the possible selftest final
 * status.
 * It is important to say that the state machine rely in an array internal to
 * the UPSINFO structure so that every capable UPS can do asyncronously a
 * selftest.
 */
typedef struct SELFTEST {
   int activate;
   int status;
} SELFTEST;


typedef struct geninfo {
   const char *name;               /* JHNC: name mustn't contain whitespace */
   const char *long_name;
   int type;
} GENINFO;                         /* for static declaration of data */

typedef struct internalgeninfo {
   char name[MAXSTRING];           /* JHNC: name mustn't contain whitespace */
   char long_name[MAXSTRING];
   int type;
} INTERNALGENINFO;                 /* for assigning into upsinfo */

/* Structure that contains information on each of our slaves.
 * Also, for a slave, the first slave packet is info on the
 * master.
 */
typedef struct slaveinfo {
   int remote_state;               /* state of master */
   int disconnecting_slave;        /* set if old style slave */
   int ms_errno;                   /* errno last error */
   int socket;                     /* current open socket this slave */
   int port;                       /* port */
   int error;                      /* set when error message printed */
   int errorcnt;                   /* count of errors */
   time_t down_time;               /* time slave was set RMT_DOWN */
   struct sockaddr_in addr;
   char usermagic[APC_MAGIC_SIZE]; /* Old style password */
   char name[MAXTOKENLEN];         /* master/slave domain name (or IP) */
   char password[MAXTOKENLEN];     /* for CRAM-MD5 authentication */
} SLAVEINFO;

/* 
 * This structure is sent over the network between the    
 * master and the slaves, so we make the
 * length of character arrays a multiple of four and
 * keep the chars at the end to minimize memory alignment
 * problems. 
 */
typedef struct netdata {
   int32_t OnBatt;
   int32_t BattLow;
   int32_t BatteryUp;
   int32_t BattChg;
   int32_t ShutDown;
   int32_t nettime;
   int32_t TimeLeft;
   int32_t ChangeBatt;
   int32_t load;
   int32_t timedout;
   int32_t timelout;
   int32_t emergencydown;
   int32_t remote_state;
   int32_t cap_battlev;
   int32_t cap_runtim;
   char apcmagic[APC_MAGIC_SIZE];
   char usermagic[APC_MAGIC_SIZE];
} NETDATA;


/* No longer really needed since we do not use shared memory */
#define UPSINFO_VERSION 12

/*
 * There is no need to change the following, but you can if
 * you want, but it must be at least 4 characters to match
 * the length of id[4] (not counting the EOS).
 */
#define UPSINFO_ID "UPS!"

class UPSINFO {
 public:
   /* Methods */
   void clear_battlow() { Status &= ~UPS_battlow; };
   void clear_belowcaplimit() { Status &= ~UPS_belowcaplimit; };
   void clear_boost() { Status &= ~UPS_boost; };
   void clear_calibration() { Status &= ~UPS_calibration; };
   void clear_commlost() { Status &= ~UPS_commlost; };
   void clear_dev_setup() { Status &= ~UPS_dev_setup; };
   void clear_fastpoll() { Status &= ~UPS_fastpoll; };
   void clear_onbatt_msg() { Status &= ~UPS_onbatt_msg; };
   void clear_onbatt() { Status &= ~UPS_onbatt; };
   void clear_online() { Status |= UPS_onbatt; Status &= ~UPS_online; };
   void clear_overload() { Status &= ~UPS_overload; };
   void clear_plugged() { Status &= ~UPS_plugged; };
   void clear_remtimelimit() { Status &= ~UPS_remtimelimit; };
   void clear_replacebatt() { Status &= ~UPS_replacebatt; };
   void clear_shut_btime() { Status &= ~UPS_shut_btime; };
   void clear_shutdownimm() { Status &= ~UPS_shutdownimm; };
   void clear_shutdown() { Status &= ~UPS_shutdown; };
   void clear_shut_emerg() { Status &= ~UPS_shut_emerg; };
   void clear_shut_load() { Status &= ~UPS_shut_load; };
   void clear_shut_ltime() { Status &= ~UPS_shut_ltime; };
   void clear_shut_remote() { Status &= ~UPS_shut_remote; };
   void clear_slavedown() { Status &= ~UPS_slavedown; };
   void clear_slave() { Status &= ~UPS_slave; };
   void clear_trim() { Status &= ~UPS_trim; };
   void clear_battpresent() {Status &= ~UPS_battpresent; };

   void set_battlow() { Status |= UPS_battlow; };
   void set_battlow(int val) { if (val) Status |= UPS_battlow; \
           else Status &= ~UPS_battlow; };
   void set_belowcaplimit() { Status |= UPS_belowcaplimit; };
   void set_boost() { Status |= UPS_boost; };
   void set_boost(int val) { if (val) Status |= UPS_boost; \
           else Status &= ~UPS_boost; };
   void set_calibration() { Status |= UPS_calibration; };
   void set_commlost() { Status |= UPS_commlost; };
   void set_dev_setup() { Status |= UPS_dev_setup; };
   void set_fastpoll() { Status |= UPS_fastpoll; };
   void set_onbatt_msg() { Status |= UPS_onbatt_msg; };
   void set_onbatt() { Status |= UPS_onbatt; };
   void set_online() { Status |= UPS_online; Status &= ~UPS_onbatt; };
   void set_online(int val) { if (val) Status |= UPS_online; \
           else Status &= ~UPS_online; };
   void set_overload() { Status |= UPS_overload; };
   void set_overload(int val) { if (val) Status |= UPS_overload; \
           else Status &= ~UPS_overload; };
   void set_plugged() { Status |= UPS_plugged; };
   void set_remtimelimit() { Status |= UPS_remtimelimit; };
   void set_replacebatt() { Status |= UPS_replacebatt; };
   void set_replacebatt(int val) { if (val) Status |= UPS_replacebatt; \
           else Status &= ~UPS_replacebatt; };
   void set_shut_btime() { Status |= UPS_shut_btime; };
   void set_shutdownimm() { Status |= UPS_shutdownimm; };
   void set_shutdownimm(int val) { if (val) Status |= UPS_shutdownimm; \
           else Status &= ~UPS_shutdownimm; };
   void set_shutdown() { Status |= UPS_shutdown; };
   void set_shut_emerg() { Status |= UPS_shut_emerg; };
   void set_shut_load() { Status |= UPS_shut_load; };
   void set_shut_ltime() { Status |= UPS_shut_ltime; };
   void set_shut_remote() { Status |= UPS_shut_remote; };
   void set_slavedown() { Status |= UPS_slavedown; };
   void set_slave() { Status |= UPS_slave; };
   void set_trim() { Status |= UPS_trim; };
   void set_trim(int val) { if (val) Status |= UPS_trim; \
           else Status &= ~UPS_trim; };
   void set_battpresent() { Status |= UPS_battpresent; };
   void set_battpresent(int val) { if (val) Status |= UPS_battpresent; \
           else Status &= ~UPS_battpresent; };

   bool is_battlow() const { return (Status & UPS_battlow) == UPS_battlow; };
   bool is_boost() const { return (Status & UPS_boost) == UPS_boost; };
   bool is_calibration() const { return (Status & UPS_calibration) == UPS_calibration; };
   bool is_commlost() const { return (Status & UPS_commlost) == UPS_commlost; };
   bool is_dev_setup() const { return (Status & UPS_dev_setup) == UPS_dev_setup; };
   bool is_fastpoll() const { return (Status & UPS_fastpoll) == UPS_fastpoll; };
   bool is_onbatt() const { return (Status & UPS_onbatt) == UPS_onbatt; };
   bool is_onbatt_msg() const { return (Status & UPS_onbatt_msg) == UPS_onbatt_msg; };
   bool is_online() const { return (Status & UPS_online) == UPS_online; };
   bool is_overload() const { return (Status & UPS_overload) == UPS_overload; };
   bool is_plugged() const { return (Status & UPS_plugged) == UPS_plugged; };
   bool is_replacebatt() const { return (Status & UPS_replacebatt) == UPS_replacebatt; };
   bool is_shutdown() const { return (Status & UPS_shutdown) == UPS_shutdown; };
   bool is_shutdownimm() const { return (Status & UPS_shutdownimm) == UPS_shutdownimm; };
   bool is_shut_remote() const { return (Status & UPS_shut_remote) == UPS_shut_remote; };
   bool is_slave() const { return (Status & UPS_slave) == UPS_slave; };
   bool is_slavedown() const { return (Status & UPS_slavedown) == UPS_slavedown; };
   bool is_trim() const { return (Status & UPS_trim) == UPS_trim; };
   bool is_battpresent() const { return (Status & UPS_battpresent) == UPS_battpresent; };

   bool chg_battlow() const { return ((Status ^ PrevStatus) & UPS_battlow) == UPS_battlow; };
   bool chg_onbatt() const { return ((Status ^ PrevStatus) & UPS_onbatt) == UPS_onbatt; };
   bool chg_battpresent() const { return ((Status ^ PrevStatus) & UPS_battpresent) == UPS_battpresent; };

   /* DATA */
   char release[20];

   int fd;                         /* UPS device node file descriptor */

   /* UPS capability array and codes */
   char UPS_Cap[CI_MAXCI + 1];          /* TRUE if UPS has capability */
   unsigned int UPS_Cmd[CI_MAXCI + 1];  /* Command or function code */

   INTERNALGENINFO cable;
   INTERNALGENINFO enable_access;
   INTERNALGENINFO nologin;
   INTERNALGENINFO mode;
   INTERNALGENINFO upsclass;
   INTERNALGENINFO sharenet;

   int num_execed_children;        /* children created in execute_command() */

   /* Internal state flags set in response to UPS condition */
   time_t ShutDown;                /* set when doing shutdown */
   time_t SelfTest;                /* start time of self test */
   time_t LastSelfTest;            /* time of last self test */
   time_t poll_time;               /* last time UPS polled -- fillUPS() */
   time_t start_time;              /* time apcupsd started */
   time_t last_onbatt_time;        /* last time on batteries */
   time_t last_offbatt_time;       /* last time off batteries */
   time_t last_time_on_line;
   time_t last_time_annoy;
   time_t last_time_nologon;
   time_t last_time_changeme;
   time_t last_master_connect_time;     /* last time master connected */
   int num_xfers;                  /* number of times on batteries */
   int cum_time_on_batt;           /* total time on batteries since startup */
   int wait_time;                  /* suggested wait time for drivers in 
                                    * device_check_state() */

   /* Items reported by smart UPS */
   /* Volatile items -- i.e. they change with the state of the UPS */
   char linequal[8];               /* Line quality */
   unsigned int reg1;              /* register 1 */
   unsigned int reg2;              /* register 2 */
   unsigned int reg3;              /* register 3 */
   unsigned int dipsw;             /* dip switch info */
   unsigned int InputPhase;        /* The current AC input phase. */
   unsigned int OutputPhase;       /* The current AC output phase. */
   LastXferCause lastxfer;         /* Reason for last xfer to battery */
   SelfTestResult testresult;      /* results of last seft test */
   double BattChg;                 /* remaining UPS charge % */
   double LineMin;                 /* min line voltage seen */
   double LineMax;                 /* max line voltage seen */
   double UPSLoad;                 /* battery load percentage */
   double LineFreq;                /* line freq. */
   double LineVoltage;             /* Line Voltage */
   double OutputVoltage;           /* Output Voltage */
   double OutputFreq;              /* Output Frequency */
   double OutputCurrent;           /* Output Current */
   double UPSTemp;                 /* UPS internal temperature */
   double BattVoltage;             /* Actual Battery voltage -- about 24V */
   double LastSTTime;              /* hours since last self test -- not yet implemented */
   int32_t Status;                 /* UPS status (Bitmapped) */
   int32_t PrevStatus;             /* Previous UPS status */
   double TimeLeft;                /* Est. time UPS can run on batt. */
   double humidity;                /* Humidity */
   double ambtemp;                 /* Ambient temperature */
   char eprom[500];                /* Eprom values */

   /* Items reported by smart UPS */
   /* Static items that normally do not change during UPS operation */
   int NomOutputVoltage;           /* Nominial voltage when on batteries */
   double nombattv;                /* Nominal batt. voltage -- not actual */
   int extbatts;                   /* number of external batteries attached */
   int badbatts;                   /* number of bad batteries */
   int lotrans;                    /* min line voltage before using batt. */
   int hitrans;                    /* max line voltage before using batt. */
   int rtnpct;                     /* % batt charge necessary for return */
   int dlowbatt;                   /* low batt warning in mins. */
   int dwake;                      /* wakeup delay seconds */
   int dshutd;                     /* shutdown delay seconds */
   char birth[20];                 /* manufacture date */
   char serial[32];                /* serial number */
   char battdat[20];               /* battery installation date */
   char selftest[9];               /* selftest interval as ASCII */
   char firmrev[20];               /* firmware revision */
   char upsname[UPSNAMELEN];       /* UPS internal name */
   char upsmodel[20];              /* ups model number */
   char sensitivity[8];            /* sensitivity to line fluxuations */
   char beepstate[8];              /* when to beep on power failure. */

   /* Items specified from config file */
   int annoy;
   int maxtime;
   int annoydelay;                 /* delay before annoying users with logoff request */
   int onbattdelay;                /* delay before reacting to a power failure */
   int killdelay;                  /* delay after pwrfail before issuing UPS shutdown */
   int nologin_time;
   int nologin_file;
   int stattime;
   int datatime;
   int sysfac;
   int nettime;                    /* Time interval for master to send to slaves */
   int percent;                    /* shutdown when batt % less than this */
   int runtime;                    /* shutdown when runtime less than this */
   char nisip[64];                 /* IP for NIS */
   int statusport;                 /* NIS port */
   int netstats;                   /* turn on/off network status */
   int logstats;                   /* turn on/off logging of status info */
   char device[MAXSTRING];         /* device name in use */
   char configfile[APC_FILENAME_MAX];   /* config filename */
   char statfile[APC_FILENAME_MAX];     /* status filename */
   char eventfile[APC_FILENAME_MAX];    /* temp events file */
   int eventfilemax;               /* max size of eventfile in kilobytes */
   int event_fd;                   /* fd for eventfile */

   int NetUpsPort;                 /* Master/slave port */
   char master_name[APC_FILENAME_MAX];
   char lockpath[APC_FILENAME_MAX];
   int lockfile;

   char usermagic[APC_MAGIC_SIZE]; /* security id string */
   int ChangeBattCounter;          /* For UPS_REPLACEBATT, see apcaction.c */

   int remote_state;

   pthread_mutex_t mutex;
   int refcnt;                     /* thread attach count */

   const struct upsdriver *driver; /* UPS driver for this UPSINFO */
   void *driver_internal_data;     /* Driver private data */
};

/* Used only in apcaccess.c */
typedef struct datainfo {
   char apcmagic[APC_MAGIC_SIZE];
   int update_master_config;
   int get_master_status;
   int slave_status;
   int call_master_shutdown;
   char accessmagic[ACCESS_MAGIC_SIZE];
} DATAINFO;

typedef int (HANDLER) (UPSINFO *, int, const GENINFO *, const char *);

typedef struct {
   const char *key;
   HANDLER *handler;
   size_t offset;
   const GENINFO *values;
   const char *help;
} PAIRS;

typedef struct configinfo {
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
} CONFIGINFO;


/*These are needed for commands executed in apcaction.c */
typedef struct {
   char *command;
   int pid;
} UPSCOMMANDS;

typedef struct s_cmd_msg {
   int level;
   char *msg;
} UPSCMDMSG;

#endif   /* _STRUCT_H */
