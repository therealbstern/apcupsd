/*
 *  setup.c -- Functions to open/setup/close the device
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *
 *  apcupsd.c	-- Simple Daemon to catch power failure signals from a
 *		   BackUPS, BackUPS Pro, or SmartUPS (from APCC).
 *		-- Now SmartMode support for SmartUPS and BackUPS Pro.
 *
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *  Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 *  All rights reserved.
 *
 */

/*
 *		       GNU GENERAL PUBLIC LICENSE
 *			  Version 2, June 1991
 *
 *  Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *			     675 Mass Ave, Cambridge, MA 02139, USA
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

#include "apc.h"
#include "apcsmart.h"


/*
 * This is the first routine in the driver that is called.
 */
int apcsmart_ups_open(UPSINFO *ups) {
    int cmd;
    SMART_DATA *my_data = (SMART_DATA *)ups->driver_internal_data;

    if (my_data == NULL) {
       my_data = (SMART_DATA *)malloc(sizeof(SMART_DATA));
       if (my_data == NULL) {
          log_event(ups, LOG_ERR, "Out of memory.");
	  exit(1);
       }
       memset(my_data, 0, sizeof(SMART_DATA));
       ups->driver_internal_data = my_data;
    } else {
       log_event(ups, LOG_ERR, "apcsmart_ups_open called twice. This shouldn't happen.");
    }

    /*
     * For a slave, we skip actually setting up the serial port
     */
    if (is_ups_set(UPS_SLAVE)) {
       ups->fd = -1;
       return 1;
    }

    if ((ups->fd = open(ups->device, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        Error_abort2(_("Cannot open UPS port %s: %s\n"),
		ups->device, strerror(errno));
    }

    /* Cancel the no delay we just set */
    cmd = fcntl(ups->fd, F_GETFL, 0);
    fcntl(ups->fd, F_SETFL, cmd & ~O_NDELAY);

    tcgetattr(ups->fd, &my_data->oldtio); /* Save old settings */

    my_data->newtio.c_cflag = DEFAULT_SPEED | CS8 | CLOCAL | CREAD;
    my_data->newtio.c_iflag = IGNPAR;	      /* Ignore errors, raw input */
    my_data->newtio.c_oflag = 0;	  /* Raw output */
    my_data->newtio.c_lflag = 0;	  /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
		defined(HAVE_FREEBSD_OS) || defined(HAVE_NETBSD_OS)
    my_data->newtio.c_ispeed = DEFAULT_SPEED;	  /* Set input speed */
    my_data->newtio.c_ospeed = DEFAULT_SPEED;	  /* Set output speed */
#endif /* __openbsd__ || __freebsd__ || __netbsd__  */

 /* w.p. This makes a non.blocking read() with TIMER_READ (10) sec. timeout */ 
    my_data->newtio.c_cc[VMIN] = 0;
    my_data->newtio.c_cc[VTIME] = TIMER_READ * 10;

#if defined(HAVE_CYGWIN) || defined(HAVE_OSF1_OS) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
    cfsetospeed(&my_data->newtio, DEFAULT_SPEED);
    cfsetispeed(&my_data->newtio, DEFAULT_SPEED);
#endif /* do it the POSIX way */

    tcflush(ups->fd, TCIFLUSH);
    tcsetattr(ups->fd, TCSANOW, &my_data->newtio);
    tcflush(ups->fd, TCIFLUSH);

    return 1;
}

/*
 * This routine is the last one called before apcupsd
 * terminates.
 */
int apcsmart_ups_close(UPSINFO *ups)
{
    SMART_DATA *my_data = (SMART_DATA *)ups->driver_internal_data;
    
    if (my_data == NULL) {
        return SUCCESS;               /* shouldn't happen */
    }
    /* Reset serial line to old values */
    if (ups->fd >= 0) {
       tcflush(ups->fd, TCIFLUSH);
       tcsetattr(ups->fd, TCSANOW, &my_data->oldtio);
       tcflush(ups->fd, TCIFLUSH);

       close(ups->fd);
    }
    ups->fd = -1;
    free(ups->driver_internal_data);
    ups->driver_internal_data = NULL;

    return 1;
}

int apcsmart_ups_setup(UPSINFO *ups) {
    int attempts;
    int rts_bit = TIOCM_RTS;
    char a = 'Y';

    if (ups->fd == -1) {
       return 1;		      /* we must be a slave */
    }

    /* The following enables communcations with the
     * BackUPS Pro models in Smart Mode.
     */
    switch(ups->cable.type) {
	case APC_940_0095A:
	case APC_940_0095B:
	case APC_940_0095C:
	    /*
	     * Have to clear RTS line to access the serial cable mode PnP
	     */
	    (void)ioctl(ups->fd, TIOCMBIC, &rts_bit);
	    break;

	default:
	    break;
    }

    write(ups->fd, &a, 1);	  /* This one might not work, if UPS is */
    sleep(1);		  /* in an unstable communication state */
    tcflush(ups->fd, TCIOFLUSH);  /* Discard UPS's response, if any */

    /* Don't use smart_poll here because it may loop waiting
     * on the serial port, and here we may be called before
     * we are a deamon, so we want to error after a reasonable
     * time.
     */
    for (attempts=0; attempts < 5; attempts++) {
	char answer[10];

	*answer = 0;
	write(ups->fd, &a, 1); /* enter smart mode */
	getline(answer, sizeof(answer), ups);
        if (strcmp("SM", answer) == 0)
	    goto out;
	sleep(1);
    }
    Error_abort0(_("PANIC! Cannot communicate with UPS via serial port.\n\
Please make sure the port specified on the DEVICE directive is correct,\n\
and that your cable specification on the UPSCABLE directive is correct.\n"));

out:
    return 1;
}
