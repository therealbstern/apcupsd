/*
 *  defines.h -- header file for apcupsd package
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
   Copyright (C) 1999-2004 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#ifndef _APC_DEFINES_H
#define _APC_DEFINES_H

#define CORENAME "*Core*"

#define APCCONTROL              SYSCONFDIR "/apccontrol"

#ifndef APCCONF
#define APCCONF                 SYSCONFDIR "/apcupsd.conf"
#endif

#ifndef PWRFAILDIR
#define PWRFAILDIR              SYSCONFDIR
#endif

#ifndef PWRFAIL
#define PWRFAIL                 PWRFAILDIR "/powerfail"
#endif

#define NOLOGIN                 NOLOGDIR "/nologin"

#define APCPID                  PIDDIR "/apcupsd.pid"

/*
 * These two are not to be touched: we can not be sure how the user will
 * insert the locks directory path so we have to prepend the '/' just to be
 * sure: is better have /blah//LCK.. than /blahLCK..
 * -RF
 */
#define APC_LOCK_PREFIX         "/LCK.."
#define LOCK_DEFAULT            "/var/lock"

/*
 * JHNC:
 * This string should be the first line of the configuration file.
 * Then if we change the format later, we can just change this string.
 * Also, we could write code to use/convert out-of-date config files.
 */
#define APC_CONFIG_MAGIC        "## apcupsd.conf v1.1 ##"

#define POWERFAIL               "POWER FAILURE\n" /* put in nologin file */


#define MAXSTRING               256
#define MESSAGELEN              256
#define MAXTOKENLEN             100
#define MAXSLAVES               20
#define UPSNAMELEN              100

#define DEFAULT_SPEED           B2400

/*
 * These are for UPS internal test routines.
 * They are all != 0 because this way there will be no
 * risk of confusing these statuses as boolean values:
 * they are all a boolean true.
 */
#define UPS_TEST_PASSED         0x01
#define UPS_TEST_FAILED         0x02
#define UPS_TEST_INPROGRESS     0x03
#define UPS_TEST_ACTIVATED      0x04

/* bit values for APC UPS Status Byte (ups->Status) */
#define UPS_CALIBRATION   0x00000001
#define UPS_SMARTTRIM     0x00000002
#define UPS_SMARTBOOST    0x00000004
#define UPS_ONLINE        0x00000008
#define UPS_ONBATT        0x00000010
#define UPS_OVERLOAD      0x00000020
#define UPS_BATTLOW       0x00000040
#define UPS_REPLACEBATT   0x00000080
/* Extended bit values added by apcupsd */
#define UPS_COMMLOST      0x00000100  /* Communications with UPS lost */
#define UPS_SHUTDOWN      0x00000200  /* Shutdown in progress */
#define UPS_SLAVE         0x00000400  /* Set if this is a slave */
#define UPS_SLAVEDOWN     0x00000800  /* Slave not responding */
#define UPS_SHUTDOWNIMM   0x00001000  /* Shutdown imminent */
#define UPS_BELOWCAPLIMIT 0x00002000  /* Below battery capacity limit */
#define UPS_REMTIMELIMIT  0x00004000  /* Remaining run time limit exceeded */
#define UPS_PREV_ONBATT   0x00008000  /* Previous value for UPS_ONBATT */
#define UPS_PREV_BATTLOW  0x00010000  /* Previous value for UPS_BATTLOW */
#define UPS_ONBATT_MSG    0x00020000  /* Set when UPS_ONBATT message is sent */
#define UPS_FASTPOLL      0x00040000  /* Set on power failure to poll faster */
#define UPS_SHUT_LOAD     0x00080000  /* Set when BatLoad <= percent */
#define UPS_SHUT_BTIME    0x00100000  /* Set when time on batts > maxtime */
#define UPS_SHUT_LTIME    0x00200000  /* Set when TimeLeft <= runtime */
#define UPS_SHUT_EMERG    0x00400000  /* Set when battery power has failed */
#define UPS_SHUT_REMOTE   0x00800000  /* Set when remote shutdown */
#define UPS_PLUGGED       0x01000000  /* Set if computer is plugged into UPS */
#define UPS_DEV_SETUP     0x02000000  /* Set if UPS's driver did the setup() */

