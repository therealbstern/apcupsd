/*
 *  apceeprom.c -- Do APC EEPROM changes.
 *
 */

#include "apc.h"
#include "apcsmart.h"

static void change_ups_battery_date(UPSINFO *ups);
static void change_ups_name(UPSINFO *ups);
static void change_extended(UPSINFO *ups);


/*********************************************************************/
/*  apcsmart_ups_program_eeprom()					      */
/*********************************************************************/

int apcsmart_ups_program_eeprom(UPSINFO *ups)
{
    if (update_battery_date) {
        printf(_("Attempting to update UPS battery date ...\n"));
	setup_device(ups);
	apcsmart_ups_get_capabilities(ups);
	if (ups->UPS_Cap[CI_BATTDAT]) {
	    change_ups_battery_date(ups);
	}
	device_close(ups);
    }

    if (configure_ups) {
        printf(_("Attempting to configure UPS ...\n"));
	setup_device(ups);
	change_extended(ups);	      /* set new values in UPS */
        printf("\nReading updated UPS configuration ...\n\n");
	device_read_volatile_data(ups);
	device_read_static_data(ups);
	/* Print report of status */
	output_status(ups, 0, stat_open, stat_print, stat_close);
    }

    if (rename_ups) {
        printf(_("Attempting to rename UPS ...\n"));
	setup_device(ups);
	apcsmart_ups_get_capabilities(ups);
	if (ups->UPS_Cap[CI_IDEN]) {
	    change_ups_name(ups);
	}
    }

    return 1;
}

/*********************************************************************/
static void change_ups_name(UPSINFO *ups)
{
    char *n;
    char response[32];
    char name[9];
    char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
    char c = ups->UPS_Cmd[CI_IDEN];
    int i;
    int j = strlen(ups->upsname);
    name[0] = '\0';

    if (j == 0) {
        fprintf(stderr, "Error, new name of zero length.\n");
	return;
    } else if (j > 8)			  
	j = 8;			    /* maximum size */

    strncpy(name, ups->upsname, j);
    /* blank fill to 8 chars */
    while (j < 8) {
        name[j] = ' ';
	j++;
    }

    /* Ask for name */
    write(ups->fd, &c, 1);  /* c = 'c' */
    getline(response, sizeof(response), ups);
    fprintf(stderr, "The old UPS name is: %s\n", response);

    /* Tell UPS we will change name */
    write(ups->fd, &a, 1);  /* a = '-' */
    sleep(1);

    n = name;
    for (i=0; i<8; i++) {
	write(ups->fd, n++, 1);
	sleep(1);
    }

    /* Expect OK after successful name change */
    *response = 0;
    getline(response, sizeof(response), ups);
    if (strcmp(response, "OK") != 0)
        fprintf(stderr, "\nError changing UPS name\n");

    ups->upsname[0] = '\0';
    smart_poll(ups->UPS_Cmd[CI_IDEN], ups);
    strcpy(ups->upsname, smart_poll(ups->UPS_Cmd[CI_IDEN], ups));

    fprintf(stderr, "The new UPS name is: %s\n", ups->upsname);
}

/********************************************************************* 
 * update date battery replaced
 */
static void change_ups_battery_date(UPSINFO *ups)
{
    char *n;
    char response[32];
    char battdat[9];
    char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
    char c = ups->UPS_Cmd[CI_BATTDAT];
    int i;
    int j = strlen(ups->battdat);

    battdat[0] = '\0';

    if (j != 8) {
        fprintf(stderr, "Error, new battery date must be 8 characters long.\n");
	return;
    }

    strcpy(battdat, ups->battdat);

    /* Ask for battdat */
    write(ups->fd, &c, 1);  /* c = 'x' */
    getline(response, sizeof(response), ups);
    fprintf(stderr, "The old UPS battery date is: %s\n", response);

    /* Tell UPS we will change battdat */
    write(ups->fd, &a, 1);  /* a = '-' */
    sleep(1);

    n = battdat;
    for (i=0; i<8; i++) {
	write(ups->fd, n++, 1);
	sleep(1);
    }

    /* Expect OK after successful battdat change */
    *response = 0;
    getline(response, sizeof(response), ups);
    if (strcmp(response, "OK") != 0)
        fprintf(stderr, "\nError changing UPS battery date\n");

    ups->battdat[0] = '\0';
    smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups);
    strcpy(ups->battdat, smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups));

    fprintf(stderr, "The new UPS battery date is: %s\n", ups->battdat);
}

