/*
 *  operations.c -- Functions for UPS operations
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
#include "dumb.h"

int dumb_ups_kill_power(UPSINFO *ups) 
{
    int serial_bits = 0;

    switch(ups->cable.type) {
	case CUSTOM_SIMPLE:	  /* killpwr_bit */
	case APC_940_0095A:
	case APC_940_0095B:
	case APC_940_0095C:	 /* killpwr_bit */
	    serial_bits = TIOCM_RTS;
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    serial_bits = TIOCM_ST;
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    break;

	case APC_940_0119A:
	case APC_940_0127A:
	case APC_940_0128A:
	case APC_940_0020B:	  /* killpwr_bit */
	case APC_940_0020C:
	    serial_bits = TIOCM_DTR;
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    break;

	case APC_940_0023A:	 /* killpwr_bit */
	    break;

	case MAM_CABLE:
	    serial_bits = TIOCM_RTS;
	    (void) ioctl(ups->fd, TIOCMBIC, &serial_bits);
	    serial_bits = TIOCM_DTR;
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    (void) ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    break;

	case CUSTOM_SMART:
	case APC_940_0024B:
	case APC_940_0024C:
	case APC_940_1524C:
	case APC_940_0024G:
	case APC_NET:
	default:
	    break;
    }
    return 1;
}

/*
 * Dumb UPSes don't have static UPS data.
 */
int dumb_ups_read_static_data(UPSINFO *ups) 
{
    return 1;
}

/*
 * Set capabilities.
 */
int dumb_ups_get_capabilities(UPSINFO *ups) 
{
    ups->UPS_Cap[CI_STATUS] = TRUE;   /* We create a Status word */
    return 1;
}

/*
 * dumb_ups_check_state is the same as dumb_read_ups_volatile_data
 * because this is the only info we can get from UPS.
 *
 *  This routine is polled. We should sleep at least 5 seconds
 *  unless we are in a FastPoll situation, otherwise, we burn
 *  too much CPU.
 */