#define UPS_LOCAL_BITS (UPS_COMMLOST|UPS_SHUTDOWN|UPS_SLAVE|UPS_SLAVEDOWN|UPS_PREV_ONBATT| \
            UPS_PREV_BATTLOW|UPS_ONBATT_MSG|UPS_FASTPOLL|UPS_PLUGGED|UPS_DEV_SETUP)

/* Macro to set/clear/test bit values in ups->Status */
#define is_ups_set(bit) ((ups->Status) & (bit))
#define set_ups(bit) ((ups->Status) |= (bit))
#define clear_ups(bit) ((ups->Status) &= ~(bit))

/*
 * Macro specific for UPS_ONLINE/UPS_ONBATT. It's a pity but
 * we can't do anything else as APC smart Status handles both.
 * Use these macros anytime you want to SET/CLEAR any of
 * UPS_ONLINE and UPS_ONBATT.
 */
#define set_ups_online() \
    do { \
        set_ups(UPS_ONLINE); \
        clear_ups(UPS_ONBATT); \
    } while (0)
#define clear_ups_online() \
    do { \
        set_ups(UPS_ONBATT); \
        clear_ups(UPS_ONLINE); \
    } while (0)


/* Old deprecated defines (hard to read) */
#define UPS_ISSET(bit) ((ups->Status) & (bit))
#define UPS_SET(bit) ((ups->Status) |= (bit))
#define UPS_CLEAR(bit) ((ups->Status) &= ~(bit))
#define UPS_SET_ONLINE() \
    do { \
        set_ups(UPS_ONLINE); \
        clear_ups(UPS_ONBATT); \
    } while (0)
#define UPS_CLEAR_ONLINE() \
    do { \
        set_ups(UPS_ONBATT); \
        clear_ups(UPS_ONLINE); \
    } while (0)


/*
 * CI_ is Capability or command index
 * APC_CMD_ is the command code sent to UPS for APC Smart UPSes
 *  NOTE: the APC_CMD_s are never used in the actual code,
 *        except to initialize the UPS_Cmd[] structure.
 *        this way, we will be able to support other UPSes
 *        later.
 *        The actual command is obtained by reference to
 *        UPS_Cmd[CI_xxx]    
 *  If the command is valid for this UPS, UPS_Cap[CI_xxx]
 *        will be true.
 */