/*********************************************************************/
int setup_ups_item(UPSINFO *ups, char *title, char cmd, char *setting)
{
    char response[32];
    char response1[32];
    char oldvalue[32];
    char lastvalue[32];
    char allvalues[256];
    char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
    int i;


    /* Ask for old value */
    write(ups->fd, &cmd, 1);
    if (getline(oldvalue, sizeof(oldvalue), ups) == FAILURE) {
        fprintf(stderr, "Could not get old value of %s.\n", title);
	return FAILURE;
    }
    if (strcmp(oldvalue, setting) == 0) {
        fprintf(stderr, "The UPS %s remains unchanged as: %s\n", title, oldvalue);
	return SUCCESS;
    }
    fprintf(stderr, "The old UPS %s is: %s\n", title, oldvalue);
    strcpy(allvalues, oldvalue);
    strcat(allvalues, " ");
    strcpy(lastvalue, oldvalue);

    /* Try a second time to ensure that it is a stable value */
    write(ups->fd, &cmd, 1);
    *response = 0;
    getline(response, sizeof(response), ups);
    if (strcmp(oldvalue, response) != 0) {
        fprintf(stderr, "\nEEPROM value of %s is not stable\n", title);
	return FAILURE;
    }

    /* Just before entering this loop, the last command sent
     * to the UPS MUST be to query the old value.   
     */
    for (i=0; i<10; i++) {
	write(ups->fd, &cmd, 1);
	getline(response1, sizeof(response1), ups);

	/* Tell UPS to cycle to next value */
        write(ups->fd, &a, 1);  /* a = '-' */

	/* Expect OK after successful change */
	*response = 0;
	getline(response, sizeof(response), ups);
        if (strcmp(response, "OK") != 0) {
            fprintf(stderr, "\nError changing UPS %s\n", title);
            fprintf(stderr, "Got %s instead of OK\n\n", response);
	    sleep(10);
	    return FAILURE;
	}

	/* get cycled value */
	write(ups->fd, &cmd, 1);
	getline(response1, sizeof(response1), ups);

	/* get cycled value again */
	write(ups->fd, &cmd, 1);
	if (getline(response, sizeof(response), ups) == FAILURE ||
	     strcmp(response1, response) != 0) {
            fprintf(stderr, "Error cycling values.\n");
	    getline(response, sizeof(response), ups); /* eat any garbage */
	    return FAILURE;
	}
	if (strcmp(setting, response) == 0) {
            fprintf(stderr, "The new UPS %s is: %s\n", title, response);
	    sleep(10);		   /* allow things to settle down */
	    return SUCCESS;
	}

	/* Check if we cycled back to the same value, but permit
	 * a duplicate because the L for sensitivy appears
	 * twice in a row, i.e. H M L L.
	 */
	if (strcmp(oldvalue, response) == 0 && i > 0)
	    break;
	if (strcmp(lastvalue, response) != 0) {
	    strcat(allvalues, response);
            strcat(allvalues, " ");
	    strcpy(lastvalue, response);
	}
        sleep(5);             /* don't cycle too fast */
    }
    fprintf(stderr, "Unable to change %s to: %s\n", title, setting);
    fprintf(stderr, "Permitted values are: %s\n", allvalues);
    getline(response, sizeof(response), ups); /* eat any garbage */
    return FAILURE;
}


/********************************************************************* 
 *
 * Set new values in EEPROM memmory.  Change the UPS EEPROM !!!!!!!!
 *
 */
