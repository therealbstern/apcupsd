/*
 *  struct.h  -- header file for apcupsd package
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  All rights reserved.
 *
 */

/*
 *                     GNU GENERAL PUBLIC LICENSE
 *                        Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *                           675 Mass Ave, Cambridge, MA 02139, USA
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

/*********************************************************************/

#ifndef _APC_STRUCT_H
#define _APC_STRUCT_H

typedef enum {
    NO_CABLE=0,     /* Default Disable            */
    CUSTOM_SIMPLE,  /* SIMPLE cable simple        */
    APC_940_0119A,  /* APC cable number 940-0119A */
    APC_940_0020B,  /* APC cable number 940-0020B */
    APC_940_0020C,  /* APC cable number 940-0020C identical to 20B */
    APC_940_0023A,  /* APC cable number 940-0023A */
    MAM_CABLE,      /* MAM cable for Alfatronic SPS500X */
    CUSTOM_SMART,   /* SMART cable smart          */
    APC_940_0024B,  /* APC cable number 940-0024B */
    APC_940_0024C,  /* APC cable number 940-0024C */
    APC_940_1524C,  /* APC cable number 940-1524C */
    APC_940_0024G,  /* APC cable number 940-0024G */
    APC_940_0095A,  /* APC cable number 940-0095A */
    APC_940_0095B,  /* APC cable number 940-0095B */
    APC_940_0095C,  /* APC cable number 940-0095C */
    APC_NET,        /* Ethernet Link              */
    USB_CABLE,      /* USB cable */
    APC_940_00XXX   /* APC cable number UNKNOWN   */
} UpsCable;

/* The order of these UpsModes is important!! */
typedef enum {
    NO_UPS=0,   /* Default Disable      */
    DUMB_UPS,   /* Dumb UPS driver      */
    BK,         /* Simple Signal        */
    SHAREBASIC, /* Simple Signal, Share */
    NETUPS,     /*                      */
    BKPRO,      /* SubSet Smart Signal  */
    VS,         /* SubSet Smart Signal  */
    NBKPRO,     /* Smarter BKPRO Signal */
    SMART,      /* Smart Signal         */
    MATRIX,     /* Smart Signal         */
    SHARESMART, /* Smart Signal, Share  */
    APCSMART_UPS, /* APC Smart UPS (any)*/
    USB_UPS,    /* USB UPS driver       */
    SNMP_UPS,   /* SNMP UPS driver      */
    NETWORK_UPS,/* NETWORK UPS driver   */
    TEST_UPS,   /* TEST UPS Driver      */
} UpsMode;

typedef enum {
    NO_CLASS=0,
    STANDALONE,     /**/
    SHARESLAVE,     /**/
    NETSLAVE,       /**/
    SHAREMASTER,    /**/
    NETMASTER,      /**/
    SHARENETMASTER  /**/
} ClassMode;

typedef enum {
    NO_SHARE_NET=0,
    DISABLE,    /* Disable Share or Net UPS  */
    SHARE,      /* ShareUPS Internal         */
    NET,        /* NetUPS                    */
    SHARENET    /* Share and Net, Master     */
} ShareNetMode;

typedef enum {
    NO_LOGON=0,   
    NEVER,      /* Disable Setting NoLogon               */
    TIMEOUT,    /* Based on TIMEOUT + 10 percent         */
    PERCENT,    /* Based on PERCENT + 10 percent         */
    MINUTES,    /* Based on MINUTES + 10 percent         */
    ALWAYS      /* Stop All New Login Attempts. but ROOT */
} NoLoginMode;

/*
 * List all the internal self tests allowed.
 */
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
    char *name;             /* JHNC: name mustn't contain whitespace */
    char *long_name;
    int type;
} GENINFO; /* for static declaration of data */

typedef struct internalgeninfo {
    char name[MAXSTRING];   /* JHNC: name mustn't contain whitespace */
    char long_name[MAXSTRING];
    int type;
} INTERNALGENINFO; /* for assigning into upsinfo */

/* Structure that contains information on each of our slaves.
 * Also, for a slave, the first slave packet is info on the
 * master.
 */