#define CI_UPSMODEL             0             /* Model number */
#define    APC_CMD_UPSMODEL       'V'
#define CI_STATUS               1             /* status function */
#define    APC_CMD_STATUS         'Q'
#define CI_LQUAL                2             /* line quality status */ 
#define    APC_CMD_LQUAL          '9'
#define CI_WHY_BATT             3              /* why transferred to battery */
#define    APC_CMD_WHY_BATT       'G'
#define CI_ST_STAT              4              /* self test stat */
#define    APC_CMD_ST_STAT        'X'
#define CI_VLINE                5              /* line voltage */
#define    APC_CMD_VLINE          'L'
#define CI_VMAX                 6              /* max voltage */
#define    APC_CMD_VMAX           'M'
#define CI_VMIN                 7              /* min line voltage */
#define    APC_CMD_VMIN           'N'
#define CI_VOUT                 8              /* Output voltage */
#define    APC_CMD_VOUT           'O'
#define CI_BATTLEV              9              /* Battery level percentage */
#define CI_RemainingCapacity    9
#define    APC_CMD_BATTLEV        'f'
#define CI_VBATT               10              /* Battery voltage */
#define    APC_CMD_VBATT          'B'
#define CI_LOAD                11              /* UPS Load */
#define    APC_CMD_LOAD           'P'
#define CI_FREQ                12              /* Line Frequency */
#define    APC_CMD_FREQ           'F'
#define CI_RUNTIM              13              /* Est. Runtime left */
#define CI_RunTimeToEmpty      13
#define    APC_CMD_RUNTIM         'j'
#define CI_ITEMP               14              /* Internal UPS temperature */
#define    APC_CMD_ITEMP          'C'
#define CI_DIPSW               15              /* Dip switch settings */
#define    APC_CMD_DIPSW          '7'
#define CI_SENS                16              /* Sensitivity */
#define    APC_CMD_SENS           's'
#define CI_DWAKE               17              /* Wakeup delay */
#define    APC_CMD_DWAKE          'r'
#define CI_DSHUTD              18              /* Shutdown delay */
#define    APC_CMD_DSHUTD         'p'
#define CI_LTRANS              19              /* Low transfer voltage */
#define    APC_CMD_LTRANS         'l'
#define CI_HTRANS              20               /* High transfer voltage */
#define    APC_CMD_HTRANS         'u'
#define CI_RETPCT              21               /* Return percent threshhold */
#define    APC_CMD_RETPCT         'e'
#define CI_DALARM              22               /* Alarm delay */
#define    APC_CMD_DALARM         'k'
#define CI_DLBATT              23               /* low battery warning, mins */
#define    APC_CMD_DLBATT         'q'
#define CI_IDEN                24               /* UPS Identification (name) */
#define    APC_CMD_IDEN           'c'
#define CI_STESTI              25               /* Self test interval */
#define    APC_CMD_STESTI         'E'
#define CI_MANDAT              26               /* Manufacture date */
#define    APC_CMD_MANDAT         'm'
#define CI_SERNO               27               /* serial number */
#define    APC_CMD_SERNO          'n'
#define CI_BATTDAT             28               /* Last battery change */
#define    APC_CMD_BATTDAT        'x'
#define CI_NOMBATTV            29               /* Nominal battery voltage */
#define    APC_CMD_NOMBATTV       'g'
#define CI_HUMID               30               /* UPS Humidity percentage */
#define    APC_CMD_HUMID          'h'
#define CI_REVNO               31               /* Firmware revision */
#define    APC_CMD_REVNO          'b'
#define CI_REG1                32               /* Register 1 */
#define    APC_CMD_REG1           '~'
#define CI_REG2                33               /* Register 2 */
#define    APC_CMD_REG2           '\''
#define CI_REG3                34               /* Register 3 */
#define    APC_CMD_REG3           '8'
#define CI_EXTBATTS            35               /* Number of external batteries */
#define    APC_CMD_EXTBATTS       '>'
#define CI_ATEMP               36               /* Ambient temp */
#define    APC_CMD_ATEMP          't'
#define CI_NOMOUTV             37               /* Nominal output voltage */
#define    APC_CMD_NOMOUTV        'o'
#define CI_BADBATTS            38               /* Number of bad battery packs */
#define    APC_CMD_BADBATTS       '<'
#define CI_EPROM               39               /* Valid eprom values */
#define    APC_CMD_EPROM          0x1a
#define CI_ST_TIME             40               /* hours since last self test */
#define    APC_CMD_ST_TIME        'd'
#define CI_Manufacturer                  41
#define    APC_CMD_MANUFACTURER           0
#define CI_ShutdownRequested             42
#define    APC_CMD_ShutdownRequested      0
#define CI_ShutdownImminent              43
#define    APC_CMD_ShutdownImminent       0
#define CI_DelayBeforeReboot             44
#define    APC_CMD_DelayBeforeReboot      0
#define CI_BelowRemCapLimit              45
#define    APC_CMD_BelowRemCapLimit       0
#define CI_RemTimeLimitExpired           46
#define    APC_CMD_RemTimeLimitExpired    0
#define CI_Charging                      47
#define    APC_CMD_Charging               0
#define CI_Discharging                   48
#define    APC_CMD_Discharging            0
#define CI_RemCapLimit                   49
#define    APC_CMD_RemCapLimit            0
#define CI_RemTimeLimit                  50
#define    APC_CMD_RemTimeLimit           0
#define CI_WarningCapacityLimit          51
#define    APC_CMD_WarningCapacityLimit   0
#define CI_CapacityMode                  52
#define    APC_CMD_CapacityMode           0
#define CI_BattPackLevel                 53
#define    APC_CMD_BattPackLevel          0
#define CI_CycleCount                    54
#define    APC_CMD_CycleCount             0
#define CI_ACPresent                     55
#define    APC_CMD_ACPresent              0
#define CI_Boost                         56
#define    APC_CMD_Boost                  0
#define CI_Trim                          57
#define    APC_CMD_Trim                   0
#define CI_Overload                      58
#define    APC_CMD_Overload               0
#define CI_NeedReplacement               59
#define    APC_CMD_NeedReplacement        0
#define CI_BattReplaceDate               60
#define    APC_CMD_BattReplaceDate        0
#define CI_APCForceShutdown              61
#define    APC_CMD_ForceShutdown          0
#define CI_DelayBeforeShutdown           62
#define    APC_CMD_DelayBeforeShutdown    0