static void change_extended(UPSINFO *ups)
{
    char setting[20];

    apcsmart_ups_get_capabilities(ups);

    /* Note, a value of -1 in the variable at the beginning
     * means that the user did not put a configuration directive
     * in /etc/apcupsd/apcupsd.conf. Consequently, if no value
     * was given, we won't attept to change it.
     */

    /* SENSITIVITY */
    if (ups->UPS_Cap[CI_SENS] && strcmp(ups->sensitivity, "-1") != 0) {
        sprintf(setting, "%.1s", ups->sensitivity);
        setup_ups_item(ups, "sensitivity", ups->UPS_Cmd[CI_SENS], setting);
    }
     
    /* WAKEUP_DELAY */
    if (ups->UPS_Cap[CI_DWAKE] && ups->dwake != -1) {
        sprintf(setting, "%03d", (int)ups->dwake);
        setup_ups_item(ups, "wakeup delay", ups->UPS_Cmd[CI_DWAKE], setting);
    }

   
    /* SLEEP_DELAY */
    if (ups->UPS_Cap[CI_DSHUTD] && ups->dshutd != -1) {
        sprintf(setting, "%03d", (int)ups->dshutd);
        setup_ups_item(ups, "shutdown delay", ups->UPS_Cmd[CI_DSHUTD], setting);
    }

    /* LOW_TRANSFER_LEVEL */
    if (ups->UPS_Cap[CI_LTRANS] && ups->lotrans != -1) {
        sprintf(setting, "%03d", (int)ups->lotrans);
        setup_ups_item(ups, "lower transfer voltage",
	       ups->UPS_Cmd[CI_LTRANS], setting);
    }

    /* HIGH_TRANSFER_LEVEL */
    if (ups->UPS_Cap[CI_HTRANS] && ups->hitrans != -1) {
        sprintf(setting, "%03d", (int)ups->hitrans);
        setup_ups_item(ups, "upper transfer voltage",
	       ups->UPS_Cmd[CI_HTRANS], setting);
    }

    /* UPS_BATT_CAP_RETURN */
    if (ups->UPS_Cap[CI_RETPCT] && ups->rtnpct != -1) {
        sprintf(setting, "%02d", (int)ups->rtnpct);
        setup_ups_item(ups, "return threshold percent",
	       ups->UPS_Cmd[CI_RETPCT], setting);
    }

    /* ALARM_STATUS */
    if (ups->UPS_Cap[CI_DALARM] && strcmp(ups->beepstate, "-1") != 0) {
        sprintf(setting, "%.1s", ups->beepstate);
        setup_ups_item(ups, "alarm delay",
	       ups->UPS_Cmd[CI_DALARM], setting);
    }

    /* LOWBATT_SHUTDOWN_LEVEL */
    if (ups->UPS_Cap[CI_DLBATT] && ups->dlowbatt != -1) {
        sprintf(setting, "%02d", (int)ups->dlowbatt);
        setup_ups_item(ups, "low battery warning delay",
	       ups->UPS_Cmd[CI_DLBATT], setting);
    }

    /* UPS_SELFTEST */
    if (ups->UPS_Cap[CI_STESTI] && strcmp(ups->selftest, "-1") != 0) {
        sprintf(setting, "%.3s", ups->selftest);
        /* Make sure "ON" is 3 characters */
	if (setting[2] == 0) {
            setting[2] = ' ';
	    setting[3] = 0;
	}
        setup_ups_item(ups, "self test interval", ups->UPS_Cmd[CI_STESTI], setting);
    }

    /* OUTPUT_VOLTAGE */
    if (ups->UPS_Cap[CI_NOMOUTV] && ups->NomOutputVoltage != -1) {
        sprintf(setting, "%03d", (int)ups->NomOutputVoltage);
        setup_ups_item(ups, "output voltage on batteries",
	       ups->UPS_Cmd[CI_NOMOUTV], setting);
    }
}
