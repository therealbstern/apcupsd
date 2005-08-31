/*
 * powerflute.c
 *
 * ncurses interface to apcupsd.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1998-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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
#ifdef HAVE_POWERFLUTE

#ifdef HAVE_NCURSES_MENU_H
# include <ncurses/curses.h>
# include <ncurses/panel.h>
# include <ncurses/menu.h>
#else
# ifdef HAVE_MENU_H
#  include <curses.h>
#  include <panel.h>
#  include <menu.h>
# else
#  error You must have ncurses installed to use powerflute.
# endif                            /* HAVE_MENU_H */
#endif                             /* HAVE_NCURSES_MENU_H */

#define TIMER_POWERFLUTE        5
#define RESTORE_BUFFER          3

/*
 * This is needed for restart ncurses and all the circus.
 * ncurses or something other seems to have a bug that prevent it to
 * refresh correctly the screen when a wall message is received.
 */
jmp_buf env;

/* Buffer to save the last 8 lines of mesgwin. */
char mesgbuf[8][1024];

/* Menubar window and panel */
static WINDOW *menuwin;
static PANEL *menupan;

/* Status window and panel */
static WINDOW *statwin;
static PANEL *statpan;

/* Monitoring window and panel */
static WINDOW *moniwin;
static PANEL *monipan;

/* Message window and panel */
static WINDOW *mesgwin;
static PANEL *mesgpan;

static MENU *mu = NULL;
static WINDOW *mw = NULL;
static WINDOW *dmw = NULL;
static PANEL *mpan;

/* UPS structure pointers for shared ptr and for local ptr. */
UPSINFO *sharedups = NULL;
UPSINFO *ups = NULL;
char argvalue[MAXSTRING];

/* Default values for contacting daemon */
static char *host = "localhost";
static int port = NISPORT;


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

int closing_curses = 0;

static void close_curses(void);
static void write_mesg(const char *fmt, ...);

void load_events(void)
{
   FILE *fp;
   int count = 0;
   char buffer[1024];

   fp = fopen(ups->eventfile, "r");

   if (fp == NULL)
      return;

   /* Count the lines in stat file. */
   while (fgets(buffer, 1024, fp) != NULL)
      count++;

   rewind(fp);

   /* Skip lines not needed. */
   while (count > 9) {
      fgets(buffer, 1024, fp);
      count--;
   }

   /* Output last 9 (or less if file is less) lines. */
   while (fgets(buffer, 1024, fp) != NULL) {
      buffer[strlen(buffer) - 1] = '\0';
      write_mesg("%s", buffer);
   }

   fclose(fp);
}


void init_timer(int timer, void (*fnhandler) (int))
{
   signal(SIGALRM, fnhandler);
   alarm(timer);
}

char *xlate_history(LastXferCause code)
{
   switch (code) {
   case XFER_NONE:
      return "Power Up";
      break;
   case XFER_SELFTEST:
      return "Self Test";
      break;
   case XFER_UNDERVOLT:
      return "Line Voltage Decrease";
      break;
   case XFER_OVERVOLT:
      return "Line Voltage Increase";
      break;
   case XFER_NOTCHSPIKE:
      return "Power Failure";
      break;
   case XFER_FORCED:
      return "Forced";
      break;
   case XFER_RIPPLE:
      return "Ripple";
      break;
   default:
      return "Unknown Event";
      break;
   }
}

void restart_curses(void)
{
   close_curses();
   longjmp(env, RESTORE_BUFFER);
}

void update_all(void)
{
   if (!closing_curses) {
      wborder(menuwin, 0, 0, 0, 0, 0, 0, 0, 0);
      wborder(statwin, 0, 0, 0, 0, 0, 0, 0, 0);
      wborder(moniwin, 0, 0, 0, 0, 0, 0, 0, 0);
      wborder(mesgwin, 0, 0, 0, 0, 0, 0, 0, 0);
      box(mw, 0, 0);
   }

   update_panels();
   refresh();
   doupdate();
}

void restore_mesg(void)
{
   int y;

   /*
    * Restore the buffer.
    * Remember that the y must start at 1 to do not touch the border.
    * The screen start at 1 but the buffer start at 0.
    */
   for (y = 1; mesgbuf[y - 1][0] != '\0' && y < 9; y++)
      mvwprintw(mesgwin, y, 1, mesgbuf[y - 1]);

   wborder(mesgwin, 0, 0, 0, 0, 0, 0, 0, 0);
   update_all();
}