/* Items below this line are not "probed" for */
#define CI_CYCLE_EPROM         63     /* Cycle programmable EPROM values */
#define    APC_CMD_CYCLE_EPROM    '-'
#define CI_UPS_CAPS            64     /* Get UPS capabilities (command) string */
#define    APC_CMD_UPS_CAPS       'a'
/* ^^^^^^^^^^ see below if you change this ^^^^^^ */
/* set to last command index. CHANGE!!! when adding new code.  */
/* vvvvvvvvvv change here vvvvvvvvvvvvvvvvvvvvvvv */
#define CI_MAXCI         CI_UPS_CAPS      /* maximum UPS commands we handle */
#define CI_MAX_CAPS      CI_DelayBeforeShutdown

#define GO_ON_BATT              'W'
#define GO_ON_LINE              'X'
#define LIGHTS_TEST             'A'
#define FAILURE_TEST            'U'

/*
 * Future additions for contolled discharing of batteries
 * extend lifetimes.
 */

#define DISCHARGE               'D'
#define CHARGE_LIM              25

#define UPS_ENABLED             '?'
#define UPS_ON_BATT             '!'
#define UPS_ON_LINE             '$'
#define UPS_REPLACE_BATTERY     '#'
#define BATT_LOW                '%'
#define BATT_OK                 '+'
#define UPS_EPROM_CHANGE        '|'
#define UPS_TRAILOR             ':'
#define UPS_LF                  '\n'
#define UPS_CR                  '\r'
/*
 * For apclock.c functions
 *
 * -RF
 */
#define LCKSUCCESS              0 /* lock file does not exist so go */
#define LCKERROR                1 /* lock file not our own and error encountered */
#define LCKEXIST                2 /* lock file is our own lock file */
#define LCKNOLOCK               3 /* lock file not needed: APC_NET */

/*
 * Generic defines for boolean return values.
 *
 * -RF
 */
#define SUCCESS                 0 /* Function successfull */
#define FAILURE                 1 /* Function failure */

/* These seem unavoidable :-( */
#ifndef TRUE
# define TRUE                   1
#endif
#ifndef FALSE
# define FALSE                  0
#endif

#ifndef __cplusplus
#define bool int
#define true  1
#define false 0
#endif


/*
 * We have a timer for the read() for Win32.
 * We have a timer for the select when nothing is expected,
 *   i.e. we prefer waiting for an state change.
 * We have a fast timer, when we are on batteries or when
 *   we expect a response (i.e. we sent a character).
 * And we have a timer for dumb UPSes for doing the sleep().
 *
 */
#define TIMER_READ              10    /* read() timeout, max 25 sec */
#define TIMER_SELECT            60    /* Select when not expecting anything */
#define TIMER_FAST              1     /* Value for fast poll */
#define TIMER_DUMB              5     /* for Dumb (ioctl) UPSes -- keep short */

#define MASTER_TIMEOUT        120     /* master must respond in this time */

/*
 * Old net code will be obsoleted sometime.
 */
#define TIMER_SLAVES            10

/* Make the size of these strings the next multiple of 4 */
#define APC_MAGIC               "apcupsd-linux-6.0"
#define APC_MAGIC_SIZE          4 * ((sizeof(APC_MAGIC) + 3) / 4)

#define ACCESS_MAGIC            "apcaccess-linux-4.0"
#define ACCESS_MAGIC_SIZE       4 * ((sizeof(APC_MAGIC) + 3) / 4)

/* 
 * These are the remote_state for networked master/slaves
 */

/*
 * The first 5 are from the original protocol. Later states apply to the
 * master only and should be hidden from the slave to preserve
 * backwards compatibility
 */
#define RMT_NOTCONNECTED        0
#define RMT_CONNECTED           1
#define RMT_RECONNECT           2
#define RMT_ERROR               3
#define RMT_DOWN                4
/*
 * Master only internal states
 */
    /*
     * Convert these to RMT_NOTCONNECTED when sending to slave
     */
#define RMT_CONNECTING1         5
#define RMT_CONNECTING2         6
#define RMT_CONNECTING3         7
    /*
     * Convert these to RMT_RECONNECT when sending to slave
     */
#define RMT_RECONNECTING1       8
#define RMT_RECONNECTING2       9
#define RMT_RECONNECTING3      10

#define MAX_THREADS             7

/*
 * IPC defines.
 */
