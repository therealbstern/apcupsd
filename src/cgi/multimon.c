/* multimon - CGI program to monitor several UPSes from one page

   Copyright (C) 1998  Russell Kroll <rkroll@exploits.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		  
 */

#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "multimon.h"
#include "upsfetch.h"
#include "cgiconfig.h"
/* #include "version.h" */
#include "status.h"

#include "apc_nis.h"


typedef struct {
	char	*var;
	char	*name;
	char	*suffix;
	void	*next;
}	ftype;

static	ftype	*firstfield = NULL;
static	int	numfields = 0, use_celsius;
static	char	*desc;

void report_error(char *str) 
{
    printf("%s</BODY></HTML>\n", str);
    exit(1);
}

void noresp(void)
{
    printf ("<TD BGCOLOR=\"#FF0000\">Unavailable</TD>\n");
}

/* handler for "SYSTEM" */
void do_system(char *monhost, char *suffix)
{
    /* provide system name and link to upsstats */
    printf ("<TD BGCOLOR=\"#00FFFF\">");
    printf ("<A HREF=\"upsstats.cgi?host=%s&amp;temp=%c\">%s</A></TD>\n", monhost, 
             use_celsius?'C':'F', desc);
}

/* handler for "STATUS" */
void do_status(char *monhost, char *suffix)
{
    char    status[64], *stat, color[16], stattxt[128], temp[128];
    int     i, severity;
    long    ups_status;

    if (getupsvar (monhost, "status", status, sizeof(status)) <= 0) {
	    noresp();
	    return;
    }

    strcpy (stattxt, "");
    severity = 0;

    ups_status = strtol(status, 0, 16);
    status[0] = 0;
    if (ups_status & UPS_CALIBRATION) 
        strcat(status, "CAL ");
    if (ups_status & UPS_SMARTTRIM)
        strcat(status, "TRIM ");
    if (ups_status & UPS_SMARTBOOST)
        strcat(status, "BOOST ");
    if (ups_status & UPS_ONLINE)
        strcat(status, "OL ");
    if (ups_status & UPS_ONBATT) 
        strcat(status, "OB ");
    if (ups_status & UPS_OVERLOAD)
        strcat(status, "OVER ");
    if (ups_status & UPS_BATTLOW) 
        strcat(status, "LB ");
    if (ups_status & UPS_REPLACEBATT)
        strcat(status, "RB ");
    if (ups_status & UPS_COMMLOST)
        strcat(status, "CL ");
    if (ups_status & UPS_SHUTDOWN)
        strcat(status, "SD ");
    if (ups_status & UPS_SLAVE)
        strcat(status, "SLAVE ");

    stat = strtok (status, " ");
    while (stat != NULL) {  
	for (i = 0; stattab[i].name != NULL; i++) {
		if (!strcmp(stattab[i].name, stat)) {
                        snprintf (temp, sizeof(temp), "%s %s<BR>",
				  stattxt, stattab[i].desc);
                        snprintf (stattxt, sizeof(stattxt), "%s",
				  temp);
			if (stattab[i].severity > severity)
				severity = stattab[i].severity;
		}
	}
        stat = strtok (NULL, " ");
    }

    /* severity: green/yellow/red for ok, warning, bad */

    /* obviously this is not universal for all cultures! */

    switch (severity) {
    case 0: strcpy (color, "#00FF00"); break;   /* normal = green */
    case 1: strcpy (color, "#FFFF00"); break;   /* warning = yellow */
    case 2: strcpy (color, "#FF0000"); break;   /* severe = red */
    }

    printf ("<TD BGCOLOR=\"%s\">%s</TD>\n", color, stattxt);
}

/* handler for "MODEL" */
void do_model (char *monhost, char *suffix)
{
    char    model[256];

    if (getupsvar(monhost, "model", model, sizeof(model)) < 0) {
	return;
    }

    printf ("<TD BGCOLOR=\"#00FFFF\">%s</TD>\n", model);
    return;
}

/* handler for "DATA" */
void do_fulldata (char *monhost, char *suffix)
{
    printf ("<TD BGCOLOR=\"#00FFFF\">");
    printf ("<A HREF=\"upsfstats.cgi?host=%s\">%s</A></TD>\n", monhost, suffix);
    return;
}


