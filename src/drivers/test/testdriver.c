/*
 *
 *  Interface for apcupsd to Test driver.
 *
 *   Kern Sibbald, September MMI 
 *
 */


#include "apc.h"

#include <asm/types.h>

extern UPSCOMMANDS cmd[];
extern UPSCMDMSG cmd_msg[];


/*
 */
static int open_test_device(UPSINFO *ups)
{
   ups->fd = 1;
   return 1;
}


/*
 * Read UPS events. I.e. state changes.
 */
int test_ups_check_state(UPSINFO *ups)
{
    return 1;
}

/*
 * Open test port
 */
int test_ups_open(UPSINFO *ups)
{
    write_lock(ups);
    if (!open_test_device(ups)) {
        Error_abort1(_("Cannot open UPS device %s\n"),
			ups->device);
    }
    ups->ups_connected = 1;
    write_unlock(ups);
    return 1;
}

int test_ups_setup(UPSINFO *ups) {
    /*
     * Seems that there is nothing to do.
     */
    return 1;
}

int test_ups_close(UPSINFO *ups) 
{
    write_lock(ups);
    /*
     * Seems that there is nothing to do.
     */
    ups->fd = -1;
    write_unlock(ups);
    return 1;
}

/*
 * Setup capabilities structure for UPS
 */
int test_ups_get_capabilities(UPSINFO *ups)
{
    int k;


    write_lock(ups);
    for (k=0; k <= CI_MAX_CAPS; k++) {
       ups->UPS_Cap[k] = TRUE;
    }
    write_unlock(ups);
    return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 *   voltages, shutdown delay, ...
 *
 *  This routine is called once when apcupsd is starting
 */
int test_ups_read_static_data(UPSINFO *ups)
{
    write_lock(ups);
    /* UPS_NAME */

    /* model, firmware */
    strcpy(ups->upsmodel, "Test Driver");
    strcpy(ups->firmrev, "Rev 1.0");
    strcpy(ups->selftestmsg, "Test Battery OK");
    strcpy(ups->selftest, "336");

    /* WAKEUP_DELAY */
    ups->dwake = 2 * 60;

    /* SLEEP_DELAY */
    ups->dshutd = 2 * 60;

    /* LOW_TRANSFER_LEVEL */
    ups->lotrans = 190;

    /* HIGH_TRANSFER_LEVEL */
    ups->hitrans = 240;

    /* UPS_BATT_CAP_RETURN */
    ups->rtnpct = 15;

    /* LOWBATT_SHUTDOWN_LEVEL */
    ups->dlowbatt = 2;

    /* UPS_MANUFACTURE_DATE */
    strcpy(ups->birth, "2001-09-21");

    /* Last UPS_BATTERY_REPLACE */
    strcpy(ups->battdat, "2001-09-21");
 
    /* UPS_SERIAL_NUMBER */
    strcpy(ups->serial, "NO-123456");

    /* Nominal output voltage when on batteries */
    ups->NomOutputVoltage = 230;

    /* Nominal battery voltage */
    ups->nombattv = (double)12;
    write_unlock(ups);
    return 1;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every 5 seconds to get
 *  a current idea of what the UPS is doing.
 */
int test_ups_read_volatile_data(UPSINFO *ups)
{

    time(&ups->poll_time);	  /* save time stamp */

    write_lock(ups);

    /* UPS_STATUS -- this is the most important status for apcupsd */

    /* No APC Status value, well, fabricate one */
    ups->Status = 0;
    ups->Status |= UPS_ONLINE;

    /* LINE_VOLTAGE */
    ups->LineVoltage = 229.5;
    ups->LineMin = 225.0;
    ups->LineMax = 230.0;

    /* OUTPUT_VOLTAGE */
    ups->OutputVoltage = 228.5;

    /* BATT_FULL Battery level percentage */
    ups->BattChg = 100;

    /* BATT_VOLTAGE */
    ups->BattVoltage = 12.5;

    /* UPS_LOAD */
    ups->UPSLoad = 40.5;

    /* LINE_FREQ */
    ups->LineFreq = 50;

    /* UPS_RUNTIME_LEFT */
    ups->TimeLeft = ((double)20*60);   /* seconds */

    /* UPS_TEMP */
    ups->UPSTemp = 32.4;

    /*	Humidity percentage */ 
    ups->humidity = 50.1;

    /*	Ambient temperature */ 
    ups->ambtemp = 22.5;

    /* Self test results */
    strcpy(ups->X, "OK");
    write_unlock(ups);

    return 1;
}

int test_ups_kill_power(UPSINFO *ups)
{
   /* Not implemented yet */
   return 0;
}

int test_ups_program_eeprom(UPSINFO *ups) {
    return 0;
}

int test_ups_entry_point(UPSINFO *ups, int command, void *data) {
    return 0;
}