int dumb_ups_read_volatile_data(UPSINFO *ups) 
{
    int stat = 1;
    int BattFail = 0;

    /* We generally poll a bit faster because we do 
     *	not have interrupts like the smarter devices
     */
    if (ups->wait_time > TIMER_DUMB) {
        ups->wait_time = TIMER_DUMB;
    }
    sleep(ups->wait_time);

    write_lock(ups);

    ioctl(ups->fd, TIOCMGET, &ups->sp_flags);
    switch(ups->mode.type) {
    case BK:
    case SHAREBASIC: 
    case NETUPS:
        if (ups->sp_flags & TIOCM_DTR) {
            BattFail = 1;
        } else {
            BattFail = 0;
        }
       break;
    default:
       break;
    }

    switch(ups->cable.type) {
	case CUSTOM_SIMPLE:
        /*
         * This is the ONBATT signal sent by UPS.
         */
	    if (ups->sp_flags & TIOCM_CD) {
            UPS_SET(UPS_ONBATT);
        } else {
            UPS_CLEAR(UPS_ONBATT);
        }
        /*
         * This is the ONLINE signal that is delivered
         * by CUSTOM_SIMPLE cable. We use the UPS_ONLINE flag
         * to report this condition to apcaction.
         * If we are both ONBATT and ONLINE there is clearly
         * something wrong with battery or charger. Set also the
         * UPS_REPLACEBATT flag if needed.
         */
        if (ups->sp_flags & TIOCM_SR) {
            UPS_CLEAR(UPS_ONLINE);
        } else {
            UPS_SET(UPS_ONLINE);
        }
        if (UPS_ISSET(UPS_ONLINE) && UPS_ISSET(UPS_ONBATT)) {
            BattFail = 1;
        } else {
            BattFail = 0;
        }
        if (!(ups->sp_flags & TIOCM_CTS)) {
            UPS_SET(UPS_BATTLOW);
        } else {
            UPS_CLEAR(UPS_BATTLOW);
        }
	    break;

	case APC_940_0119A:
	case APC_940_0127A:
	case APC_940_0128A:
	case APC_940_0020B:
	case APC_940_0020C:
	    if (ups->sp_flags & TIOCM_CTS) {
            UPS_CLEAR_ONLINE();
        } else {
            UPS_SET_ONLINE();
        }
        if (ups->sp_flags & TIOCM_CD) {
            UPS_SET(UPS_BATTLOW);
        } else {
            UPS_CLEAR(UPS_BATTLOW);
        }
	    break;

	case APC_940_0023A:
	    if (ups->sp_flags & TIOCM_CD) {
            UPS_CLEAR_ONLINE();
        } else {
            UPS_SET_ONLINE();
        }

	    /* BOGUS STUFF MUST FIX IF POSSIBLE */

        if (ups->sp_flags & TIOCM_SR) {
            UPS_SET(UPS_BATTLOW);
        } else {
            UPS_CLEAR(UPS_BATTLOW);
        }
	    break;

	case APC_940_0095A:
	case APC_940_0095C:
	    if (ups->sp_flags & TIOCM_RNG) {
            UPS_CLEAR_ONLINE();
        } else {
            UPS_SET_ONLINE();
        }
        if (ups->sp_flags & TIOCM_CD) {
            UPS_SET(UPS_BATTLOW);
        } else {
            UPS_CLEAR(UPS_BATTLOW);
        }
	    break;

	case APC_940_0095B:
	    if (ups->sp_flags & TIOCM_RNG) {
            UPS_CLEAR_ONLINE();
        } else {
            UPS_SET_ONLINE();
        }
	    break;

	case MAM_CABLE:
        if (!(ups->sp_flags & TIOCM_CTS)) {
            UPS_CLEAR_ONLINE();
        } else {
            UPS_SET_ONLINE();
        }
        if (!(ups->sp_flags & TIOCM_CD)) {
            UPS_SET(UPS_BATTLOW);
        } else {
            UPS_CLEAR(UPS_BATTLOW);
        }
        break;

	case CUSTOM_SMART:
	case APC_940_0024B:
	case APC_940_0024C:
	case APC_940_1524C:
	case APC_940_0024G:
	case APC_NET:
	default:
        stat = 0;
    }

    if (UPS_ISSET(UPS_ONBATT) && BattFail) {
        UPS_SET(UPS_REPLACEBATT);
    } else {
        UPS_CLEAR(UPS_REPLACEBATT);
    }

    write_unlock(ups);

    return stat;
}

int dumb_ups_program_eeprom(UPSINFO *ups) 
{
#if 0
    printf(_("This model cannot be configured.\n"));
#endif
    return 0;
}

int dumb_ups_entry_point(UPSINFO *ups, int command, void *data) 
{
    int serial_bits = 0;

    switch(command) {
	case DEVICE_CMD_DTR_ENABLE:
	    if (ups->cable.type == CUSTOM_SIMPLE) {
		/* 
		 * A power failure just happened.
		 *
		 * Now enable the DTR for the CUSTOM_SIMPLE cable
		 * Note: this enables the the CTS bit, which allows
		 * us to detect the UPS_BATTLOW condition!!!!
		 */
		serial_bits = TIOCM_DTR;
		(void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
	    }
	    break;

	case DEVICE_CMD_DTR_ST_DISABLE:
	    if (ups->cable.type == CUSTOM_SIMPLE) {
		/* 
		 * Mains power just returned.
		 *
		 * Clearing DTR and TxD (ST bit).
		 */
		serial_bits = TIOCM_DTR | TIOCM_ST;
/* Leave it set */
/*		(void)ioctl(ups->fd, TIOCMBIC, &serial_bits);
 */
	    }
	    break;
	default:
	    return 0;
	    break;
    }
    return 1;
}