/* handler for "UPSTEMP" */
void do_upstemp(char *monhost, char *suffix)
{
    char    upstemp[64];
    float   tempf;

    if (getupsvar (monhost, "upstemp", upstemp, sizeof(upstemp)) > 0) {
	if (use_celsius)
            printf ("<TD BGCOLOR=\"#00FF00\">%s &deg;C</TD>\n", upstemp); 
	else {
	    tempf = (strtod(upstemp, 0) * 1.8) + 32;
            printf ("<TD BGCOLOR=\"#00FF00\">%.1f &deg;F</TD>\n", tempf); 
	}
    } else {
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
    }
}

/* handler for "UPSTEMPC" */
void do_upstempc(char *monhost, char *suffix)
{
    char    upstemp[64];

    if (getupsvar(monhost, "upstemp", upstemp, sizeof(upstemp)) > 0)
        printf ("<TD BGCOLOR=\"#00FF00\">%s &deg;C</TD>\n", upstemp); 
    else
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
}

/* handler for "UPSTEMPF" */
void do_upstempf(char *monhost, char *suffix)
{
    char    upstemp[64];
    float   tempf;

    if (getupsvar(monhost, "upstemp", upstemp, sizeof(upstemp)) > 0) {
	tempf = (strtod (upstemp, 0) * 1.8) + 32;
        printf ("<TD BGCOLOR=\"#00FF00\">%.1f &deg;F</TD>\n", tempf); 
    } else {
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
    }
}

/* handler for "HUMIDITY" */
void do_humidity(char *monhost, char *suffix)
{
	char	humidity[64];
	float	ambhum;

     if (getupsvar (monhost, "humidity", humidity, sizeof(humidity)) > 0) {
	 ambhum = strtod (humidity, 0);
         printf ("<TD BGCOLOR=\"#00FF00\">%.1f %%</TD>\n", ambhum);
     }
     else
         printf ("<TD CLASS=\"Empty\">-</TD>\n");
}

/* handler for "AMBTEMP" */
void do_ambtemp(char *monhost, char *suffix)
{
    char    ambtemp[64];
    float   tempf;

    if (getupsvar (monhost, "ambtemp", ambtemp, sizeof(ambtemp)) > 0) {
	if (use_celsius) 
            printf ("<TD BGCOLOR=\"#00FF00\">%s °C</TD>\n", ambtemp); 
	else {
	    tempf = (strtod (ambtemp, 0) * 1.8) + 32;
            printf ("<TD BGCOLOR=\"#00FF00\">%.1f °F</TD>\n", tempf); 
	}
    } 
    else {
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
    }
}

/* handler for "AMBTEMPC" */
void do_ambtempc(char *monhost, char *suffix)
{
	char	ambtemp[64];

    if (getupsvar (monhost, "ambtemp", ambtemp, sizeof(ambtemp)) > 0)
        printf ("<TD BGCOLOR=\"#00FF00\">%s °C</TD>\n", ambtemp); 
    else
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
}

/* handler for "AMBTEMPF" */
void do_ambtempf (char *monhost, char *suffix)
{
	char	ambtemp[64];
	float	tempf;

    if (getupsvar (monhost, "ambtemp", ambtemp, sizeof(ambtemp)) > 0) {
	tempf = (strtod (ambtemp, 0) * 1.8) + 32;
        printf ("<TD BGCOLOR=\"#00FF00\">%.1f °F</TD>\n", tempf); 
    }
    else
        printf ("<TD CLASS=\"Empty\">-</TD>\n");
}

/* handler for "UTILITY" */
void do_utility(char *monhost, char *suffix)
{
    char utility[64], lowxfer[64], highxfer[64];
    int  lowx, highx, util;

    if (getupsvar (monhost, "utility", utility, sizeof(utility)) > 0) {
	/* try to get low and high transfer points for color codes */

	lowx = highx = 0;

        if (getupsvar (monhost, "lowxfer", lowxfer, sizeof(lowxfer)) > 0)
	    lowx = atoi(lowxfer);
        if (getupsvar (monhost, "highxfer", highxfer, sizeof(highxfer)) > 0)
	    highx = atoi(highxfer);

        printf ("<TD BGCOLOR=\"#");

	/* only do this if we got both values */
	if ((lowx != 0) && (highx != 0)) {
	    util = atoi(utility);

	    if ((util < lowx) || (util > highx))
                printf ("FF0000");
	    else
                printf ("00FF00");
	}
	else
            printf ("00FF00");

        printf ("\">%s VAC</TD>\n", utility);
    } else
        printf ("<TD></TD>\n");
}



