/*
 *  apctest.c -- a cable tester program for apcupsd
 *	   Hacked from apcupsd.c by Kern Sibbald, Sept 2000
 *
 */

/*
 *			 GNU GENERAL PUBLIC LICENSE
 *			    Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			       675 Mass Ave, Cambridge, MA 02139, USA
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

#include "apc.h"

UPSINFO *core_ups;
UPSINFO *ups;

char argvalue[MAXSTRING];


int le_bit   = TIOCM_LE;
int st_bit   = TIOCM_ST;
int sr_bit   = TIOCM_SR;
int dtr_bit  = TIOCM_DTR;
int rts_bit  = TIOCM_RTS;
int cts_bit  = TIOCM_CTS;
int cd_bit   = TIOCM_CD;
int rng_bit  = TIOCM_RNG;
int dsr_bit  = TIOCM_DSR;

struct termios oldtio;
struct termios newtio;

/* Forward referenced functions */
static void do_dumb_testing();
static void test1();
static void test2();
static void test3();
static void test4();
static void test5();
static void test6();
static void guess();

static void do_smart_testing();
static void smart_test1();
static void smart_calibration();
static void monitor_calibration_progress();
static void terminate_calibration(int ask);

static void do_usb_testing();
static void usb_kill_power_test();


static void strip_trailing_junk(char *cmd);
static char *get_cmd(char *prompt);
static int write_file(char *buf);
void parse_eprom_cmds(char *eprom, char locale);
void print_valid_eprom_values(char *model);


/* Static variables */
static int normal, no_cable, no_power, low_batt;
static int test1_done = 0;
static int test2_done = 0;
static int test3_done = 0;
static int test4_done = 0;
static int test5_done = 0;


int shm_OK = 0;

/*
 * Print a message, and also write it to an output file
 */
static void pmsg(char *fmt,...)
{
    char      buf[3000];
    va_list   arg_ptr;

    va_start(arg_ptr, fmt);
    vsprintf(buf, (char *) fmt, arg_ptr);
    va_end(arg_ptr);
    printf(buf);
    write_file(buf);
}

/*
 * Write output into "log" file 
 */
static int write_file(char *buf)
{
    static int out_fd = -1;
    if (out_fd == -1) {
        out_fd = open("apctest.output", O_WRONLY|O_CREAT|O_APPEND, 0644);
	if (out_fd < 0) {
            printf("Could not create apctest.output: %s\n", strerror(errno));
	    return -1;
	}
    }
    return write(out_fd, buf, strlen(buf));
}

/*
 * Print out current time 
 */
static void ptime()
{
   char dt[MAXSTRING];
   time_t now = time(NULL);
   strftime(dt, MAXSTRING, "%Y-%m-%d %T ", localtime(&now));
   pmsg(dt);
}


/**********************************************************************
 * the terminate function and trapping signals allows apctest
 * to exit and cleanly close logfiles, and reset the tty back
 * to its original settings. You may want to add this
 * to all configurations.  Of course, the file descriptors
 * must be global for this to work, I also set them to
 * NULL initially to allow terminate to determine whether
 * it should close them.
 *********************************************************************/
void apctest_terminate(int sig)
{

    restore_signals();

    if (sig != 0) {
	 ptime();
         pmsg("apctest exiting, signal %u\n", sig);
    }

    clear_files();

    device_close(ups);

    delete_lockfile(ups);

    clean_threads();


    pmsg("apctest shutdown succeeded\n");
    closelog();
    destroy_ups(ups);
    _exit(0);
}

void apctest_error_cleanup(UPSINFO *ups)
{
    device_close(ups);
    delete_lockfile(ups);
    clean_threads();
    pmsg("apctest error shutdown completed\n");
    closelog();
    destroy_ups(ups);
    exit(1);
}

/*********************************************************************
 * subroutine error_out prints FATAL ERROR with file,
 *  line number, and the error message then cleans up 
 *  and exits. It is normally called from the Error_abort
 *  define, which inserts the file and line number.
 */
void apctest_error_out(char *file, int line, char *fmt,...)
{
    char      buf[256];
    va_list   arg_ptr;
    int       i;

    sprintf(buf, _("apctest FATAL ERROR in %s at line %d\n"), file, line);
    i = strlen(buf);
    va_start(arg_ptr, fmt);
    vsprintf((char *)&buf[i], (char *) fmt, arg_ptr);
    va_end(arg_ptr);
    fprintf(stderr, buf);
    pmsg(buf);
    apctest_error_cleanup(core_ups);		  /* finish the work */
}

/* subroutine error_exit simply prints the supplied error
 * message, cleans up, and exits
 */
void apctest_error_exit(char *fmt,...)
{
    char      buf[256];
    va_list   arg_ptr;

    va_start(arg_ptr, fmt);
    vsprintf(buf, (char *) fmt, arg_ptr);
    va_end(arg_ptr);
    pmsg(buf);
    apctest_error_cleanup(core_ups);		  /* finish the work */
}
	    

/*********************************************************************/
/*			 Main program.					   */
/*********************************************************************/

/*
 * This application must be linked as console app.
 */
#ifdef HAVE_CYGWIN
#undef main
#endif

#define M_SMART  1 
#define M_DUMB	 2
#define M_USB	 3
static int mode = M_DUMB;

