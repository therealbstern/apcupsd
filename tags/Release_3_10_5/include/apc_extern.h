/*
 *  apc_extern.h  -- header file for apcupsd package
 *
 *  apcupsd.c -- Simple Daemon to catch power failure signals from a
 *               BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *            -- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
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

#ifndef _APC_EXTERN_H
#define _APC_EXTERN_H

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern UPSINFO myUPS;
extern UPSINFO *core_ups;
extern UPSDRIVER drivers[];
extern char argvalue[MAXSTRING];
extern void (*error_out)(char *file, int line, char *fmt,...);
extern void (*error_exit)(char *fmt,...);
extern void (*error_cleanup)(void);

/*
 * Serial bits
 */
extern int le_bit;
extern int dtr_bit;
extern int rts_bit;
extern int st_bit;
extern int sr_bit;
extern int cts_bit;
extern int cd_bit;
extern int rng_bit;
extern int dsr_bit;

/*
 * File opened
 */
extern int flags;

extern struct termios newtio;

extern int debug_net;

/*
 * getopt flags (see apcoptd.c)
 */
extern int show_version;
extern char *cfgfile;
extern int configure_ups;
extern int update_battery_date;
extern int debug_level;
extern int rename_ups;
extern int terminate_on_powerfail;
extern int kill_ups_power;
extern int dumb_mode_test;
extern int go_background;

/*
 * In apcdevice.c
 */
void setup_device(UPSINFO *ups);

/*
 * In apceeprom.c
 */
extern int do_eeprom_programming(UPSINFO *ups);


/*
 * In apcopt.c
 */
extern int parse_options(int argc, char *argv[]);

/*
 * In apcupsd.c
 */
extern void powerfail (int ok);
extern void logonfail (int ok);

extern void thread_terminate (int sig);
extern void clean_threads(void);
extern void apcupsd_terminate (int sig);
extern void clear_files (void);
extern void make_pid (void);

/*
 * In apcdevice.c
 */
extern void setup_serial(UPSINFO *ups);
extern void kill_power(UPSINFO *ups);
extern void save_dumb_status(UPSINFO *ups);
extern int check_serial(UPSINFO *ups);
extern void prep_device(UPSINFO *ups);
extern void do_device(UPSINFO *ups);

/*
 * In apcaction.c
 */
extern void timer_action(int sig);
extern void do_action(UPSINFO *ups);
extern void generate_event(UPSINFO *ups, int event);

/*
 * In apclock.c
 */
extern int create_lockfile (UPSINFO *ups);
extern void delete_lockfile (UPSINFO *ups);

/*
 * In apcnet.c
 */
extern int write_struct_net (int wsocketfd, struct netdata *write_struct, size_t size);
extern int read_struct_net (int rsocketfd, struct netdata *read_struct, size_t size);
extern void log_struct (struct netdata *logstruct);

extern int reconnect_master (UPSINFO *ups, int who);
extern int prepare_master (UPSINFO *ups);

extern int reconnect_slave (UPSINFO *ups);
extern int prepare_slave (UPSINFO *ups);
extern void kill_net (UPSINFO *ups);

extern void timer_net (int sig);
extern void do_net(UPSINFO *ups);
extern void timer_slaves (int sig);
extern void do_slaves(UPSINFO *ups);

/*
 * In apcfile.c
 */
extern int make_file(UPSINFO *ups, const char *path);
extern void make_pid_file(void);

/*
 * In apcpipe.c
 */
extern void sig_fifo_alarm (int sig);
extern int pipe_requests (UPSINFO *ups);
extern int pipe_reconfig (UPSINFO *ups, CONFIGINFO *config);
extern int pipe_master_status (UPSINFO *ups);
extern int pipe_call_shutdown (UPSINFO *ups);
extern int pipe_slave_reconnect (UPSINFO *ups);
extern int pipe_slave_release (UPSINFO *ups);

/*
 * In apcconfig.c
 */
extern int slave_count;
extern SLAVEINFO slaves[MAXSLAVES];

extern void init_ups_struct(UPSINFO *ups);
extern void check_for_config(UPSINFO *ups, char *cfgfile);

/*
 * In apcsetup.c
 */
extern void setup_ups_name (UPSINFO *ups);
extern void setup_ups_replace (UPSINFO *ups);

extern char *setup_ups_string (UPSINFO *ups, char cmd, char *setting);
extern int setup_ups_single (UPSINFO *ups, char cmd, int single);
extern int setup_ups_bubble (UPSINFO *ups, char cmd, int setting);