/* Get and print information for requested host */
void getinfo(char *monhost)
{
    ftype   *tmp;
    char    tmpbuf[256];
    int     i, found;


    /* grab a dummy variable to see if the host is up */
    if (getupsvar(monhost, "date", tmpbuf, sizeof(tmpbuf)) <= 0) {
        printf ("<TD COLSPAN=%i BGCOLOR=\"#FF0000\">%s&nbsp;&nbsp;&nbsp;&nbsp&nbsp;&nbsp; Not available: %s</TD></TR>\n",
		 numfields, desc, net_errmsg);
	return;
    }
    printf ("<TR ALIGN=CENTER>\n");

    /* process each field one by one */
    for (tmp = (ftype *)firstfield; tmp != NULL; tmp = (ftype *)tmp->next) {
	found = 0;

	/* search for a recognized special field name */
	for (i = 0; fields[i].name != NULL; i++) {
		if (!strcmp(fields[i].name, tmp->var)) {
			fields[i].func(monhost, tmp->suffix);
			found = 1;
		}
	}

	if (found)
	    continue;

	if (getupsvar(monhost, tmp->var, tmpbuf, sizeof(tmpbuf)) > 0) {
	    if (tmp->suffix == NULL)
                printf ("<TD BGCOLOR=\"#00FF00\">%s</TD>\n", tmpbuf);
	    else
                printf ("<TD BGCOLOR=\"#00FF00\">%s %s</TD>\n", 
				tmpbuf, tmp->suffix);
	} else
           printf ("<TD></TD>\n");
    }

    printf ("</TR>\n");

    return;
}

/* add a field to the linked list */
void addfield(char *var, char *name, char *suffix)
{
    ftype   *tmp, *last;

    tmp = last = firstfield;

    while (tmp != NULL) {
	last = tmp;
	tmp = (ftype *)tmp->next;
    }

    tmp = (ftype *)malloc (sizeof (ftype));
    tmp->var = var;
    tmp->name = name;
    tmp->suffix = suffix;
    tmp->next = NULL;

    if (last == NULL) {
	firstfield = tmp;
    } else {
	last->next = tmp;
    }

    numfields++;
}

/* parse a FIELD line from the buf and call addfield */
void parsefield(char *buf)
{
    char *ptr, *var, *name = NULL, *suffix = NULL, *tmp;
    int i = 0, in_string = 0;

    tmp = (char *)malloc(strlen(buf) + 1);

    /* <variable> "<field name>" "<field suffix>" */

    ptr = strtok(buf, " ");
    if (ptr == NULL) {
        report_error("multimon.conf: "
                      "No separating space in FIELD line.");
    }
    var = strdup(ptr);

    ptr = buf + strlen(var) + 1;
    while (*ptr) {
        if (*ptr == '"') {
	    in_string = !in_string;
	    if (!in_string) {
                tmp[i] = '\0';
		i = 0;
		if (suffix) {
                        sprintf(tmp, "multimon.conf: "
                                "More than two strings "
                                "in field %s.", var);
			report_error(tmp);
		}
		else if (name) {
			suffix = strdup(tmp);
		}
		else {
			name = strdup(tmp);
		}
	    }
	} else if (in_string) {
            if (*ptr == '\\') {
		++ptr;
                if (*ptr == '\0') {
                        sprintf(tmp, "multimon.conf: "
                                "Backslash at end of line "
                                "in field %s.", var);
			report_error(tmp);
		}
	    }
	    tmp[i++] = *ptr;
	}
	++ptr;
    }

    if (in_string) {
            sprintf(tmp, "multimon.conf: "
                    "Unbalanced quotes in field %s!", var);
	    report_error(tmp);
    }

    free(tmp);

    addfield(var, name, suffix);
}	