#define SEM_ID                  0x00FEED00
#define SHM_ID                  0x10FEED01
#define READ_CNT                0
#define WRITE_LCK               1
#define NUM_SEM                 2
#define NUM_SEM_OPER            3    /* 3 for write sem that need 1 operation more */

/*
 * Find members position in the UPSINFO and GLOBALCFG structures.
 */
#define WHERE(MEMBER) ((size_t) &((UPSINFO *)0)->MEMBER)
#define AT(UPS,OFFSET) ((size_t)UPS + OFFSET)
#define SIZE(MEMBER) ((GENINFO *)sizeof(((UPSINFO *)0)->MEMBER))


/*
 * Only 1 semaphore
 */
#define SEMNUM                  0


/*
 * These are the commands understood by the apccontrol shell script.
 * You _must_ keep the #defines in sync with the commands[] array in
 * apcaction.c
 */
#define CMDPOWEROUT      0
#define CMDONBATTERY     1
#define CMDFAILING       2
#define CMDTIMEOUT       3
#define CMDLOADLIMIT     4
#define CMDRUNLIMIT      5
#define CMDDOREBOOT      6
#define CMDDOSHUTDOWN    7
#define CMDMAINSBACK     8
#define CMDANNOYME       9
#define CMDEMERGENCY     10
#define CMDCHANGEME      11
#define CMDREMOTEDOWN    12
#define CMDCOMMFAILURE   13
#define CMDCOMMOK        14
#define CMDSTARTSELFTEST 15
#define CMDENDSELFTEST   16
#define CMDMASTERTIMEOUT 17           /* Master timed out */
#define CMDMASTERCONN    18           /* Connect to master */
#define CMDOFFBATTERY    19           /* off battery power */

/*
 * NetCodes for numeric chatting.
 */
#define NETCODENUL          100
#define NETCODEQUIT         101
#define NETCODERETRY        102
#define NETCODEOK           200
#define NETCODEURG          201
#define NETCODENOERR        202
#define NETCODEDONE         203
#define NETCODEERR          300
#define NETCODEINACT        301


/* Simple way of handling varargs for those compilers that
 * don't support varargs in #defines.
 */
#define Error_abort0(fmd) error_out(__FILE__, __LINE__, fmd)
#define Error_abort1(fmd, arg1) error_out(__FILE__, __LINE__, fmd, arg1)
#define Error_abort2(fmd, arg1,arg2) error_out(__FILE__, __LINE__, fmd, arg1,arg2)
#define Error_abort3(fmd, arg1,arg2,arg3) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3)
#define Error_abort4(fmd, arg1,arg2,arg3,arg4) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4)
#define Error_abort5(fmd, arg1,arg2,arg3,arg4,arg5) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4,arg5)
#define Error_abort6(fmd, arg1,arg2,arg3,arg4,arg5,arg6) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4,arg5,arg5)


/*
 * The digit following Dmsg and Emsg indicates the number of substitutions in
 * the message string. We need to do this kludge because non-GNU compilers
 * do not handle varargs #defines.
 */
/* Debug Messages that are printed */
#ifdef DEBUG
#define Dmsg0(lvl, msg)             d_msg(__FILE__, __LINE__, lvl, msg)
#define Dmsg1(lvl, msg, a1)         d_msg(__FILE__, __LINE__, lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)     d_msg(__FILE__, __LINE__, lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4) d_msg(__FILE__, __LINE__, lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
void d_msg(const char *file, int line, int level, const char *fmt,...);
#else
#define Dmsg0(lvl, msg)
#define Dmsg1(lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#endif


#ifdef HAVE_PTHREADS
/* These probably should be subroutines */
#define P(x) \
   do { int errstat; if ((errstat=pthread_mutex_lock(&(x)))) \
      error_out(__FILE__, __LINE__, "Mutex lock failure. ERR=%s\n",\
           strerror(errstat)); \
   } while(0)

#define V(x) \
   do { int errstat; if ((errstat=pthread_mutex_unlock(&(x)))) \
         error_out(__FILE__, __LINE__, "Mutex unlock failure. ERR=%s\n",\
           strerror(errstat)); \
   } while(0)

#endif

/*
 * Send terminate signal to itself.
 */
#define sendsig_terminate() \
    { \
        kill(getpid(), SIGTERM); \
        exit(0); \
    }

#endif /* _APC_DEFINES_H */