static void write_mesg(const char *fmt, ...)
{
   va_list args;
   static char buffer[1024];
   int x, y;

   memset(buffer, 0, sizeof(buffer));

   va_start(args, fmt);
   avsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
   va_end(args);

   buffer[strlen(buffer)] = '\n';

   getyx(mesgwin, y, x);

   /*
    * If we write the first line of window we must start at 1 because we
    * do not want to touch the nice border.
    */
   if (y == 0)
      y = 1;

   /*
    * At the low end of the window we scroll the entire content up one
    * line.
    */
   if (y == 9) {
      wscrl(mesgwin, 1);
      y = 8;
   }

   mvwprintw(mesgwin, y, 1, buffer);
   wborder(mesgwin, 0, 0, 0, 0, 0, 0, 0, 0);

   /* Save the last 8 lines of buffer. */

   /* If buffer full */
   if (mesgbuf[7][0]) {
      /* Scroll the buffer. */
      for (y = 1; y < 8; y++)
         astrncpy(mesgbuf[y - 1], mesgbuf[y], sizeof(mesgbuf[y - 1]));

      /* Now point to the last line of the mesgbuf. */
      y = 7;
   } else {
      /* Find the first free slot. */
      for (y = 0; mesgbuf[y][0]; y++)
         ;
   }

   /* y now point to a free slot. */
   astrncpy(mesgbuf[y], buffer, sizeof(mesgbuf[y]));
}