int main (int argc, char *argv[]) {

    /*
     * Set specific error_* handlers.
     */
    error_out = apctest_error_out;
    error_exit = apctest_error_exit;

    /*
     * Default config file. If we set a config file in startup switches, it
     * will be re-filled by parse_options()
     */
    cfgfile = APCCONF;


#ifndef HAVE_SETPROCTITLE
    /* If there's not one in libc, then we have to use our own version
     * which requires initialization.
     */
    init_proctitle(argv[0]);
#endif

    ups = new_ups();		      /* get new ups */
    if (!ups) {
        Error_abort1(_("%s: init_ipc failed.\n"), argv[0]);
    }

    init_ups_struct(ups);
    core_ups = ups;		      /* this is our core ups structure */

    /*
     * parse_options is self messaging on errors, so we need only to exit()
     */
    if (parse_options(argc, argv))
	exit(1);

    pmsg("\n\n");
    ptime();
    pmsg("apctest " APCUPSD_RELEASE " (" DATE ") " APCUPSD_HOST"\n");

    pmsg("Checking configuration ...\n");
    check_for_config(ups, cfgfile);
    
    attach_driver(ups);
	 
    if (ups->driver == NULL) {
        Error_abort0(_("Apcupsd cannot continue without a valid driver.\n"));
    }

    pmsg("Attached to driver: %s\n", ups->driver->driver_name);        

    /* Print configuration */
    switch(ups->sharenet.type) {
	case DISABLE:
            pmsg("sharenet.type = DISABLE\n");
	    break;
	case SHARE:
            pmsg("sharenet.type = SHARE\n");
	    break;
	case NET:
            pmsg("sharenet.type = NET\n");

	    switch(ups->class.type) {
		case NO_CLASS:
                    pmsg("class.type = NO_CLASS\n");
		    break;
		case STANDALONE:
                    pmsg("class.type = STANDALONE\n");
		    break;
		case SHARESLAVE:
                    pmsg("class.type = SHARESLAVE\n");
		    break;
		case SHAREMASTER:
                    pmsg("class.type = SHAREMASTER\n");
		    break;
		case SHARENETMASTER:
                    pmsg("class.type = SHARENETMASTER\n");
		    break;
		case NETSLAVE:
                    pmsg("class.type = NETSLAVE\n");
		    break;
		case NETMASTER:
                    pmsg("class.type = NETMASTER\n");
		    break;
		default:
                    pmsg("class.type = DEFAULT\n");
		    break;

		}
	    break;
	case SHARENET:
            pmsg("sharenet.type = SHARENET\n");
            pmsg("I cannot handle sharenet.type = SHARENET\n");
	    apctest_terminate(1);
	default:
            pmsg("sharenet.type = DEFAULT\n");
            pmsg("I cannot handle sharenet.type = DEFAULT\n");
	    apctest_terminate(1);
    }

    switch (ups->cable.type) {
       case NO_CABLE:
          pmsg("cable.type = NO_CABLE\n");
	  break;
       case CUSTOM_SIMPLE:
          pmsg("cable.type = CUSTOM_SIMPLE\n");
	  break;
       case APC_940_0119A:
          pmsg("cable.type = APC_940_0119A (simple)\n");
      break;
       case APC_940_0127A:
          pmsg("cable.type = APC_940_0127A (simple)\n");
	  break;
       case APC_940_0128A:
          pmsg("cable.type = APC_940_0128A (simple)\n");
	  break;
       case APC_940_0020B:
          pmsg("cable.type = APC_940_0020B (simple)\n");
	  break;
       case APC_940_0020C:
          pmsg("cable.type = APC_940_0020C (simple)\n");
	  break;
       case APC_940_0023A:
          pmsg("cable.type = APC_940_0023A (simple)\n");
	  break;
       case CUSTOM_SMART:
          pmsg("cable.type = CUSTOM_SMART\n");
	  break;
       case APC_940_0024B:
          pmsg("cable.type = APC_940_0024B (smart)\n");
	  break;
       case APC_940_0024C:
          pmsg("cable.type = APC_940_0024C (smart)\n");
	  break;
       case APC_940_1524C:
          pmsg("cable.type = APC_940_1524C (smart)\n");
	  break;
       case APC_940_0024G:
          pmsg("cable.type = APC_940_0024G (smart)\n");
	  break;
       case APC_940_0095A:
          pmsg("cable.type = APC_940_0095A (smart)\n");
	  break;
       case APC_940_0095B:
          pmsg("cable.type = APC_940_0095B (smart)\n");
	  break;
       case APC_940_0095C:
          pmsg("cable.type = APC_940_0095C (smart)\n");
	  break;
       case APC_NET:
          pmsg("cable.type = APC_NET\n");
	  break;
       case USB_CABLE:
          pmsg("cable.type = USB_CABLE\n");
	  break;
       case MAM_CABLE:
          pmsg("cable.type = MAM_CABLE\n");
	  break;
       case APC_940_00XXX:
          pmsg("cable.type = APC_940_00XXX (unknown)\n");
	  break;
       default:
          pmsg("Unknown cable type: %d\n", ups->cable.type);
	  break;

    }
    if (ups->cable.type == USB_CABLE) {
       pmsg("\nYou are using a USB cable type, so I'm entering USB test mode\n");
       mode = M_USB;
    } else if (ups->cable.type >= CUSTOM_SMART) {
       pmsg("\nYou are using a SMART cable type, so I'm entering SMART test mode\n");
       mode = M_SMART;
    }

    switch (ups->mode.type) {
       case NO_UPS:
          pmsg("mode.type = NO_UPS\n");
	  break;
       case BK:
          pmsg("mode.type = BK\n");
	  break;
       case SHAREBASIC:
          pmsg("mode.type = SHAREBASIC\n");
	  break;
       case NETUPS:
          pmsg("mode.type = NETUPS\n");
	  break;
       case BKPRO:
          pmsg("mode.type = BKPRO\n");
	  break;
       case VS:
          pmsg("mode.type = VS\n");
	  break;
       case NBKPRO:
          pmsg("mode.type = NBKPRO\n");
	  break;
       case SMART:
          pmsg("mode.type = SMART\n");
	  break;
       case MATRIX:
          pmsg("mode.type = MATRIX\n");
	  break;
       case SHARESMART:
          pmsg("mode.type = SHARESMART\n");
	  break;
       case USB_UPS:
          pmsg("mode.type = USB_UPS\n");
	  break;
       case NETWORK_UPS:
          pmsg("mode.type = NETWORK_UPS\n");
	  break;
       case SNMP_UPS:
          pmsg("mode.type = SNMP_UPS\n");
	  break;
       default:
          pmsg("Unknown mode.type: %d\n", ups->mode.type);
    }
    if (ups->mode.type > SHAREBASIC) {
       if (mode != M_SMART && mode != M_USB) {
          pmsg("You specified a DUMB mode cable, but a SMART UPS. I give up.\n");
       }
    }


    delete_lockfile(ups);


    switch(ups->sharenet.type) {
	case DISABLE:
	case SHARE:
            pmsg("Setting up serial port ...\n");
	    setup_device(ups);
	    break;
	case NET:
	    switch(ups->class.type) {
		case NO_CLASS:
		case STANDALONE:
		case SHARESLAVE:
		case SHAREMASTER:
		case SHARENETMASTER:
		    break;
		case NETSLAVE:
		    if (kill_ups_power)
                        Error_abort0(_("Ignoring killpower for slave\n"));
		    if (prepare_slave(ups)) 
                        Error_abort0(_("Error setting up slave\n"));
		    break;
		case NETMASTER:
		    setup_device(ups);
		    if ((kill_ups_power == 0) && (prepare_master(ups)))
                        Error_abort0("Error setting up master\n");
		    break;
		default:
                    Error_abort1(_("NET Class Error %s\n\a"), strerror(errno));
	    }
	    break;
	case SHARENET:
	    setup_device(ups);
	    if ((kill_ups_power == 0) && (prepare_master(ups)))
                Error_abort0("Error setting up master.\n");
	    break;
	default:
            Error_abort0(_("Unknown share net type\n"));
    }

    if (kill_ups_power) {
        pmsg("apctest: bad option, I cannot do a killpower\n");
	apctest_terminate(0);
    }

    /*
     * Delete the lockfile just before fork. We will reacquire it just
     * after.
     */

    delete_lockfile(ups);

    make_pid_file();

    pmsg("Creating serial port lock file ...\n");
    if (create_lockfile(ups) == LCKERROR) {
        Error_abort1(_("failed to reacquire serial port lock file on device %s\n"), ups->device);
    }

    init_signals(apctest_terminate);

    if (ups->fd != -1) {
	if (mode == M_DUMB) {
           pmsg("Doing prep_device() ...\n");
	   prep_device(ups);
	}
        /* This isn't a documented option but can be used
	 * for testing dumb mode on a SmartUPS if you have
	 * the proper cable.
	 */
	if (dumb_mode_test) {
	    char ans[20];
            write(ups->fd, "R", 1);  /* enter dumb mode */
	    *ans = 0;
	    getline(ans, sizeof(ans), ups);
            pmsg("Going dumb: %s\n", ans);
	    mode = M_DUMB; /* run in dumb mode */
	}
    }

    shm_OK = 1;

    if (ups->fd != -1) {
       if (mode == M_DUMB) {
	  do_dumb_testing();
       } else if (mode == M_SMART) {
	  do_smart_testing();
       } else if (mode == M_USB) {
	  do_usb_testing();
       } else {
          pmsg("USB testing not yet implemented.\n");
       }

    } else {
       pmsg("apctest: there is a problem here.  We have no serial port.\n");
    }

    apctest_terminate(0);
    return -1;			    /* to keep compiler happy */
}