void readconf(void)
{
    FILE    *conf;
    char    buf[512], fn[256];

    snprintf (fn, sizeof(fn), "%s/multimon.conf", CONFPATH);
    conf = fopen (fn, "r");

    /* the config file is not required */
    if (conf == NULL)
	return;

    while (fgets (buf, sizeof(buf), conf)) {
	buf[strlen(buf) - 1] = 0;
        if (!strncmp (buf, "FIELD", 5))
	     parsefield (&buf[6]);
        if (!strncmp (buf, "TEMPC", 5))
	     use_celsius = 1;
        if (!strncmp (buf, "TEMPF", 5))
	     use_celsius = 0;
    }

    fclose (conf);
}	

/* create default field configuration */
void defaultfields(void)
{
    addfield ("SYSTEM", "System", "");
    addfield ("MODEL", "Model", "");
    addfield ("STATUS", "Status", "");
    addfield ("battpct", "Batt Chg", "%");
    addfield ("UTILITY", "Utility", "VAC");
    addfield ("loadpct", "UPS Load", "%");
    addfield ("UPSTEMP", "UPS Temperature", "");
    addfield ("DATA",  "Data", "All data");
}	

int main(int argc, char **argv) 
{
    FILE    *conf;
    time_t  tod;
    char    buf[256], fn[256], addr[256], timestr[256], *rest;
    int     restofs;
    ftype   *tmp;

    /* set default according to compile time, but config may override */
#ifdef USE_CELSIUS
    use_celsius = 1;
#else
    use_celsius = 0;
#endif

    /* print out header first so we can give error reports */
    printf ("Content-type: text/html\n");
    printf ("\n");

    printf ("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"\n");
    printf ("       \"http://www.w3.org/TR/REC-html40/loose.dtd\">\n");

    printf ("<HTML>\n");
    printf ("<HEAD><TITLE>Multimon: UPS Status Page</TITLE>\n");
    printf ("<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n");
    printf ("</HEAD>\n");
    printf ("<BODY BGCOLOR=\"#FFFFFF\">\n");

    readconf();

    if (firstfield == NULL)	    /* nothing from config file? */
	    defaultfields();

    printf ("<TABLE BGCOLOR=\"#50A0A0\" ALIGN=CENTER>\n");
    printf ("<TR><TD>\n");
    printf ("<TABLE CELLPADDING=5>\n");
    printf ("<TR>\n");
    
    time (&tod);
    strftime (timestr, 100, "%a %b %d %X %Z %Y", localtime(&tod));

    printf ("<TH COLSPAN=%i BGCOLOR=\"#60B0B0\">\n", numfields + 1);
    printf ("<FONT SIZE=\"+2\">APCUPSD UPS Network Monitor</FONT>\n");
    printf ("<BR>%s</TH>\n", timestr);
    printf ("</TR>\n"); 

    printf ("<TR BGCOLOR=\"#60B0B0\">\n");

    /* print column names */
    for (tmp = (ftype *)firstfield; tmp != NULL; tmp = (ftype *)tmp->next)
        printf ("<TH COLSPAN=1>%s</TH>\n", tmp->name);

    printf ("</TR>\n"); 

    /* ups status */

    snprintf (fn, sizeof(fn), "%s/hosts.conf", CONFPATH);
    conf = fopen (fn, "r");
    if (conf == NULL) {
        printf ("<TD COLSPAN=%i BGCOLOR=\"#FF0000\">Error: Cannot open hosts file</TD></TR>\n",
		   numfields);
    } else {
	while (fgets (buf, sizeof(buf), conf)) {
            if (strncmp("MONITOR", buf, 4) == 0) {
                sscanf (buf, "%*s %s %n", addr, &restofs);
		rest = buf + restofs + 1;
                desc = strtok(rest, "\"");
		getinfo(addr);	/* get and print info for this host */
	    }
	}
	fclose (conf);
    }
    
    printf ("</TABLE>\n"); 
    printf ("</TD></TR>\n"); 
    printf ("</TABLE></BODY></HTML>\n");

    exit(0);
}
