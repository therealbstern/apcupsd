/*
 * upsstats - cgi program to generate the main UPS info page
 *
 * Author: Russell Kroll <rkroll@exploits.org>
 *
 * To use: install the binary in a directory where CGI programs may be
 *	 executed by your web server.  On many systems something like
 *	 /usr/local/etc/httpd/cgi-bin will work nicely.  I recommend
 *         calling the binary "upsstats.cgi" in that directory.
 *
 *	 Assuming a path like the above, the following link will suffice:
 *         <A HREF="/cgi-bin/upsstats.cgi">UPS Status</A>
 * 
 *	 This program assumes that upsimage.cgi will be in the same 
 *	 directory.  The install-cgi target will take care of putting
 *	 things in the right place if you set the paths properly in the
 *	 Makefile.
 *
 * Modified: Jonathan Benson <jbenson@technologist.com>
 *	   19/6/98 to suit apcupsd
 *	   23/6/98 added more graphs and menu options
 *
 * Modified: Kern Sibbald <kern@sibbald.com>
 *	  Nov 1999 to work with apcupsd networking and
 *		   to include as much of the NUT code
 *		     as possible.
 *		   added runtim status
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "cgiconfig.h"
#include "cgilib.h"
#include "upsfetch.h"

#ifndef DEFAULT_REFRESH
#define DEFAULT_REFRESH 30
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

static char   monhost[MAXHOSTNAMELEN] = "127.0.0.1";
static char   img1s[16] = "";;
static char   img2s[16] = "";;
static char   img3s[16] = "";;
static char   temps[16] = "";;
static int    refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
	strncpy (monhost, value, sizeof(monhost));
	monhost[sizeof(monhost) - 1] = '\0';

    } else if (strcmp(var, "img1") == 0) {
	strncpy (img1s, value, sizeof(img1s));
	img1s[sizeof(img1s) - 1] = '\0';

    } else if (strcmp(var, "img2") == 0) {
	strncpy (img2s, value, sizeof(img2s));
	img2s[sizeof(img2s) - 1] = '\0';

    } else if (strcmp(var, "img3") == 0) {
	strncpy (img3s, value, sizeof(img3s));
	img3s[sizeof(img3s) - 1] = '\0';

    } else if (strcmp(var, "temp") == 0) {
	strncpy (temps, value, sizeof(temps));
	temps[sizeof(temps) - 1] = '\0';

    } else if (strcmp(var, "refresh") == 0) {
	refresh = atoi(value);
	if (refresh < 0) {
		refresh = DEFAULT_REFRESH;
	}
    }
}

void send_values(int report, int defrpt)
{
    char   answer[256], answer2[256], answer3[256];

    if (report < 1 || report > 6)
	   report = defrpt;
    switch ( report ) {
    case 1:
        getupsvar (monhost, "battcap", answer, sizeof(answer));  
        getupsvar (monhost, "mbattchg", answer2, sizeof(answer2));
        printf ("battcap&value=%s&value2=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"Battery Capacity\\\"\"",answer, answer2);
	break;
    case 2: 
        getupsvar (monhost, "battvolt", answer, sizeof(answer));  
        getupsvar (monhost, "nombattv", answer2, sizeof(answer2));
        printf ("battvolt&value=%s&value2=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"Battery Voltage\\\"\"",answer, answer2);
	break;
    case 3: 
        getupsvar (monhost, "utility", answer, sizeof(answer));  
        getupsvar (monhost, "lowxfer", answer2, sizeof(answer2));
        getupsvar (monhost, "highxfer", answer3, sizeof(answer3));
        printf ("utility&value=%s&value2=%s&value3=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"Utility Voltage\\\"\"",answer,answer2,answer3);
	break;
    case 4: 
        getupsvar (monhost, "outputv", answer, sizeof(answer));  
        printf ("outputv&value=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"Output Voltage\\\"\"",answer);
	break;
    case 5: 
        getupsvar (monhost, "upsload", answer, sizeof(answer));  
        printf ("upsload&value=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"UPS Load\\\"\"",answer);
	break;
    case 6:
        getupsvar (monhost, "runtime", answer, sizeof(answer));  
        getupsvar (monhost, "mintimel", answer2, sizeof(answer2));
        printf ("runtime&value=%s&value2=%s\",\" WIDTH=150 HEIGHT=350 ALT=\\\"Run time remaining\\\"\"",answer, answer2);
	break;
    }
}

int main(int argc, char **argv) 
{
    int     status,img1,img2,img3;
    double  tempf;
    char *p;
    char   answer[256];

    (void) extractcgiargs();

    printf ("Content-type: text/html\n");
    printf ("Pragma: no-cache\n\n");

    printf ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    printf ("<html>\n");

    img1 = atoi(img1s);
    img2 = atoi(img2s);
    img3 = atoi(img3s);
    
    if (img1 <= 0) img1 = 1;
    if (img2 <= 0) img2 = 6;
    if (img3 <= 0) img3 = 5;

    p = strchr(monhost, '%');
    if (p && *(p+1) == '3') {
       *p++ = ':';                    /* set colon */
       strcpy(p, p+2);		      /* get rid of hex 3A */
    }

    if (!checkhost(monhost)) {
        printf("<head></head><body>\n");
        printf ("<p>Access to %s host is not authorized.</p>\n", monhost);
	html_finish();
	exit (0);
    }
    
    /* check if host is available */
    if (getupsvar(monhost, "date", answer, sizeof(answer)) <= 0) {
           printf("<head></head><body>\n");
           printf ("<p>Unable to communicate with the UPS on %s</p>\n", monhost);
	   html_finish();
	   exit (0);
    }

     printf ("<head>\n<title>%s on %s</title>\n", answer, monhost);
     printf ("  <meta http-equiv=\"Pragma\" content=\"no-cache\">\n");
     if (refresh != 0) {
         printf ("  <meta http-equiv=\"Refresh\" content=\"%d\">\n", refresh);
     }

     printf ("<script type=\"text/javascript\">\n");
     printf ("var now = new Date()\n");
     printf ("var seed = now.getTime() %% (0xffffffff)\n");

     printf ("function rand(n) {\n");
     printf ("seed = (0x015a4e35 * seed) %% (0x7fffffff);\n");
     printf ("return ((seed >> 16) %% (n));\n");
     printf ("}\n");

     printf ("function rimg(url,extra,n) {\n");
     printf ("return \"<img src=\\\"\" + url + \"&rand=\" + rand(n) + \"\\\" \" + extra + \">\" \n");
     printf ("}\n");

     printf ("\n//-->\n</script>\n");

     printf ("</head>\n");

     printf ("<body>\n");
     printf ("<center>\n<table border cellspacing=\"10\" cellpadding=\"5\">\n");
     getupsvar(monhost, "date", answer, sizeof(answer));
     printf ("<tr><th>%s</th>\n", answer);

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form1\">\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <select onChange=\"document.form1.submit();return true\" name=\"img1\">\n");

     printf ("               <option "); if (img1 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img1 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img1 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img1 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img1 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img1 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%s\">\n",img2s);
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%s\">\n",img3s);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       </form>\n");
     printf ("</th>\n");

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form2\">\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%s\">\n",img1s);
     printf ("       <select onChange=\"document.form2.submit();return true\" name=\"img2\">\n");

     printf ("               <option "); if (img2 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img2 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img2 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img2 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img2 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img2 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");
     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%s\">\n",img3s);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       </form>\n");
     printf ("</th>\n");

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form3\">\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%s\">\n",img1s);
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%s\">\n",img2s);
     printf ("       <select onChange=\"document.form3.submit();return true\" name=\"img3\">\n");

     printf ("               <option "); if (img3 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img3 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img3 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img3 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img3 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img3 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       </form>\n");
     printf ("</th>\n");

     printf ("</tr>\n");

     printf ("<tr><td BGCOLOR=\"#FFFFFF\">\n<pre>\n");
     getupsvar (monhost, "hostname", answer, sizeof(answer));
     printf ("Monitoring: %s\n", answer);
     getupsvar (monhost, "model", answer, sizeof(answer));  
     printf (" UPS Model: %s\n", answer);
     getupsvar (monhost, "upsname", answer, sizeof(answer));
     printf ("  UPS Name: %s\n", answer);
     getupsvar (monhost, "release", answer, sizeof(answer));
     printf ("   APCUPSD: Version %s\n", answer);

     printf ("    Status: ");

     if (getupsvar (monhost, "status", answer, sizeof(answer)) <= 0) {
             printf ("Not available\n");
     } else {
	 status = strtol(answer, 0, 16);
	 if (status & UPS_CALIBRATION) 
             printf ("CALIBRATION "); 
	 if (status & UPS_SMARTTRIM)
             printf ("SMART TRIM ");
	 if (status & UPS_SMARTBOOST)
             printf ("SMART BOOST ");
	 if (status & UPS_ONLINE)
             printf ("ONLINE "); 
	 if (status & UPS_ONBATT) 
             printf ("ON BATTERY "); 
	 if (status & UPS_OVERLOAD)
             printf ("OVERLOADED ");
	 if (status & UPS_BATTLOW) 
             printf ("BATTERY LOW "); 
	 if (status & UPS_REPLACEBATT)
             printf ("REPLACE BATTERY ");
	 if (status & UPS_COMMLOST)
             printf("COMM LOST "); 
	 if (status & UPS_SHUTDOWN)
             printf("SHUTDOWN ");
	 if (status & UPS_SLAVE)
             printf("SLAVE ");
         printf ("\n"); 
     }

     printf ("</pre>\n</td>\n");

     printf ("<td rowspan=\"3\"><script type=\"text/javascript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img1, 1);

     printf (",1000))</script>");
     printf ("</td>\n");

     printf ("<td rowspan=\"3\"><script type=\"text/javascript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img2, 3);

     printf (",1000))</script>");
     printf ("</td>\n");

     printf ("<td rowspan=\"3\"><script type=\"text/javascript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img3, 5);

     printf (",1000))</script>");
     printf ("</td>\n</tr>\n");

     printf ("<tr>\n<td BGCOLOR=\"#FFFFFF\">\n<pre>\n");
     getupsvar (monhost, "selftest", answer, sizeof(answer));
     printf ("Last UPS Self Test: %s\n", answer);
     getupsvar(monhost, "laststest", answer, sizeof(answer));
     /* To reduce the length of the output, we drop the 
      * seconds and the trailing year.
      */
     for (p=answer; *p && *p != ':'; p++) ;
     if (*p == ':')
	p++;
     for ( ; *p && *p != ':'; p++) ;
     *p = 0;
     printf ("Last Test Date: %s\n", answer);
     printf ("</pre>\n</td>\n</tr>\n");

     printf ("<tr>\n<td BGCOLOR=\"#FFFFFF\">\n<pre>\n");
     getupsvar (monhost, "utility", answer, sizeof(answer));
     printf ("Utility Voltage: %s VAC\n", answer);

     getupsvar (monhost, "linemin", answer, sizeof(answer));
     printf ("   Line Minimum: %s VAC\n", answer);

     getupsvar (monhost, "linemax", answer, sizeof(answer));
     printf ("   Line Maximum: %s VAC\n", answer);

     getupsvar (monhost, "outputfreq", answer, sizeof(answer));
     printf ("    Output Freq: %s Hz\n", answer);

     if (getupsvar(monhost, "ambtemp", answer, sizeof(answer)) > 0) {
         if (strcmp(answer, "Not found" )) {
             if (!strcmp(temps,"F")) {
		tempf = (strtod (answer, 0) * 1.8) + 32;
                printf ("     Amb. Temp.: %.1f &deg;F\n", tempf);
             } else if (!strcmp(temps,"K")) {
		tempf = (strtod (answer, 0)) + 273;
                printf ("     Amb. Temp.: %.1f &deg;K\n", tempf);
	     } else {
                printf ("     Amb. Temp.: %s &deg;C\n", answer);
	     } 
	 }
     }

     if ( getupsvar (monhost, "humidity", answer, sizeof(answer)) > 0) {
         if (strcmp( answer, "Not found" )) {
              printf ("  Amb. Humidity: %s %%\n", answer);
	 }
     }

     printf ("</pre>\n<table BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n<tr>\n<td colspan=\"2\">\n<pre>\n");

    if (getupsvar (monhost, "upstemp", answer, sizeof(answer)) > 0) {
         if (strcmp(temps,"F") == 0) {
	      tempf = (strtod (answer, 0) * 1.8) + 32;
              printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
              printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\">\n");
              printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
              printf ("       <input type=\"hidden\" name=\"img1\" value=\"%s\">\n",img1s);
              printf ("       <input type=\"hidden\" name=\"img2\" value=\"%s\">\n",img2s);
              printf ("       <input type=\"hidden\" name=\"img3\" value=\"%s\">\n",img3s);
              printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
              printf ("              <option value=\"C\">&deg;C\n");
              printf ("               <option selected value=\"F\">&deg;F\n");
              printf ("               <option value=\"K\">&deg;K\n");

	  } else if (strcmp(temps,"K") == 0) {
	       tempf = (strtod (answer, 0)) + 273;
               printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
               printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\">\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%s\">\n",img1s);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%s\">\n",img2s);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%s\">\n",img3s);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
               printf ("              <option value=\"C\">&deg;C\n");
               printf ("               <option value=\"F\">&deg;F\n");
               printf ("               <option selected value=\"K\">&deg;K\n");

	 } else {
               printf ("       UPS Temp: %s \n</pre>\n</td>\n<td>\n", answer);
               printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\">\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%s\">\n",img1s);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%s\">\n",img2s);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%s\">\n",img3s);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
               printf ("              <option selected value=\"C\">&deg;C\n");
               printf ("               <option value=\"F\">&deg;F\n");
               printf ("               <option value=\"K\">&deg;K\n");
	 }
    }

     printf ("       </select>\n");
     printf ("       </form>\n");
     printf ("</td></tr></table>\n");   
     printf ("</tr>\n");

     printf ("<tr><td colspan=\"4\">Most recent events:<br>\n");
     printf ("<textarea rows=\"5\" cols=\"95\">\n");

     fetch_events(monhost);
     html_puts (statbuf);

     printf ("</textarea>\n");
     printf ("</td></tr>");

     printf ("</table>\n");
     printf ("</center>\n");

     html_finish();
     return 0;
}