void update_upsdata(int sig)
{
   static int power_fail = FALSE;
   static int battery_fail = FALSE;
   time_t now;
   char *t;

   memcpy(ups, sharedups, sizeof(UPSINFO));

   get_raw_upsinfo(ups, host, port);

   time(&now);
   t = ctime(&now);
   t[strlen(t) - 1] = '\0';

   mvwprintw(statwin, 1, 1, "Last update: %s", t);
   mvwprintw(statwin, 2, 1, "Model      : %s", ups->mode.long_name);
   mvwprintw(statwin, 3, 1, "Serial No. : %s", ups->serial);
   mvwprintw(statwin, 4, 1, "Cable      : %s", ups->cable.long_name);
   mvwprintw(statwin, 5, 1, "Mode       : %s", ups->upsclass.long_name);
   mvwprintw(statwin, 6, 1, "AC Line    : %s",
      (ups->is_onbatt() ? "failing" : "okay"));
   mvwprintw(statwin, 7, 1, "Battery    : %s",
      (ups->is_battlow() ? "failing" : "okay"));
   mvwprintw(statwin, 8, 1, "AC Level   : %s",
      (ups->is_boost() ? "low" :
         (ups->is_trim() ? "high" : "normal")));
   mvwprintw(statwin, 9, 1, "Last event : %s", xlate_history(ups->lastxfer));

   mvwprintw(moniwin, 1, 1, "ACin");
   if (ups->UPS_Cap[CI_VLINE]) {
      int count;

      wprintw(moniwin, "  %03d [", ups->lotrans);
      mvwprintw(moniwin, 1, 32, "] %03d V", ups->hitrans);
      count = (int)(20 * (ups->LineVoltage - ups->lotrans) /
         (ups->hitrans - ups->lotrans));
      mvwprintw(moniwin, 1, 19, "%5.1f", ups->LineVoltage);
      mvwchgat(moniwin, 1, 12, count, A_REVERSE, 0, NULL);
   } else {
      wprintw(moniwin, ": N/A");
   }

   mvwprintw(moniwin, 2, 1, "ACout");
   if (ups->UPS_Cap[CI_VOUT]) {
      int count;

      wprintw(moniwin, " %03d [", ups->lotrans);
      mvwprintw(moniwin, 2, 32, "] %03d V", ups->hitrans);
      count = (int)(20 * (ups->OutputVoltage - ups->lotrans) /
         (ups->hitrans - ups->lotrans));
      mvwprintw(moniwin, 2, 19, "%5.1f", ups->OutputVoltage);
      mvwchgat(moniwin, 2, 12, count, A_REVERSE, 0, NULL);
   } else {
      wprintw(moniwin, ": N/A");
   }

   mvwprintw(moniwin, 3, 1, "Charge");
   if (ups->UPS_Cap[CI_BATTLEV]) {
      int count;

      wprintw(moniwin, "  0 [");
      mvwprintw(moniwin, 3, 32, "] 100 %%");
      mvwprintw(moniwin, 3, 19, "%5.0f", ups->BattChg);
      count = (int)(20 * ups->BattChg / 100);
      mvwchgat(moniwin, 3, 12, count, A_REVERSE, 0, NULL);
   } else {
      wprintw(moniwin, ": N/A");
   }

   mvwprintw(moniwin, 4, 1, "V Batt.");
   if (ups->UPS_Cap[CI_VBATT]) {
      int count;

      wprintw(moniwin, " 0 [");
      mvwprintw(moniwin, 4, 32, "] %2.1f", ups->nombattv);
      mvwprintw(moniwin, 4, 20, "%2.1f", ups->BattVoltage);
      count = (int)(20 * ups->BattVoltage / ups->nombattv);
      if (count > 20)
         count = 20;

      mvwchgat(moniwin, 4, 12, count, A_REVERSE, 0, NULL);
   } else {
      wprintw(moniwin, ": N/A");
   }

   mvwprintw(moniwin, 5, 1, "Load");
   if (ups->UPS_Cap[CI_LOAD]) {
      int count;

      wprintw(moniwin, "    0 [");
      mvwprintw(moniwin, 5, 32, "] 100 %%");
      mvwprintw(moniwin, 5, 19, "%5.0f", ups->UPSLoad);
      count = (int)(20 * ups->UPSLoad / 100);
      mvwchgat(moniwin, 5, 12, count, A_REVERSE, 0, NULL);
   } else {
      wprintw(moniwin, ": N/A");
   }

   mvwprintw(moniwin, 6, 1, "Runtime: ");
   if (ups->UPS_Cap[CI_RUNTIM])
      wprintw(moniwin, "%5.1f minutes", ups->TimeLeft);
   else
      wprintw(moniwin, "N/A");

   mvwprintw(moniwin, 7, 1, "Input line frequency: ");
   if (ups->UPS_Cap[CI_FREQ])
      wprintw(moniwin, "%2.1f Hz", ups->LineFreq);
   else
      wprintw(moniwin, "N/A");

   mvwprintw(moniwin, 8, 1, "Internal temperature: ");
   if (ups->UPS_Cap[CI_ITEMP])
      wprintw(moniwin, "%2.1f C", ups->UPSTemp);
   else
      wprintw(moniwin, "N/A");

   mvwprintw(moniwin, 9, 1, "Ambient temperature: ");
   if (ups->UPS_Cap[CI_ATEMP])
      wprintw(moniwin, "%2.1f C", ups->ambtemp);
   else
      wprintw(moniwin, "N/A");

   if (ups->is_boost())
      write_mesg("* [%s] warning: AC level is low", t);

   if (ups->is_trim())
      write_mesg("* [%s] warning: AC level is high", t);

   if (ups->is_onbatt()) {
      if (power_fail == FALSE) {
         write_mesg("* [%s] warning: power is failing", t);
         power_fail = TRUE;
      }
   } else {
      if (power_fail == TRUE) {
         write_mesg("* [%s] warning: power has returned", t);
         power_fail = FALSE;
      }
   }

   if (ups->is_battlow()) {
      if (battery_fail == FALSE) {
         write_mesg("* [%s] warning: battery is failing", t);
         power_fail = TRUE;
      }
   } else {
      if (battery_fail == TRUE) {
         write_mesg("* [%s] warning: battery is okay", t);
         battery_fail = FALSE;
      }
   }

   update_all();

   init_timer(TIMER_POWERFLUTE, update_upsdata);
}

static void close_curses(void)
{
   signal(SIGALRM, SIG_IGN);
   closing_curses = 1;
   del_panel(menupan);
   del_panel(statpan);
   del_panel(monipan);
   del_panel(mesgpan);
   delwin(menuwin);
   delwin(statwin);
   delwin(moniwin);
   delwin(mesgwin);
   werase(stdscr);
   update_all();
   endwin();
   detach_ups(sharedups);
}

