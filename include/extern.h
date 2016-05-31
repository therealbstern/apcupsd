/*
 * extern.h
 *
 * Public exports.
 */

/*
 * Copyright (C) 2000-2005 Kern Sibbald
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

#ifndef _EXTERN_H
#define _EXTERN_H

/* Function Prototypes */

extern UPSINFO *core_ups;
extern char argvalue[MAXSTRING];
extern void (*error_out) (const char *file, int line, const char *fmt, va_list arg_ptr);
extern void (*error_cleanup) (void);
extern void error_out_wrapper(const char *file, int line, const char *fmt, ...) __attribute__((noreturn));

extern UPSCOMMANDS ups_event[];
extern UPSCMDMSG event_msg[];


/* Serial bits */
extern int le_bit;
extern int dtr_bit;
extern int rts_bit;
extern int st_bit;
extern int sr_bit;
extern int cts_bit;
extern int cd_bit;
extern int rng_bit;
extern int dsr_bit;

/* File opened */
extern int flags;
extern struct termios newtio;
extern int debug_net;

/* getopt flags (see apcoptd.c) */
extern int show_version;
extern char *cfgfile;
extern int configure_ups;
extern int update_battery_date;
extern int debug_level;
extern int rename_ups;
extern int terminate_on_powerfail;
extern int hibernate_ups;
extern int shutdown_ups;
extern int dumb_mode_test;
extern int go_background;

/* In apcopt.c */
extern int parse_options(int argc, char *argv[]);

/* In apcupsd.c */
extern void apcupsd_terminate(int sig);
extern void clear_files(void);

/* In apcdevice.c */
bool setup_device(UPSINFO *ups);
extern void initiate_hibernate(UPSINFO *ups);
extern void initiate_shutdown(UPSINFO *ups);
extern void prep_device(UPSINFO *ups);
extern void do_device(UPSINFO *ups);
extern int fillUPS(UPSINFO *ups);

/* In apcaction.c */
extern void do_action(UPSINFO *ups);
extern void generate_event(UPSINFO *ups, int event);

/* In apclock.c */
extern int create_lockfile(UPSINFO *ups);
extern void delete_lockfile(UPSINFO *ups);

/* In apcfile.c */
extern int make_file(UPSINFO *ups, const char *path);
extern void make_pid_file(void);
extern void make_pid(void);

/* In apcconfig.c */
extern char APCCONF[APC_FILENAME_MAX];
extern void init_ups_struct(UPSINFO *ups);
extern void check_for_config(UPSINFO *ups, char *cfgfile);

/* In apcnis.c */
extern void do_server(UPSINFO *ups);
extern int check_wrappers(char *av, int newsock);

/* In apcstatus.c */
extern int output_status(UPSINFO *ups, int fd, void s_open(UPSINFO * ups),
   void s_write(UPSINFO *ups, const char *fmt, ...), int s_close(UPSINFO * ups, int fd));
extern void stat_open(UPSINFO *ups);
extern int stat_close(UPSINFO *ups, int fd);
extern void stat_print(UPSINFO *ups, const char *fmt, ...);

/* In apcevents.c */
extern int trim_eventfile(UPSINFO *ups);
extern int output_events(int sockfd, FILE *events_file);

/* In apcreports.c */
extern void clear_files(void);
extern int log_status(UPSINFO *ups);
extern void do_reports(UPSINFO *ups);

/* In apcsignal.c */
extern void init_signals(void (*handler) (int));

/* In newups.c */
extern UPSINFO *new_ups(void);
extern UPSINFO *attach_ups(UPSINFO *ups);
extern void detach_ups(UPSINFO *ups);
extern void destroy_ups(UPSINFO *ups);

#define read_lock(ups) _read_lock(__FILE__, __LINE__, (ups))
#define read_unlock(ups) _read_unlock(__FILE__, __LINE__, (ups))
#define write_lock(ups) _write_lock(__FILE__, __LINE__, (ups))
#define write_unlock(ups) _write_unlock(__FILE__, __LINE__, (ups))
#define read_lock(ups) _read_lock(__FILE__, __LINE__, (ups))

extern void _read_lock(const char *file, int line, UPSINFO *ups);
extern void _read_unlock(const char *file, int line, UPSINFO *ups);
extern void _write_lock(const char *file, int line, UPSINFO *ups);
extern void _write_unlock(const char *file, int line, UPSINFO *ups);

/* In apcexec.c */
extern int start_thread(UPSINFO *ups, void (*action) (UPSINFO * ups),
   const char *proctitle, char *argv0);
extern int execute_command(UPSINFO *ups, UPSCOMMANDS cmd);
extern void clean_threads(void);

/* In apclog.c */
extern void log_event(const UPSINFO *ups, int level, const char *fmt, ...);
extern void logf(const char *fmt, ...);
extern int format_date(time_t timestamp, char *dest, size_t destlen);

/* In apcerror.c */
extern void generic_error_out(const char *file, int line, const char *fmt, ...);
extern void generic_error_exit(const char *fmt, ...);

/* In asys.c */
int avsnprintf(char *str, size_t size, const char *format, va_list ap);
int asnprintf(char *str, size_t size, const char *fmt, ...);
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t size);
#endif

/* In sleep.c */
#ifndef HAVE_NANOSLEEP
  int nanosleep(const struct timespec *req, struct timespec *rem);
#endif

/* In sockcloexec.c */
sock_t socket_cloexec(int domain, int type, int protocol);
sock_t accept_cloexec(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/*
 * Common interface to the various versions of gethostbyname_r().
 * Implemented in gethostname.c.
 */
struct hostent * gethostname_re
    (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen);

#endif   /* _EXTERN_H */