static void
print_bits(int bits)
{
      char buf[200];

      sprintf(buf, "IOCTL GET: %x ", bits);
      if (bits & le_bit)
         strcat(buf, "LE ");
      if (bits & st_bit)
         strcat(buf, "ST ");
      if (bits & sr_bit)
         strcat(buf, "SR ");
      if (bits & dtr_bit)
         strcat(buf, "DTR ");
      if (bits & rts_bit)
         strcat(buf, "RTS ");
      if (bits & cts_bit)
         strcat(buf, "CTS ");
      if (bits & cd_bit)
         strcat(buf, "CD ");
      if (bits & rng_bit)
         strcat(buf, "RNG ");
      if (bits & dsr_bit)
         strcat(buf, "DSR ");
      strcat(buf, "\n");
      pmsg(buf);
}

static void
do_dumb_testing()
{
   int quit = FALSE;
   char *cmd;

   pmsg("Hello, this is the apcupsd Cable Test program.\n\n\
We are beginning testing for dumb UPSes, which\n\
use signaling rather than commands.\n\
Most tests enter a loop polling every second for 10 seconds.\n");

   while (!quit) {
      pmsg( "\n\
1) Test 1 - normal mode\n\
2) Test 2 - no cable\n\
3) Test 3 - no power\n\
4) Test 4 - low battery (requires test 3 first)\n\
5) Test 5 - battery exhausted\n\
6) Test 6 - kill UPS power\n\
7) Test 7 - run tests 1 through 5\n\
8) Guess which is the appropriate cable\n\
9) Quit\n\n");

      cmd = get_cmd("Select test number: ");
      if (cmd) {
	 int item = atoi(cmd);
	 switch (item) {
	    case 1:
	       test1();
	       break;
	    case 2:
	       test2();
	       break;
	    case 3:
	       test3();
	       break;
	    case 4:
	       test4();
	       break;
	    case 5:
	       test5();
	       break;
	    case 6:
	       test6();
	       break;
	    case 7:
	       test1();
	       test2();
	       test3();
	       test4();
	       test5();
	       break;
	    case 8:
	       guess();
	       break;
	    case 9:
	       quit = TRUE;
	       break;
	    default:
               pmsg("Illegal response. Please enter 1-9\n");
	       break;
	 }
      } else {
         pmsg("Illegal response. Please enter 1-9\n");
      }
   }
   ptime();
   pmsg("End apctest.\n");
}

