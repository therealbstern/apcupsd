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
static int    img1 = 1;
static int    img2 = 6;
static int    img3 = 5;
static char   temps[16] = "C";
static int    refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
	strncpy (monhost, value, sizeof(monhost));
	monhost[sizeof(monhost) - 1] = '\0';

    } else if (strcmp(var, "img1") == 0) {
	img1 = atoi(value);
        if ((img1 <= 0) || (img1 > 6)) {
            img1 = 1;
        }

    } else if (strcmp(var, "img2") == 0) {
	img2 = atoi(value);
        if ((img2 <= 0) || (img2 > 6)) {
            img2 = 6;
        }

    } else if (strcmp(var, "img3") == 0) {
	img3 = atoi(value);
        if ((img3 <= 0) || (img3 > 6)) {
            img3 = 5;
        }

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
        printf ("battcap&value=%s&value2=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"Battery Capacity\\\"\"",answer, answer2);
	break;
    case 2: 
        getupsvar (monhost, "battvolt", answer, sizeof(answer));  
        getupsvar (monhost, "nombattv", answer2, sizeof(answer2));
        printf ("battvolt&value=%s&value2=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"Battery Voltage\\\"\"",answer, answer2);
	break;
    case 3: 
        getupsvar (monhost, "utility", answer, sizeof(answer));  
        getupsvar (monhost, "lowxfer", answer2, sizeof(answer2));
        getupsvar (monhost, "highxfer", answer3, sizeof(answer3));
        printf ("utility&value=%s&value2=%s&value3=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"Utility Voltage\\\"\"",answer,answer2,answer3);
	break;
    case 4: 
        getupsvar (monhost, "outputv", answer, sizeof(answer));  
        printf ("outputv&value=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"Output Voltage\\\"\"",answer);
	break;
    case 5: 
        getupsvar (monhost, "upsload", answer, sizeof(answer));  
        printf ("upsload&value=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"UPS Load\\\"\"",answer);
	break;
    case 6:
        getupsvar (monhost, "runtime", answer, sizeof(answer));  
        getupsvar (monhost, "mintimel", answer2, sizeof(answer2));
        printf ("runtime&value=%s&value2=%s\",\" width=\\\"150\\\" height=\\\"350\\\" alt=\\\"Run time remaining\\\"\"",answer, answer2);
	break;
    }
}