static int menu_virtualize(int c)
{
   if (c == '\n' || c == KEY_EXIT)
      return (MAX_COMMAND + 1);
   else if (c == 'n' || c == KEY_DOWN)
      return (REQ_NEXT_ITEM);
   else if (c == 'p' || c == KEY_UP)
      return (REQ_PREV_ITEM);
   else if (c == ' ')
      return (REQ_TOGGLE_ITEM);
   else
      return (c);
}

char *do_menu(int *pos, ITEM ** it0, int pos_x)
{
   ITEM **it = it0;
   static char answer[80];
   int r, c;
   int mc;

   mu = new_menu(it0);
   scale_menu(mu, &r, &c);

   mw = newwin(r + 2, c + 2, 2, pos_x);
   mpan = new_panel(mw);
   set_menu_win(mu, mw);
   keypad(mw, TRUE);
   box(mw, 0, 0);
   top_panel(mpan);
   show_panel(mpan);
   dmw = derwin(mw, r, c, 1, 1);
   set_menu_sub(mu, dmw);
   post_menu(mu);

   answer[0] = '\0';

   do {
      mc = wgetch(mw);
      if ((mc == KEY_UP) && (item_index(current_item(mu)) == 0))
         goto out_nokey;

      if (mc == KEY_RIGHT) {
         *pos = (*pos + 1) % 3;
         goto out_nokey;
      }

      if (mc == KEY_LEFT) {
         *pos = (*pos + 2) % 3;
         goto out_nokey;
      }
   } while (menu_driver(mu, menu_virtualize(mc)) != E_UNKNOWN_COMMAND);

   astrncpy(answer, item_name(current_item(mu)), sizeof(answer));

out_nokey:
   unpost_menu(mu);

   del_panel(mpan);
   delwin(dmw);
   dmw = NULL;
   delwin(mw);
   mw = NULL;
   free_menu(mu);
   mu = NULL;

   for (it = it0; *it; it++)
      free_item(*it);

   return answer;
}

void do_file_menu(int *pos)
{
   ITEM *it0[64];
   ITEM **it = it0;
   char *answer;

   *it++ = new_item("Load", "No op");
   *it++ = new_item("Save", "No op");
   *it++ = new_item("Refresh", "screen");
   *it++ = new_item("Quit", "");
   *it = NULL;

   answer = do_menu(pos, it0, 0);
   if (answer[0] == '\0')
      return;

   if (!strcmp(answer, "Refresh")) {
      restart_curses();

      /* Never reached. */
   }

   if (!strcmp(answer, "Quit")) {
      close_curses();
      exit(0);
   }
}

void do_upscontrol_menu(int *pos)
{
   ITEM *it0[64];
   ITEM **it = it0;
   char *answer;

   *it++ = new_item("Test battery", "No op");
   *it++ = new_item("Test leds", "No op");
   *it++ = new_item("UPS shutdown", "No op");
   *it = NULL;

   answer = do_menu(pos, it0, 14);
   if (answer[0] == '\0')
      return;
}

void do_help_menu(int *pos)
{
   ITEM *it0[64];
   ITEM **it = it0;
   char *answer;

   *it++ = new_item("About powerflute", "");
   *it = NULL;

   answer = do_menu(pos, it0, 28);
   if (answer[0] == '\0')
      return;

   if (!strcmp(answer, "About powerflute")) {
      write_mesg("Powerflute, a program to monitor the UPS and "
         "control the apcupsd daemon");
      write_mesg("Written by Riccardo Facchetti <riccardo@master.oasi.gpa.it>");
   }
}

static const char *hm[] = {
   "File          ",
   "UPS           ",
   "Help          ",
   NULL
};

void display_h_menu(int pos)
{
   const char **p = hm;

   mvwprintw(menuwin, 1, 2, "%s", "");

   for (p = hm; *p != NULL; p++) {
      if (pos == (p - hm))
         wattron(menuwin, A_REVERSE);

      wprintw(menuwin, "%s", *p);
      if (pos == (p - hm))
         wattroff(menuwin, A_REVERSE);
   }

   update_all();
}