/*
 * Poll 10 times once per second and report any 
 * change in serial port bits.
 */
static int test_bits(int inbits)
{
   int i, nbits;
   int bits = inbits;

   for (i=0; i < 10; i++) {
      if (ioctl(ups->fd, TIOCMGET, &nbits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
	 return 0;
      }
      if (i == 0 || nbits != bits) {
	 ptime();
	 print_bits(nbits);
	 bits = nbits;
      }
      sleep(1);
   }
   return bits;
}

static void test1()
{
   pmsg("\nFor the first test, everything should be normal.\n\
The UPS should be plugged in to the power, and the serial cable\n\
should be connected to the computer.\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");
 
   normal = test_bits(0);

   ptime();
   pmsg("Test 1: normal condition, completed.\n");
   test1_done = TRUE;
}

static void test2()
{
   pmsg("\nFor the second test, the UPS should be plugged in to the power, \n\
but the serial port should be DISCONNECTED from the computer.\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   no_cable = test_bits(0);
 
   ptime();
   pmsg("Test 2: no cable, completed. \n");
   test2_done = TRUE;
}

static void test3()
{
   pmsg("\nFor the third test, the serial cable should be plugged\n\
back into the UPS, but the AC power plug to the UPS should be DISCONNECTED.\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");
 
   no_power = test_bits(0);

   ptime();
   pmsg("Test 3: no power, completed.\n");
   test3_done = TRUE;
}

static void test4()
{
   int i, bits;

   if (!test3_done) {
      pmsg("We need the output of test 3 to run this test.\n\
Please run test 3 first then this test.\n");
      return;
   }

   pmsg("\nThe fourth test is the same as the third test:\n\
the serial cable should be plugged in to the UPS, but the AC power\n\
plug to the UPS should be DISCONNECTED. In addition, you should\n\
continue this test until the batteries are exhausted.\n\
If apctest detects a state change, it will stop\n\
the test. If not, hit control-C to stop the program\n\n\
PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   low_batt = no_power;

   ptime();
   pmsg("Start test 4: ");
   pmsg("\n");

   /* Spin until we get a state change */
   for (i=0; ; i++) {
      if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
	 return;
      }
      if (bits != low_batt) {
	 ptime();
	 print_bits(bits);
	 low_batt = bits;
	 break;
      } else if (i == 0) {
	 ptime();
	 print_bits(bits);
      }
      sleep(1);
   }

   ptime();
   pmsg("Test 4: low battery, completed.\n");
   test4_done = TRUE;
}

static void test5()
{
   int i, bits, last_bits = 0;

   pmsg("\nThe fifth test is the same as the third test:\n\
the serial cable should be plugged in to the UPS, but the AC power\n\
plug to the UPS should be DISCONNECTED. In addition, you should\n\
continue this test until the batteries are exhausted.\n\
If apctest detects a state change, contrary to test 4, it\n\
will not stop. The idea is to see ANY changed bits just up to\n\
the very moment that the UPS powers down.\n\n\
PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   pmsg("Start test 5:\n");

   /* Spin until we get a state change */
   for (i=0; ; i++) {
      if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
	 return;
      }
      if (i == 60) {
	 i = 0; 		      /* force print once a minute */
      }
      if (i == 0 || bits != last_bits) {
	 ptime();
	 print_bits(bits);
	 last_bits = bits;
      }
      sleep(1);
   }

   /* Should never get here */
   /* NOTREACHED */
   ptime();
   pmsg("Test 5: battery exhausted, completed.\n");
   test5_done = TRUE;
}

static void test6()
{
   int bits;

   pmsg("\nThis test will attempt to power down the UPS.\n\
The serial cable should be plugged in to the UPS, but the\n\
AC power plug to the UPS should be DISCONNECTED.\n\n\
PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");
   if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
      pmsg("ioctl error, big problem: %s\n", strerror(errno));
      return;
   }
   ptime();
   print_bits(bits);
   make_file(ups, PWRFAIL);
   kill_power(ups);
   unlink(PWRFAIL);
   ptime();
   pmsg("returned from kill_power function.\n");
   if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
      pmsg("ioctl error, big problem: %s\n", strerror(errno));
      return;
   }
   ptime();
   print_bits(bits);
}

/*
 * Make a wild guess at the cable type 
 *
 *  If I had more data on each of the cable types, this could
 *  be much improved.
 */