int main(int argc, char **argv) 
{
    int     status;
    double  tempf;
    char *p;
    char   answer[256];

    (void) extractcgiargs();

    puts ("Content-type: text/html");
    puts ("Pragma: no-cache\n");

    puts ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\"");
    puts ("    \"http://www.w3.org/TR/html4/strict.dtd\">");
    puts ("<html>");
    
    p = strchr(monhost, '%');
    if (p && *(p+1) == '3') {
       *p++ = ':';                    /* set colon */
       strcpy(p, p+2);		      /* get rid of hex 3A */
    }

    if (!checkhost(monhost)) {
        puts ("<head></head><body>");
        printf ("<p>Access to %s host is not authorized.</p>\n", monhost);
	html_finish();
	exit (0);
    }
    
    /* check if host is available */
    if (getupsvar(monhost, "date", answer, sizeof(answer)) <= 0) {
           puts ("<head></head><body>");
           printf ("<p>Unable to communicate with the UPS on %s</p>\n", monhost);
	   html_finish();
	   exit (0);
    }

     printf ("<head>\n<title>%s on %s</title>\n", answer, monhost);
     puts ("  <meta http-equiv=\"Pragma\" content=\"no-cache\">");
     if (refresh != 0) {
         printf ("  <meta http-equiv=\"Refresh\" content=\"%d\">\n", refresh);
     }

     puts ("<style type=\"text/css\">");
     puts ("  body {color: black; background: white}");
     puts ("  div.center {text-align: center}");
     puts ("  pre {text-align: left}");
     puts ("</style>");

     puts ("<script type=\"text/javascript\">");
     puts ("var now = new Date();");
     puts ("var seed = now.getTime() % (0xffffffff);");

     puts ("function rand(n) {");
     puts ("seed = (0x015a4e35 * seed) % (0x7fffffff);");
     puts ("return ((seed >> 16) % (n));");
     puts ("}");

     puts ("function rimg(url,extra,n) {");
     puts ("return \"<img src=\\\"\" + url + \"&rand=\" + rand(n) + \"\\\" \" + extra + \">\" ");
     puts ("}");

     puts ("</script>");
     puts ("</head>");

     puts ("<body><div class=\"center\">");
     puts ("<table border cellspacing=\"10\" cellpadding=\"5\">");

     getupsvar(monhost, "date", answer, sizeof(answer));
     fputs ("<tr><th>", stdout);
     html_puts(answer);
     puts ("</th>");

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form1\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <select onChange=\"document.form1.submit();return true\" name=\"img1\">\n");

     printf ("               <option "); if (img1 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img1 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img1 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img1 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img1 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img1 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\">\n",img2);
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\">\n",img3);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
     printf ("       </div></form>\n");
     printf ("</th>\n");

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form2\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\">\n",img1);
     printf ("       <select onChange=\"document.form2.submit();return true\" name=\"img2\">\n");

     printf ("               <option "); if (img2 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img2 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img2 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img2 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img2 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img2 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");
     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\">\n",img3);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
     printf ("       </div></form>\n");
     printf ("</th>\n");

     printf ("<th>\n");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form3\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\">\n",img1);
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\">\n",img2);
     printf ("       <select onChange=\"document.form3.submit();return true\" name=\"img3\">\n");

     printf ("               <option "); if (img3 == 1) printf ("selected "); printf ("value=\"1\">Battery Capacity\n");
     printf ("               <option "); if (img3 == 2) printf ("selected "); printf ("value=\"2\">Battery Voltage\n");
     printf ("               <option "); if (img3 == 3) printf ("selected "); printf ("value=\"3\">Utility Voltage\n");
     printf ("               <option "); if (img3 == 4) printf ("selected "); printf ("value=\"4\">Output Voltage\n");
     printf ("               <option "); if (img3 == 5) printf ("selected "); printf ("value=\"5\">UPS Load\n");
     printf ("               <option "); if (img3 == 6) printf ("selected "); printf ("value=\"6\">Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\">\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
     printf ("       </div></form>\n");
     printf ("</th>\n");

     printf ("</tr>\n");

     printf ("<tr><td>\n<pre>\n");

     getupsvar (monhost, "hostname", answer, sizeof(answer));
     fputs ("Monitoring: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "model", answer, sizeof(answer));  
     fputs (" UPS Model: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "upsname", answer, sizeof(answer));
     fputs ("  UPS Name: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "release", answer, sizeof(answer));
     fputs ("   APCUPSD: Version ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     fputs ("    Status: ", stdout);

     if (getupsvar (monhost, "status", answer, sizeof(answer)) <= 0) {
             puts ("Not available");
     } else {
	 status = strtol(answer, 0, 16);
	 if (status & UPS_CALIBRATION) 
             fputs ("CALIBRATION ", stdout); 
	 if (status & UPS_SMARTTRIM)
             fputs ("SMART TRIM ", stdout);
	 if (status & UPS_SMARTBOOST)
             fputs ("SMART BOOST ", stdout);
	 if (status & UPS_ONLINE)
             fputs ("ONLINE ", stdout); 
	 if (status & UPS_ONBATT) 
             fputs ("ON BATTERY ", stdout); 
	 if (status & UPS_OVERLOAD)
             fputs ("OVERLOADED ", stdout);
	 if (status & UPS_BATTLOW) 
             fputs ("BATTERY LOW ", stdout); 
	 if (status & UPS_REPLACEBATT)
             fputs ("REPLACE BATTERY ", stdout);
	 if (status & UPS_COMMLOST)
             fputs("COMM LOST ", stdout); 
	 if (status & UPS_SHUTDOWN)
             fputs("SHUTDOWN ", stdout);
	 if (status & UPS_SLAVE)
             fputs("SLAVE ", stdout);
         fputs ("\n", stdout); 
     }

     puts ("</pre></td>");

     puts ("<td rowspan=\"3\"><script type=\"text/javascript\">");
     fputs ("document.write(rimg(\"upsimage.cgi?display=", stdout);
     send_values(img1, 1);
     puts (", 1000))</script></td>");

     puts ("<td rowspan=\"3\"><script type=\"text/javascript\">");
     fputs ("document.write(rimg(\"upsimage.cgi?display=", stdout);
     send_values(img2, 6);
     puts (", 1000))</script></td>");

     puts ("<td rowspan=\"3\"><script type=\"text/javascript\">");
     fputs ("document.write(rimg(\"upsimage.cgi?display=", stdout);
     send_values(img3, 5);
     puts (", 1000))</script></td>");

     printf ("<tr>\n<td>\n<pre>\n");

     getupsvar (monhost, "selftest", answer, sizeof(answer));
     fputs ("Last UPS Self Test: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar(monhost, "laststest", answer, sizeof(answer));
     /* To reduce the length of the output, we drop the 
      * seconds and the trailing year.
      */
     for (p=answer; *p && *p != ':'; p++) ;
     if (*p == ':')
	p++;
     for ( ; *p && *p != ':'; p++) ;
     *p = '\0';
     fputs ("Last Test Date: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     puts ("</pre></td></tr>");

     printf ("<tr>\n<td>\n<pre>\n");

     getupsvar (monhost, "utility", answer, sizeof(answer));
     fputs ("Utility Voltage: ", stdout);
     html_puts (answer);
     fputs (" VAC\n", stdout);

     getupsvar (monhost, "linemin", answer, sizeof(answer));
     fputs ("   Line Minimum: ", stdout);
     html_puts (answer);
     fputs (" VAC\n", stdout);

     getupsvar (monhost, "linemax", answer, sizeof(answer));
     fputs ("   Line Maximum: ", stdout);
     html_puts (answer);
     fputs (" VAC\n", stdout);

     getupsvar (monhost, "outputfreq", answer, sizeof(answer));
     fputs ("    Output Freq: ", stdout);
     html_puts (answer);
     fputs (" Hz\n", stdout);

     if (getupsvar(monhost, "ambtemp", answer, sizeof(answer)) > 0) {
         if (strcmp(answer, "Not found" ) != 0) {
             if (strcmp(temps,"F") == 0) {
		tempf = (strtod (answer, 0) * 1.8) + 32;
                printf ("     Amb. Temp.: %.1f&deg; F\n", tempf);
             } else if (strcmp(temps,"K") == 0) {
		tempf = (strtod (answer, 0)) + 273;
                printf ("     Amb. Temp.: %.1f&deg; K\n", tempf);
	     } else {
                printf ("     Amb. Temp.: %s&deg; C\n", answer);
	     } 
	 }
     }

     if ( getupsvar (monhost, "humidity", answer, sizeof(answer)) > 0) {
         if (strcmp(answer, "Not found") != 0) {
              fputs ("  Amb. Humidity: ", stdout);
              html_puts (answer);
              puts (" %");
	 }
     }

     printf ("</pre>\n<table BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n<tr>\n<td colspan=\"2\">\n<pre>\n");

    if (getupsvar (monhost, "upstemp", answer, sizeof(answer)) > 0) {
         if (strcmp(temps,"F") == 0) {
	      tempf = (strtod (answer, 0) * 1.8) + 32;
              printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
              printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\"><div>\n");
              printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
              printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\">\n",img1);
              printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\">\n",img2);
              printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\">\n",img3);
              printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
              printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
              printf ("          <option value=\"C\">&deg; C\n");
              printf ("          <option selected value=\"F\">&deg; F\n");
              printf ("          <option value=\"K\">&deg; K\n");

	  } else if (strcmp(temps,"K") == 0) {
	       tempf = (strtod (answer, 0)) + 273;
               printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
               printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\"><div>\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\">\n",img1);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\">\n",img2);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\">\n",img3);
               printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
               printf ("         <option value=\"C\">&deg; C\n");
               printf ("         <option value=\"F\">&deg; F\n");
               printf ("         <option selected value=\"K\">&deg; K\n");

	 } else {
               printf ("       UPS Temp: %s \n</pre>\n</td>\n<td>\n", answer);
               printf ("       <form method=\"get\" action=\"upsstats.cgi\" name=\"form4\"><div>\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\">\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\">\n",img1);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\">\n",img2);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\">\n",img3);
               printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\">\n",refresh);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=\"temp\">\n");
               printf ("         <option selected value=\"C\">&deg; C\n");
               printf ("         <option value=\"F\">&deg; F\n");
               printf ("         <option value=\"K\">&deg; K\n");
	 }
    }

     puts ("       </select>");
     puts ("       </div></form>");
     puts ("</td></tr></table>");   
     puts ("</tr>");

     puts ("<tr><td colspan=\"4\"><b>Recent Events</b><br>");
     puts ("<textarea rows=\"5\" cols=\"95\">");

     fetch_events(monhost);
     html_puts (statbuf);

     puts ("</textarea>");
     puts ("</td></tr>");

     puts ("</table></div>");

     html_finish();
     return 0;
}