int do_horiz_menu(void)
{
   int pos = 0;
   int oldpos = 0;

   while (1) {

      display_h_menu(pos);

      switch (wgetch(menuwin)) {
      case KEY_LEFT:
      case 'h':
         pos = (pos + 2) % 3;
         break;

      case KEY_RIGHT:
      case 'l':
         pos = (pos + 1) % 3;
         break;

      case KEY_DOWN:
      case 10:                    /* RETURN */
       change_menu:
         oldpos = pos;
         switch (pos) {
         case 0:
            do_file_menu(&pos);
            break;
         case 1:
            do_upscontrol_menu(&pos);
            break;
         case 2:
            do_help_menu(&pos);
            break;
         default:
            break;
         }

         pos = pos % 4;

         if (oldpos != pos) {
            display_h_menu(pos);
            goto change_menu;
         }
         break;

      default:
         break;
      }
   }

   return SUCCESS;
}

int init_main_screen(void)
{
   /* Init ncurses subsystem */
   if (initscr() == NULL)
      goto out_err;

   if (LINES < 25 || COLS < 80)
      goto out_err;

   if (cbreak() == ERR)
      goto out_err;

   if (noecho() == ERR)
      goto out_err;

   /* Invisible cursor */
   curs_set(0);

   /* Build all windows and panels */
   menuwin = newwin(3, 0, 0, 0);
   keypad(menuwin, TRUE);
   menupan = new_panel(menuwin);
   wborder(menuwin, 0, 0, 0, 0, 0, 0, 0, 0);
   show_panel(menupan);

   statwin = newwin(12, 40, 3, 0);
   statpan = new_panel(statwin);
   wborder(statwin, 0, 0, 0, 0, 0, 0, 0, 0);
   show_panel(statpan);

   moniwin = newwin(12, 40, 3, 40);
   monipan = new_panel(moniwin);
   wborder(moniwin, 0, 0, 0, 0, 0, 0, 0, 0);
   show_panel(monipan);

   mesgwin = newwin(10, 0, 15, 0);
   mesgpan = new_panel(mesgwin);
   wborder(mesgwin, 0, 0, 0, 0, 0, 0, 0, 0);
   show_panel(mesgpan);
   scrollok(mesgwin, TRUE);

   update_all();

   return SUCCESS;

 out_err:
   close_curses();
   return FAILURE;
}

void powerflute_error_cleanup(void)
{
   detach_ups(sharedups);
   close_curses();
   exit(1);
}

int main(int argc, char **argv)
{
   int status;
   int flag;

   /* Set generic cleanup handler. */
   error_cleanup = powerflute_error_cleanup;
   ups = (UPSINFO *) malloc(sizeof(UPSINFO));

   if (!ups) {
      fprintf(stderr, "Out of memory.\n");
      exit(-1);
   }

   strncpy(argvalue, argv[0], sizeof(argvalue) - 1);
   argvalue[sizeof(argvalue) - 1] = 0;

   if (argc > 2) {                 /* assume host:port */
      char *p;

      host = argv[2];
      p = strchr(host, ':');
      if (p) {
         *p++ = 0;
         port = atoi(p);
      }
   }

   /* Do not move this line below the setjmp. */
   memset(mesgbuf, 0, 1024 * 8);

   /*
    * For refreshing the screen in very bad situations like when
    * someone broadcast a wall.
    */
   flag = setjmp(env);

   status = SUCCESS;
   closing_curses = 0;

   /* Attach daemon */
   sharedups = attach_ups(sharedups);
   if (!sharedups) {
      fprintf(stderr, "Can not attach SYSV IPC: "
         "Apcupsd not running or you are not root.\n");
      exit(1);
   }

   /* Init ncurses subsystem. */
   if (init_main_screen() == FAILURE) {
      if (LINES < 25 || COLS < 80)
         fprintf(stderr, "You must have at least 80 columns " "x 25 lines.\n");
      fprintf(stderr, "Error initializing ncurses subsystem\n");
      status = FAILURE;
      goto out;
   }

   /* If after the longjmp we need to restore the mesg buffer, do it now. */
   if (flag == RESTORE_BUFFER)
      restore_mesg();

   /* Update data immediately. */
   update_upsdata(0);

   load_events();
   do_horiz_menu();
   close_curses();

out:
   detach_ups(sharedups);
   return status;
}

#endif   /* HAVE_POWERFLUTE */