static void guess()
{
   int found = 0;

   if (!(test1_done && test3_done)) {
      pmsg("Test 1 and test 3 must be performed before I can make a guess.\n");
      return;
   }
   if (!(normal & (cd_bit|cts_bit)) && (no_power & cd_bit) && (low_batt & cts_bit)) {
      pmsg("This looks like a CUSTOM_SIMPLE cable\n");
      found = 1;
   }

   if (!(normal & (cd_bit|cts_bit)) && (no_power & cts_bit) && (low_batt & cd_bit)) {
      pmsg("This looks like a 940-0020A\n");                   
      found = 1;
   }

   if (!(normal & (cd_bit|sr_bit)) && (no_power & cd_bit) && (low_batt & sr_bit)) {
      pmsg("This looks like a 940-0023A cable\n");
      found = 1;
   }

   if (!(normal & (rng_bit|cd_bit)) && (no_power & rng_bit) && (low_batt & cd_bit)) {
      pmsg("This looks like a 940-0095A cable\n");
      found = 1;
   }

   if (!found) {
      pmsg("Hmmm. I don't quite know what you have. Sorry.\n"); 
   }
}

static void do_smart_testing()
{
   char *cmd;
   int quit = FALSE;

   pmsg("Hello, this is the apcupsd Cable Test program.\n\
This part of apctest is for testing Smart UPSes.\n\
Please select the function you want to perform.\n");

   while (!quit) {
      pmsg( "\n\
1) Query the UPS for all known values\n\
2) Perform a Battery Runtime Calibration\n\
3) Abort Battery Calibration\n\
4) Monitor Battery Calibration progress\n\
5) Quit\n\n");

      cmd = get_cmd("Select function number: ");
      if (cmd) {
	 int item = atoi(cmd);
	 switch (item) {
	    case 1:
	       smart_test1();
	       break;
	    case 2:
	       smart_calibration();
	       break;
	    case 3:
	       terminate_calibration(1);
	       break;
	    case 4:
	       monitor_calibration_progress();
	       break;
	    case 5:
	       quit = TRUE;
	       break;
	    default:
               pmsg("Illegal response. Please enter 1-5\n");
	       break;
	 }
      } else {
         pmsg("Illegal response. Please enter 1-5\n");
      }
   }
   ptime();
   pmsg("End apctest.\n");
}

static void smart_calibration()
{
   char *ans, cmd;
   char answer[2000];
   int stat;

   pmsg("First ensure that we have a good link and \n\
that the UPS is functionning normally.\n");
   pmsg("Simulating UPSlinkCheck ...\n");
   tcflush(ups->fd, TCIOFLUSH);
   /* Start really simply */
   cmd = 'Y';
   stat = write(ups->fd, &cmd, 1);
   if (stat < 0) {
     pmsg("Bad status from write: %s\n", strerror(errno));
   }
   stat = getline(answer, sizeof(answer), ups);
   pmsg("Wrote: Y Got: %s\n", answer);
   if (stat == FAILURE) {
      pmsg("getline failed. Apparently the link is not up.\n\
Giving up.\n");
      return;
   }
   pmsg("Attempting to use smart_poll() ...\n");
   ans = smart_poll('Y', ups);
   pmsg("Sent: Y Got: %s  ", ans);
   if (strcmp(ans, "SM") == 0) {
      pmsg("Good -- smart_poll() works!.\n\n");
   } else {
      pmsg("Not good.\nGiving up.\n");
      return;
   }
   pmsg("Checking estimated runtime ...\n");
   ans = smart_poll('j', ups);
   if (*ans >= '0' && *ans <= '9') {
      int rt = atoi(ans);
      pmsg("Current runtime is %d minutes\n", rt);
   } else {
      pmsg("Unexpected response from UPS: %s\n", ans);
      return;
   }
   pmsg("Checking for battery level ...\n");
   ans = smart_poll('f', ups);
   if (strncmp(ans, "100.0", 5) == 0) {
      pmsg("Battery level is 100.0 -- OK\n");
   } else {
      pmsg("Battery level %s insufficient to run test.\n", ans);
      return;
   }
   pmsg("\nSending Battery Calibration command. The calibration\n\
should automatically end when the battery level\n\
drops below 35-25 depending on your UPS. In any\n\
case, I will stop the calibration if it reaches\n\
20. If you wish to stop the calibration, enter a return.\n\n");

   ans = smart_poll('D', ups);
   if (*ans == '!' || strcmp(ans, "OK") == 0) {
      pmsg("UPS has initiated battery calibration.\n");
   } else {
      pmsg("Unexpected response from UPS: %s\n", ans);
      return;
   }
   monitor_calibration_progress();
}

static void terminate_calibration(int ask)
{
   char *ans, *cmd;

   if (ask) {
      pmsg("\nCAUTION! Don't use this command unless the UPS\n\
is already doing a calibration.\n");
      cmd = get_cmd("Are you sure? (yes/no): ");
      if (strcmp(cmd, "yes") != 0) {
	 return;
      }
   }
   pmsg("\nAttempting to abort calibration ...\n");
   ans = smart_poll('D', ups);  /* abort calibration */
   if (*ans == '$' || strcmp(ans, "NO") == 0) {
      pmsg("Battery Runtime Calibration terminated.\n");
      pmsg("Checking estimated runtime ...\n");
      ans = smart_poll('j', ups);
      if (*ans >= '0' && *ans <= '9') {
	 int rt = atoi(ans);
         pmsg("Updated runtime is %d\n", rt);
      } else {
         pmsg("Unexpected response from UPS: %s\n", ans);
      }
   } else {
      pmsg("Response to abort request: %s\n", ans);
   }
}