typedef struct slaveinfo {
    int remote_state;                 /* state of master */
    int disconnecting_slave;          /* set if old style slave */
    int ms_errno;                     /* errno last error */
    int socket;                       /* current open socket this slave */
    int port;                         /* port */
    int error;                        /* set when error message printed */
    int errorcnt;                     /* count of errors */
    time_t down_time;                 /* time slave was set RMT_DOWN */
    struct sockaddr_in addr;
    char usermagic[APC_MAGIC_SIZE];   /* Old style password */
    char name[MAXTOKENLEN];           /* master/slave domain name (or IP) */
    char password[MAXTOKENLEN];       /* for CRAM-MD5 authentication */
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


/* the following define MUST be changed each time you
 * change something in the upsinfo structure. Otherwise, you
 * risk to have users reading the shared memory with an old
 * nonvalid upsinfo structure. 
 */
#define UPSINFO_VERSION 12
/*
 * There is no need to change the following, but you can if
 * you want, but it must be at least 4 characters to match
 * the length of id[4] (not counting the EOS).
 */
#define UPSINFO_ID "UPS!"

typedef struct upsinfo {
    char id[4];
    int version;
    int size;
    char release[20];

    int fd;                       /* serial port file descriptor */
    int sp_flags;                 /* serial port flags on DUMB UPSes */

    /* UPS capability array and codes */
    char     UPS_Cap[CI_MAXCI+1];     /* TRUE if UPS has capability */
    unsigned int UPS_Cmd[CI_MAXCI+1]; /* Command or function code */

    char *buf;                        /* scratch buffer */
    int buf_len;                      /* buffer length */


    INTERNALGENINFO cable;
    INTERNALGENINFO enable_access;
    INTERNALGENINFO nologin;
    INTERNALGENINFO mode;
    INTERNALGENINFO class;
    INTERNALGENINFO sharenet;

    int num_execed_children;      /* children created in execute_command() */

    /* Internal state flags set in response to UPS condition */
    time_t ShutDown;              /* set when doing shutdown */
    time_t SelfTest;              /* start time of self test */
    time_t LastSelfTest;          /* time of last self test */
    time_t poll_time;             /* last time UPS polled -- fillUPS() */
    time_t start_time;            /* time apcupsd started */
    time_t last_onbatt_time;      /* last time on batteries */
    time_t last_offbatt_time;     /* last time off batteries */
    time_t last_time_on_line;
    time_t last_time_annoy;
    time_t last_time_nologon;
    time_t last_time_changeme;
    time_t last_master_connect_time; /* last time master connected */
    int num_xfers;                /* number of times on batteries */
    int cum_time_on_batt;         /* total time on batteries since startup */
    int wait_time;                /* suggested wait time for drivers in 
                                   * device_check_state() 
                                   */

    /* Items reported by smart UPS */
    /* Volatile items -- i.e. they change with the state of the UPS */
    char linequal[8];             /* Line quality */
    unsigned int reg1;            /* register 1 */
    unsigned int reg2;            /* register 2 */
    unsigned int reg3;            /* register 3 */
    unsigned int dipsw;           /* dip switch info */
    char G[8];                    /* reason for last switch to batteries */
    char X[8];                    /* results of last seft test */
    double BattChg;               /* remaining UPS charge % */
    double LineMin;               /* min line voltage seen */
    double LineMax;               /* max line voltage seen */
    double UPSLoad;               /* battery load percentage */
    double LineFreq;              /* line freq. */
    double LineVoltage;           /* Line Voltage */
    double OutputVoltage;         /* Output Voltage */
    double UPSTemp;               /* UPS internal temperature */
    double BattVoltage;           /* Actual Battery voltage -- about 24V */
    double LastSTTime;            /* hours since last self test -- not yet implemented */
    int32_t Status;               /* UPS status (Bitmapped) */
    int PoweredByUPS;             /* The only bit left out from the bitmap */
    double TimeLeft;              /* Est. time UPS can run on batt. */
    double humidity;              /* Humidity */
    double ambtemp;               /* Ambient temperature */
    char eprom[500];              /* Eprom values */

    /* Items reported by smart UPS */
    /* Static items that normally do not change during UPS operation */
    int NomOutputVoltage;         /* Nominial voltage when on batteries */
    double nombattv;              /* Nominal batt. voltage -- not actual */
    int extbatts;                 /* number of external batteries attached */
    int badbatts;                 /* number of bad batteries */
    int lotrans;                  /* min line voltage before using batt. */   
    int hitrans;                  /* max line voltage before using batt. */
    int rtnpct;                   /* % batt charge necessary for return */
    int dlowbatt;                 /* low batt warning in mins. */
    int dwake;                    /* wakeup delay seconds */
    int dshutd;                   /* shutdown delay seconds */
    char birth[20];               /* manufacture date */
    char serial[32];              /* serial number */
    char battdat[20];             /* battery installation date */
    char selftest[8];             /* selftest interval as ASCII */
    char firmrev[20];             /* firmware revision */
    char upsmodel[20];            /* ups model number */
    char sensitivity[8];          /* sensitivity to line fluxuations */
    char beepstate[8];            /* when to beep on power failure. */
    char selftestmsg[80];

    /* Items specified from config file */
    int annoy;
    int maxtime;
    int annoydelay;      /* delay before annoying users with logoff request */
    int killdelay;       /* delay after pwrfail before issuing UPS shutdown */
    int nologin_time;
    int nologin_file;
    int stattime;
    int datatime;
    int sysfac;
    int reports;
    int nettime;         /* Time interval for master to send to slaves */
    int percent;                     /* shutdown when batt % less than this */
    int runtime;                     /* shutdown when runtime less than this */
    char nisip[64];                  /* IP for NIS */
    int statusport;                  /* TCP port for STATUS */
    int netstats;                    /* turn on/off network status */
    int logstats;                    /* turn on/off logging of status info */
    char device[MAXSTRING];          /* device name in use */
    char configfile[APC_FILENAME_MAX];   /* config filename */
    char statfile[APC_FILENAME_MAX];     /* status filename */
    char eventfile[APC_FILENAME_MAX];    /* temp events file */
    int eventfilemax;                /* max size of eventfile in kilobytes */
    int event_fd;                    /* fd for eventfile */

    int NetUpsPort;                  /* Our communication port */
    char master_name[APC_FILENAME_MAX];  /**/

    char lockpath[APC_FILENAME_MAX];     /* BSC, made static -RF */
    int lockfile;                    /* BSC */

    char usermagic[APC_MAGIC_SIZE];  /* security id string */
    int ChangeBattCounter;           /* For UPS_REPLACEBATT, see apcaction.c */

    int remote_state;                /**/

    /*
     * Added with multi-UPS. We try to mantain the UPSINFO layout as clean as
     * possible to be backward compatible. All moved variables are kept as
     * reserved and all the new variables are at the end of the structure.
     */
    char upsname[UPSNAMELEN];   /* UPS config name */

#ifndef HAVE_PTHREADS
    /*
     * IPC
     */
    /*
     * Don't use shmUPS directly: call ipc functions to get it or
     * unpredictable results may happen.
     */
    int sem_id;                 /* Semaphore ID */
    int shm_id;                 /* Shared memory ID */
    struct sembuf semUPS[NUM_SEM_OPER]; /* Semaphore operators */
    int idshmUPS;               /* key of shared memory area */
    int idsemUPS;               /* key of semphores */
#endif

    /*
     * Linked list of UPSes used in apclist.c
     */

    struct upsinfo *next;
#ifdef HAVE_PTHREADS
    pthread_mutex_t mutex;
    int refcnt;                       /* thread attach count */
#endif

    struct upsdriver *driver;       /* UPS driver for this UPSINFO */
    void *driver_internal_data;     /* Driver private data */
} UPSINFO;

/* Used only in apcaccess.c */
typedef struct datainfo {
    char apcmagic[APC_MAGIC_SIZE];
    int update_master_config;
    int get_master_status;
    int slave_status;
    int call_master_shutdown;
    char accessmagic[ACCESS_MAGIC_SIZE];
} DATAINFO;

typedef int (HANDLER) (UPSINFO *, size_t, GENINFO *, const char *);

typedef struct {
    const char *key;
    HANDLER *handler;
    size_t offset;
    GENINFO *values;
    const char *help;
} PAIRS;

typedef struct configinfo {
    int new_annoy;
    int new_maxtime;
    int new_delay;
#ifdef __NOLOGIN
    int new_nologin;
#endif /* __NOLOGIN */
    int new_stattime;
    int new_datatime;
    int new_nettime;
    int new_percent;

} CONFIGINFO;


/*
 * These are needed for commands executed in apcaction.c
 */
typedef struct {
    char *command;
    int pid;
} UPSCOMMANDS;

typedef struct s_cmd_msg {
    int level;
    char *msg;
} UPSCMDMSG;

typedef struct netclient_s {
    int fd;                     /* Network FD */
    struct sockaddr_in who;     /* Other side's IP address */
    UPSINFO *ups;               /* Client's selected UPS */
    int authlevel;              /* Level of authorization of the client */
    int net_poll_time;          /* Time between two client's polls */
    int last_poll;              /* Last client's poll */
    struct netclient_s *next;   /* Linked list */
} NETCLIENT;

/*
 * Net commands structure.
 */
typedef struct {
    char *name;
    int (*function) (NETCLIENT * nc, int argc, char *argv[]);
    char *syntax;
    char *help;
    int authlevel;
} NETCMD;

#if defined(_SEM_SEMUN_UNDEFINED)
union semun {
    int val;                           /* value for SETVAL */
    struct semid_ds *buf;              /* buffer for IPC_STAT & IPC_SET */
    unsigned short int *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf;             /* buffer for IPC_INFO */
};
#endif

#endif /* _APC_STRUCT_H */