extern void setup_ups_sensitivity (UPSINFO *ups);
extern void setup_ups_wakeup (UPSINFO *ups);
extern void setup_ups_sleep (UPSINFO *ups);
extern void setup_ups_lo_xfer (UPSINFO *ups);
extern void setup_ups_hi_xfer (UPSINFO *ups);
extern void setup_ups_chargepoint (UPSINFO *ups);
extern void setup_ups_alarm (UPSINFO *ups);
extern void setup_ups_lowbatt_delay (UPSINFO *ups);
extern void setup_ups_selftest (UPSINFO *ups);

extern void get_apc_model(UPSINFO *ups);
extern void get_apc_capabilities(UPSINFO *ups);
extern void read_extended (UPSINFO *ups); 
extern void setup_extended(UPSINFO *ups); 

/*
 * In apcnis.c
 */
extern void do_server(UPSINFO *ups);
extern int check_wrappers(char *av, int newsock);

/*
 * In apcstatus.c
 */
extern int output_status (UPSINFO *ups, int fd, void s_open(UPSINFO *ups), 
           void s_write(UPSINFO *ups, char *fmt,...),  
           int s_close(UPSINFO *ups, int fd));
extern void stat_open(UPSINFO *ups);
extern int stat_close(UPSINFO *ups, int fd);
extern void stat_print(UPSINFO *ups, char *fmt, ...);


/*
 * In apcevents.c
 */
extern int trim_eventfile(UPSINFO *ups);
extern int output_events(int sockfd, FILE *events_file);


/*
 * In apcreports.c
 */
extern void clear_files(void);
extern int log_status (UPSINFO *ups);

extern void timer_reports (int sig);
extern void do_reports (UPSINFO *ups);

/*
 * In apcsmart.c
 */
extern int apc_enable(UPSINFO *ups);
extern int apc_write(char cmd, UPSINFO *ups);
extern char *apc_read(UPSINFO *ups);
extern char *apc_chat(char cmd, UPSINFO *ups);
extern int getline (char *s, int len, UPSINFO *ups);
extern void UPSlinkCheck (UPSINFO *ups);
extern char *smart_poll (char cmd, UPSINFO *ups);
extern int fillUPS (UPSINFO *ups);

/*
 * In apcsignal.c
 */
extern void init_timer (int timer, void (*fnhandler)(int));
extern void init_signals(void (*handler)(int));
extern void init_thread_signals(void);
extern void restore_signals(void);
extern void sleep_forever(void);

/*
 * In apcipc.c
 */
extern UPSINFO *new_ups();
extern UPSINFO *attach_ups(UPSINFO *ups, int shmperm);
extern int detach_ups(UPSINFO *ups);
extern int destroy_ups(UPSINFO *ups);

#define read_lock(ups) _read_lock(__FILE__, __LINE__, (ups))
#define read_unlock(ups) _read_unlock(__FILE__, __LINE__, (ups))
#define write_lock(ups) _write_lock(__FILE__, __LINE__, (ups))
#define write_unlock(ups) _write_unlock(__FILE__, __LINE__, (ups))
#define read_lock(ups) _read_lock(__FILE__, __LINE__, (ups))

extern int _read_lock(char *file, int line, UPSINFO *ups);
extern int _read_unlock(char *file, int line, UPSINFO *ups);
extern int _write_lock(char *file, int line, UPSINFO *ups);
extern int _write_unlock(char *file, int line, UPSINFO *ups);

/*
 * In apcexec.c
 */
extern int start_thread(UPSINFO *ups, void(*action)(UPSINFO *ups),
                       char *proctitle, char *argv0);
extern int execute_command(UPSINFO *ups, UPSCOMMANDS cmd);
extern void wait_for_termination (int serial_pid);

/*
 * In apclog.c
 */
extern void log_event (UPSINFO *ups, int level, char *fmt, ...);

/*
 * In apcproctitle.c
 */
#ifndef HAVE_SETPROCTITLE
extern void init_proctitle(char *a0);
extern int setproctitle(char *fmt, ...);
#endif

/*
 * In apcnetlib.c
 */
extern int net_open(char *host, char *service, int port);
extern void net_close(int sockfd);
extern int net_send(int sockfd, char *buff, int len);
extern int net_recv(int sockfd, char *buff, int maxlen);

/*
 * In apclist.c
 */
extern int insertUps(UPSINFO *ups);
extern UPSINFO *getNextUps(UPSINFO *ups);
extern UPSINFO *getUpsByname(char *name);

/*
 * In apcerror.c
 */
extern void generic_error_out(char *file, int line, char *fmt, ...);
extern void generic_error_exit(char *fmt, ...);
extern int avsnprintf(char *str, size_t size, const char *format, va_list ap);

/* 
 * In apcwinipc.c
 */
int winioctl(int fd, int func, int *addr);

#endif /* _APC_EXTERN_H */