static void monitor_calibration_progress()
{
   char *ans;
   int count = 0;
   int max_count = 10;
   char period = '.';

   pmsg("Monitoring the calibration progress ...\n\
To stop the calibration, enter a return.\n");
   for ( ;; ) {
      fd_set rfds;
      struct timeval tv;
      int retval, percent;
      char cmd;

      FD_ZERO(&rfds);
      FD_SET(ups->fd, &rfds);
      FD_SET(STDIN_FILENO, &rfds);
      tv.tv_sec = 12;
      tv.tv_usec = 0;
      errno = 0;
      retval = select((ups->fd)+1, &rfds, NULL, NULL, &tv);
      switch (retval) {
      case 0:
	 write(STDOUT_FILENO, &period, 1);
	 if (++count >= max_count) {
            ans = smart_poll('f', ups);   /* Get battery charge */
	    percent = (int)strtod(ans, NULL);
            pmsg("\nBattery charge %d\n", percent);
	    if (percent > 0) {
	       if (percent <= 20) {
		  terminate_calibration(0);
		  return;
	       }
	       if (percent < 40) {
		  max_count = 3;      /* report faster */
	       }
	    }
            ans = smart_poll('j', ups);  /* Get runtime */
            if (*ans >= '0' && *ans <= '9') {
	       int rt = atoi(ans);
               pmsg("Remaining runtime is %d minutes\n", rt);
	       if (rt > 0 && rt < 4) {
		  terminate_calibration(0);
		  return;
	       }
	    }
	    count = 0;
	 }
	 continue;
      case -1:
	 if (errno == EINTR || errno == EAGAIN) {
	    continue;
	 }
         pmsg("Select error. ERR=%s\n", strerror(errno));
	 return;
      default:
	 break;
      }
      if (FD_ISSET(STDIN_FILENO, &rfds)) {
	 read(STDIN_FILENO, &cmd, 1);
	 terminate_calibration(0);
	 return;
      }
      if (FD_ISSET(ups->fd, &rfds)) {
	 read(ups->fd, &cmd, 1);
         if (cmd == '$') {
            pmsg("Battery Runtime Calibration terminated.\n");
            pmsg("Checking estimated runtime ...\n");
            ans = smart_poll('j', ups);
            if (*ans >= '0' && *ans <= '9') {
	       int rt = atoi(ans);
               pmsg("Remaining runtime is %d minutes\n", rt);
	    } else {
               pmsg("Unexpected response from UPS: %s\n", ans);
	    }
	    return;
	 } else {
            pmsg("UPS sent: %c\n", cmd);
	 }
      }
   }
}



static void smart_test1()
{
   char *ans, *p, *o, cmd;
   char answer[2000];
   char parts[2000];
   int stat, i;
#ifdef working
   char locale, locale1, locale2;
#endif

   pmsg("I am going to run through the series of queries of the UPS\n\
that are used in initializing apcupsd.\n\n");

   pmsg("Simulating UPSlinkCheck ...\n");
   tcflush(ups->fd, TCIOFLUSH);
   /* Start really simply */
   cmd = 'Y';
   stat = write(ups->fd, &cmd, 1);
   if (stat < 0) {
     pmsg("Bad status from write: %s\n", strerror(errno));
   }
   stat = getline(answer, sizeof(answer), ups);
   pmsg("Wrote: Y Got: %s\n", answer);
   if (stat == FAILURE) {
      pmsg("getline failed. Apparently the link is not up.\n\
Giving up.\n");
      return;
   }
   pmsg("Attempting to use smart_poll() ...\n");
   ans = smart_poll('Y', ups);
   pmsg("Sent: Y Got: %s  ", ans);
   if (strcmp(ans, "SM") == 0) {
      pmsg("Good -- smart_poll() works!.\n\n");
   } else {
      pmsg("Not good.\nGiving up.\n");
      return;
   }
   pmsg("Going to ask for valid commands...\n");
   cmd = 'a';
   stat = write(ups->fd, &cmd, 1);
   if (stat != 1) {
      pmsg("Bad response from write: %d %s\n", stat, strerror(errno));
   }
   *answer = 0;
   for (i=0; i < sizeof(answer); i++) {
      stat = read(ups->fd, &cmd, 1);
      if (stat < 0) {
         pmsg("Bad response from read: %s\n", strerror(errno));
      }
      if (cmd == '\n') {
	 answer[i] = 0;
	 break;
      }
      answer[i] = cmd;
   }
   /* Get protocol version */
   for (p=answer, o=parts; *p && *p != '.'; p++) {
      *o++ = *p;     
   }
   *o = 0;
   pmsg("Protocol version is: %s\n", parts);
   if (*p == '.') {
      p++;
   }
   /* Get alert characters */
   for (o=parts; *p && *p != '.'; p++) {
      if (*p < 0x20) {
         *o++ = '^';
         *o++ = *p + 'A' - 1;
      } else {
	 *o++ = *p;
      }
   }
   *o = 0;
   pmsg("Alert characters are: %s\n", parts);
   if (*p == '.') {
      p++;
   }
   /* Get command characters */
   for (o=parts; *p; p++) {
      if (*p < 0x20) {
         *o++ = '^';
         *o++ = *p + 'A' - 1;
      } else {
	 *o++ = *p;
      }
   }
   *o = 0;
   pmsg("Command characters are: %s\n", parts);
   pmsg("\nNow running through apcupsd get_UPS capabilities().\n\
NA  indicates that the feature is Not Available\n\n");

   pmsg("UPS Status: %s\n", smart_poll(ups->UPS_Cmd[CI_STATUS], ups));

   pmsg("Line quality: %s\n", smart_poll(ups->UPS_Cmd[CI_LQUAL], ups));

   pmsg("Reason for last transfer to batteries: %s\n", 
	smart_poll(ups->UPS_Cmd[CI_WHY_BATT], ups));

   pmsg("Self-Test Status: %s\n", smart_poll(ups->UPS_Cmd[CI_ST_STAT], ups));

   pmsg("Line Voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VLINE], ups));

   pmsg("Line Voltage Max: %s\n", smart_poll(ups->UPS_Cmd[CI_VMAX], ups));

   pmsg("Line Voltage Min: %s\n", smart_poll(ups->UPS_Cmd[CI_VMIN], ups));

   pmsg("Output Voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VOUT], ups));

   pmsg("Batt level percent: %s\n", smart_poll(ups->UPS_Cmd[CI_BATTLEV], ups));

   pmsg("Batt voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VBATT], ups));

   pmsg("UPS Load: %s\n", smart_poll(ups->UPS_Cmd[CI_LOAD], ups));

   pmsg("Line freq: %s\n", smart_poll(ups->UPS_Cmd[CI_FREQ], ups));

   pmsg("Runtime left: %s\n", smart_poll(ups->UPS_Cmd[CI_RUNTIM], ups));

   pmsg("UPS Internal temp: %s\n", smart_poll(ups->UPS_Cmd[CI_ITEMP], ups));

   pmsg("Dip switch settings: %s\n", smart_poll(ups->UPS_Cmd[CI_DIPSW], ups));

   pmsg("Register 1: %s\n", smart_poll(ups->UPS_Cmd[CI_REG1], ups));

   pmsg("Register 2: %s\n", smart_poll(ups->UPS_Cmd[CI_REG2], ups));

   pmsg("Register 3: %s\n", smart_poll('8', ups));

   pmsg("Sensitivity: %s\n", smart_poll(ups->UPS_Cmd[CI_SENS], ups));

   pmsg("Wakeup delay: %s\n", smart_poll(ups->UPS_Cmd[CI_DWAKE], ups));

   pmsg("Sleep delay: %s\n", smart_poll(ups->UPS_Cmd[CI_DSHUTD], ups));

   pmsg("Low transfer voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_LTRANS], ups));

   pmsg("High transfer voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_HTRANS], ups));

   pmsg("Batt charge for return: %s\n", smart_poll(ups->UPS_Cmd[CI_RETPCT], ups));

   pmsg("Alarm status: %s\n", smart_poll(ups->UPS_Cmd[CI_DALARM], ups));

   pmsg("Low battery shutdown level: %s\n", smart_poll(ups->UPS_Cmd[CI_DLBATT], ups));

   pmsg("UPS Name: %s\n", smart_poll(ups->UPS_Cmd[CI_IDEN], ups));

   pmsg("UPS Self test interval: %s\n", smart_poll(ups->UPS_Cmd[CI_STESTI], ups));

   pmsg("UPS manufacture date: %s\n", smart_poll(ups->UPS_Cmd[CI_MANDAT], ups));
	
   pmsg("UPS serial number: %s\n", smart_poll(ups->UPS_Cmd[CI_SERNO], ups));

   pmsg("Date battery replaced: %s\n", smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups));

   pmsg("Output voltage when on batteries: %s\n", smart_poll(ups->UPS_Cmd[CI_NOMOUTV], ups)); 

   pmsg("Nominal battery voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_NOMBATTV], ups)); 

   pmsg("Percent humidity: %s\n", smart_poll(ups->UPS_Cmd[CI_HUMID], ups));

   pmsg("Ambient temperature: %s\n", smart_poll(ups->UPS_Cmd[CI_ATEMP], ups));

   pmsg("Firmware revision: %s\n", smart_poll(ups->UPS_Cmd[CI_REVNO], ups));

   pmsg("Number of external batteries installed: %s\n", smart_poll(ups->UPS_Cmd[CI_EXTBATTS], ups));

   pmsg("Number of bad batteries installed: %s\n", smart_poll(ups->UPS_Cmd[CI_BADBATTS], ups));
	
   pmsg("UPS model as defined by UPS: %s\n", smart_poll(ups->UPS_Cmd[CI_UPSMODEL], ups));

   pmsg("UPS EPROM capabilities string: %s\n", (ans=smart_poll(ups->UPS_Cmd[CI_EPROM], ups)));
   pmsg("The EPROM string is %d characters long!\n", strlen(ans));

   pmsg("Hours since last self test: %s\n", smart_poll(ups->UPS_Cmd[CI_ST_TIME], ups));

#ifdef working
   pmsg("get_UPS_capabilities() simulation done. Now running it.\n\n");

   get_UPS_capabilities(ups);
   fillUPS(ups);

   pmsg("Analysis of EEPROM settings\n");

   if (!ups->UPS_Cap[CI_EPROM]) {
      pmsg("Sorry, your model does not support EPROM programming.\n");
      return;
   }
   if (ups->UPS_Cap[CI_REVNO])
      locale1 = *(ups->firmrev + strlen(ups->firmrev) - 1);
   else  
      locale1 = 0;
   if (ups->UPS_Cap[CI_UPSMODEL])
      locale2 = *(ups->upsmodel + strlen(ups->upsmodel) - 1);
   else
      locale2 = 0;
   if (locale1 == locale2 && locale1 == 0) {
      pmsg("Sorry, your model does not support EPROM programming.\n");
      return;
   }
   if (locale1 == locale2)
       locale = locale1;
   if (locale1 == 0)
       locale = locale2;
   else 
       locale = locale1;
   parse_eprom_cmds(ups->eprom, locale);
   print_valid_eprom_values(ups->mode.long_name);
#endif
       
   pmsg("\nThat is all for now.\n");
   return;
}

/* EPROM commands and their values as parsed from the
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

static UPSINFO eeprom_ups;

/* Table of the UPS command, the apcupsd configuration directive,
 *  and an explanation of what the command sets in the EPROM.
 */
static struct {
 char cmd;
 char *config_directive;
 char *descript;
 char type;
 int *data;
} cmd_table[] = {
  {'u', "HITRANSFER",    "Upper transfer voltage", 'i', &eeprom_ups.hitrans},
  {'l', "LOTRANSFER",    "Lower transfer voltage", 'i', &eeprom_ups.lotrans},
  {'e', "RETURNCHARGE",  "Return threshold", 'i', &eeprom_ups.rtnpct},
  {'o', "OUTPUTVOLTS",   "Output voltage on batts", 'i', &eeprom_ups.NomOutputVoltage},
  {'s', "SENSITIVITY",   "Sensitivity", 'c', (int *)eeprom_ups.sensitivity},
  {'q', "LOWBATT",       "Low battery warning", 'i', &eeprom_ups.dlowbatt},
  {'p', "SLEEP",         "Shutdown grace delay", 'i', &eeprom_ups.dshutd},
  {'k', "BEEPSTATE",     "Alarm delay", 'c', (int *)eeprom_ups.beepstate},
  {'r', "WAKEUP",        "Wakeup delay", 'i', &eeprom_ups.dwake},
  {'E', "SELFTEST",      "Self test interval", 'c', (int *)eeprom_ups.selftest},
  {0, NULL, NULL}	 /* Last entry */
  };

void print_valid_eprom_values(char *model)
{
    int i, j, k, l;
    char *p;
    char val[10];

    pmsg("\nValid EPROM values for the %s\n\n", model);

    pmsg("%-24s %-12s  %-6s  %s\n", "           ", "Config",   "Current", "Permitted");
    pmsg("%-24s %-12s  %-6s  %s\n", "Description", "Directive","Value  ", "Values");
    pmsg("===================================================================\n");
    for (i=0; i<ncmd; i++) {
       for (j=0; cmd_table[j].cmd; j++) {
	  if (cmd[i].cmd == cmd_table[j].cmd) {
             if (cmd_table[j].type == 'c') 
                sprintf(val, "%s", (char *)cmd_table[j].data);
	     else
                sprintf(val, "%d", *cmd_table[j].data);
             pmsg("%-24s %-12s  %-6s   ", cmd_table[j].descript, 
		     cmd_table[j].config_directive, val);
	     p = cmd[i].cmdvals;
	     for (k=cmd[i].num; k; k--) {
		for (l=cmd[i].size; l; l--)
                   pmsg("%c", *p++);
                pmsg(" ");
	     }
             pmsg("\n");
	     break;
	  }
       }
    }
    pmsg("\n");
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
       l = *p++;		  /* get locale */
       n = *p++ - '0';            /* get number of commands */
       s = *p++ - '0';            /* get character size */
       if (l != '4' && l != locale) {  /* skip this locale */
	  p += n * s;
	  continue;
       }
       cmd[ncmd].cmd = c;	   /* store command */
       cmd[ncmd].size = s;	    /* chare length of each value */
       cmd[ncmd].num = n;	   /* number of values */
       strncpy(cmd[ncmd].cmdvals, p, n*s); /* save values */
       p += n * s;
       ncmd++;
    }
#ifdef debuggggggg
        pmsg("\n");
	for (i=0; i<ncmd; i++) 
          pmsg("cmd=%c len=%d nvals=%d vals=%s\n", cmd[i].cmd,
	     cmd[i].size, cmd[i].num, cmd[i].cmdvals);
#endif
}
    
static void do_usb_testing()
{
   char *cmd;
   int quit = FALSE;

   pmsg("Hello, this is the apcupsd Cable Test program.\n\
This part of apctest is for testing USB UPSes.\n\
Please select the function you want to perform.\n");

   while (!quit) {
      pmsg( "\n\
1) Test 1 - kill UPS power\n\
2) Quit\n\n");

      cmd = get_cmd("Select function number: ");
      if (cmd) {
	 int item = atoi(cmd);
	 switch (item) {
	    case 1:
	       usb_kill_power_test();
	       break;
	    case 2:
	       quit = TRUE;
	       break;
	    default:
               pmsg("Illegal response. Please enter 1-2\n");
	       break;
	 }
      } else {
         pmsg("Illegal response. Please enter 1-2\n");
      }
   }
   ptime();
   pmsg("End apctest.\n");
}


static void usb_kill_power_test()
{
   pmsg("\nThis test will attempt to power down the UPS.\n\
The USB cable should be plugged in to the UPS, but the\n\
AC power plug to the UPS should be DISCONNECTED.\n\n\
PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n\
Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");
   ptime();
   pmsg("calling kill_power function.\n");
   make_file(ups, PWRFAIL);
   kill_power(ups);
   unlink(PWRFAIL);
   ptime();
   pmsg("returned from kill_power function.\n");
}


/*
 * Gen next input command from the terminal
 */
static char *get_cmd(char *prompt)
{
   static char cmd[1000];

   pmsg(prompt);
   if (fgets(cmd, sizeof(cmd), stdin) == NULL)
      return NULL;
   write_file(cmd);
   pmsg("\n");
   strip_trailing_junk(cmd);
   return cmd;
}

/* Strip any trailing junk from the command */
static void strip_trailing_junk(char *cmd)
{
   char *p;
   p = cmd + strlen(cmd) - 1;

   /* strip trailing junk from command */
   while ((p >= cmd) && (*p == '\n' || *p == '\r' || *p == ' '))
      *p-- = 0;
}
