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

/* default host - only used if you don't specify one in the URL */
#define MONHOST "127.0.0.1"
#define REFRESH 30

#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include "cgiconfig.h"
#include "cgilib.h"
#include "upsfetch.h"
#include "status.h"


char   monhost[256];
char   img1s[16];
char   img2s[16];
char   img3s[16];
char   temps[16];
char   answer[256];
char   answer2[256];
char   answer3[256];

extern int fetch_events(char *host);

void parsearg(char var[255], char value[255]) 
{
    if (!strcmp(var, "host")) {
	strncpy (monhost, value, sizeof(monhost));
    }
    if (!strcmp(var, "img1")) {
	strncpy (img1s, value, sizeof(img1s));
    }
    if (!strcmp(var, "img2")) {
	strncpy (img2s, value, sizeof(img2s));
    }
    if (!strcmp(var, "img3")) {
	strncpy (img3s, value, sizeof(img3s));
    }
    if (!strcmp(var, "temp")) {
	strncpy (temps, value, sizeof(temps));
    }
}


void nocomm(char *monhost)
{
    printf ("upsstats.c: Unable to communicate with the UPS on %s\n", monhost);
    exit (0);
}

void send_values(int report, int defrpt)
{
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
    int     status,img1,img2,img3,refresh;
    double  tempf;
    char *p;

    strcpy (monhost, MONHOST);	    /* default host */


    refresh = REFRESH;

    printf ("Content-type: text/html\n");
    printf ("Pragma: no-cache\n");
 /*     printf ("Expires: Thu, 01 Jan 1998 00:00:00 GMT\n"); */
    printf ("\n");

    if (!extractcgiargs()) {
       printf("Unable to extract cgi arguments.\n");
       exit(0);
    }

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
        printf ("Access to %s host is not authorized.\n", monhost);
	exit (0);
    }
    
    /* check if host is available */
    if (getupsvar(monhost, "date", answer, sizeof(answer)) <= 0) 
	   nocomm(monhost);

     printf ("<HTML>\n<HEAD>\n       <TITLE>%s on %s</TITLE>\n", answer, monhost);
     printf ("       <META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n");
     printf ("       <META HTTP-EQUIV=\"Refresh\" CONTENT=\"%i\">\n",refresh);

     printf ("<SCRIPT LANGUAGE = \"JavaScript\">\n");
     printf ("var now = new Date()\n");
     printf ("var seed = now.getTime() %% (0xffffffff)\n");

     printf ("function rand(n) {\n");
     printf ("seed = (0x015a4e35 * seed) %% (0x7fffffff);\n");
     printf ("return ((seed >> 16) %% (n));\n");
     printf ("}\n");

     printf ("function rimg(url,extra,n) {\n");
     printf ("return \"<IMG SRC=\\\"\" + url + \"&rand=\" + rand(n) + \"\\\" \" + extra + \">\" \n");
     printf ("}\n");

     printf ("\n//-->\n</SCRIPT>\n");

     printf ("</HEAD>\n");
 /*      printf ("<BODY TEXTCOLOR=\"#000000\" onLoad=\"transfer()\">\n"); */
     printf ("<BODY>\n");
     printf ("<CENTER>\n<TABLE BORDER CELLSPACING=10 CELLPADDING=5>\n");
     getupsvar(monhost, "date", answer, sizeof(answer));
     printf ("<TR><TH>%s</TH>\n", answer);

     printf ("<TH>\n");
     printf ("       <form method=GET action=\"upsstats.cgi\" name=form1>\n");
     printf ("       <input type=hidden name=host value=%s>\n", monhost);
     printf ("       <select onChange=\"document.form1.submit();return true\" name=img1>\n");

     printf ("               <option "); if (img1 == 1) printf ("selected "); printf ("value=1>Battery Capacity\n");
     printf ("               <option "); if (img1 == 2) printf ("selected "); printf ("value=2>Battery Voltage\n");
     printf ("               <option "); if (img1 == 3) printf ("selected "); printf ("value=3>Utility Voltage\n");
     printf ("               <option "); if (img1 == 4) printf ("selected "); printf ("value=4>Output Voltage\n");
     printf ("               <option "); if (img1 == 5) printf ("selected "); printf ("value=5>UPS Load\n");
     printf ("               <option "); if (img1 == 6) printf ("selected "); printf ("value=6>Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=hidden name=img2 value=%s>\n",img2s);
     printf ("       <input type=hidden name=img3 value=%s>\n",img3s);
     printf ("       <input type=hidden name=temp value=%s>\n",temps);
     printf ("       </form>\n");
     printf ("</TH>\n");

     printf ("<TH>\n");
     printf ("       <form method=GET action=\"upsstats.cgi\" name=form2>\n");
     printf ("       <input type=hidden name=host value=%s>\n", monhost);
     printf ("       <input type=hidden name=img1 value=%s>\n",img1s);
     printf ("       <select onChange=\"document.form2.submit();return true\" name=img2>\n");

     printf ("               <option "); if (img2 == 1) printf ("selected "); printf ("value=1>Battery Capacity\n");
     printf ("               <option "); if (img2 == 2) printf ("selected "); printf ("value=2>Battery Voltage\n");
     printf ("               <option "); if (img2 == 3) printf ("selected "); printf ("value=3>Utility Voltage\n");
     printf ("               <option "); if (img2 == 4) printf ("selected "); printf ("value=4>Output Voltage\n");
     printf ("               <option "); if (img2 == 5) printf ("selected "); printf ("value=5>UPS Load\n");
     printf ("               <option "); if (img2 == 6) printf ("selected "); printf ("value=6>Run Time Remaining\n");
     printf ("       </select>\n");
     printf ("       <input type=hidden name=img3 value=%s>\n",img3s);
     printf ("       <input type=hidden name=temp value=%s>\n",temps);
     printf ("       </form>\n");
     printf ("</TH>\n");

     printf ("<TH>\n");
     printf ("       <form method=GET action=\"upsstats.cgi\" name=form3>\n");
     printf ("       <input type=hidden name=host value=%s>\n", monhost);
     printf ("       <input type=hidden name=img1 value=%s>\n",img1s);
     printf ("       <input type=hidden name=img2 value=%s>\n",img2s);
     printf ("       <select onChange=\"document.form3.submit();return true\" name=img3>\n");

     printf ("               <option "); if (img3 == 1) printf ("selected "); printf ("value=1>Battery Capacity\n");
     printf ("               <option "); if (img3 == 2) printf ("selected "); printf ("value=2>Battery Voltage\n");
     printf ("               <option "); if (img3 == 3) printf ("selected "); printf ("value=3>Utility Voltage\n");
     printf ("               <option "); if (img3 == 4) printf ("selected "); printf ("value=4>Output Voltage\n");
     printf ("               <option "); if (img3 == 5) printf ("selected "); printf ("value=5>UPS Load\n");
     printf ("               <option "); if (img3 == 6) printf ("selected "); printf ("value=6>Run Time Remaining\n");

     printf ("       </select>\n");
     printf ("       <input type=hidden name=temp value=%s>\n",temps);
     printf ("       </form>\n");
     printf ("</TH>\n");

     printf ("</TR>\n");

     printf ("<TR><TD BGCOLOR=\"#FFFFFF\">\n<PRE>\n");
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

     printf ("</PRE>\n</TD>\n");

     printf ("<TD ROWSPAN=3><SCRIPT LANGUAGE = \"JavaScript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img1, 1);

     printf (",1000))</SCRIPT>");
     printf ("</TD>\n");

     printf ("<TD ROWSPAN=3><SCRIPT LANGUAGE = \"JavaScript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img2, 3);

     printf (",1000))</SCRIPT>");
     printf ("</TD>\n");

     printf ("<TD ROWSPAN=3><SCRIPT LANGUAGE = \"JavaScript\">\n");
     printf ("document.write(rimg(\"upsimage.cgi?host=%s&display=", monhost);
     send_values(img3, 5);

     printf (",1000))</SCRIPT>");
     printf ("</TD>\n</TR>\n");

     printf ("<TR>\n<TD BGCOLOR=\"#FFFFFF\">\n<PRE>\n");
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
     printf ("</PRE>\n</TD>\n</TR>\n");

     printf ("<TR>\n<TD BGCOLOR=\"#FFFFFF\">\n<PRE>\n");
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

     printf ("</PRE>\n<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n<TR>\n<TD colspan=2>\n<PRE>\n");

    if (getupsvar (monhost, "upstemp", answer, sizeof(answer)) > 0) {
         if (!strcmp(temps,"F")) {
	      tempf = (strtod (answer, 0) * 1.8) + 32;
              printf ("       UPS Temp: %.1f \n</PRE>\n</TD>\n<TD>\n", tempf); 
              printf ("       <form method=GET action=\"upsstats.cgi\" name=form4>\n");
              printf ("       <input type=hidden name=host value=%s>\n",monhost);
              printf ("       <input type=hidden name=img1 value=%s>\n",img1s);
              printf ("       <input type=hidden name=img2 value=%s>\n",img2s);
              printf ("       <input type=hidden name=img3 value=%s>\n",img3s);
              printf ("       <select onChange=\"document.form4.submit();return true\" name=temp>\n");
              printf ("              <option value=\"C\">&deg;C\n");
              printf ("               <option selected value=\"F\">&deg;F\n");
              printf ("               <option value=\"K\">&deg;K\n");
	  }
          if (!strcmp(temps,"K")) {
	       tempf = (strtod (answer, 0)) + 273;
               printf ("       UPS Temp: %.1f \n</PRE>\n</TD>\n<TD>\n", tempf); 
               printf ("       <form method=GET action=\"upsstats.cgi\" name=form4>\n");
               printf ("       <input type=hidden name=host value=%s>\n",monhost);
               printf ("       <input type=hidden name=img1 value=%s>\n",img1s);
               printf ("       <input type=hidden name=img2 value=%s>\n",img2s);
               printf ("       <input type=hidden name=img3 value=%s>\n",img3s);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=temp>\n");
               printf ("              <option value=\"C\">&deg;C\n");
               printf ("               <option value=\"F\">&deg;F\n");
               printf ("               <option selected value=\"K\">&deg;K\n");
	 }
         if ( ( !strcmp(temps,"F") || !strcmp(temps,"K") ) == 0) {
               printf ("       UPS Temp: %s \n</PRE>\n</TD>\n<TD>\n", answer);
               printf ("       <form method=GET action=\"upsstats.cgi\" name=form4>\n");
               printf ("       <input type=hidden name=host value=%s>\n",monhost);
               printf ("       <input type=hidden name=img1 value=%s>\n",img1s);
               printf ("       <input type=hidden name=img2 value=%s>\n",img2s);
               printf ("       <input type=hidden name=img3 value=%s>\n",img3s);
               printf ("       <select onChange=\"document.form4.submit();return true\" name=temp>\n");
               printf ("              <option selected value=\"C\">&deg;C\n");
               printf ("               <option value=\"F\">&deg;F\n");
               printf ("               <option value=\"K\">&deg;K\n");
	 }
    }

     printf ("       </select>\n");
     printf ("       </form>\n");
     printf ("</TD></TR></TABLE>\n");   
     printf ("</TR>\n");

     printf ("<tr><td colspan=4>Most recent events:\n");
     printf ("<form method=post>\n<textarea rows=5 cols=95>\n");

     fetch_events(monhost);
     printf (statbuf);
     printf ("</textarea> \n</form>\n");
     printf ("</td></tr>");

     printf ("</TABLE>\n");
     printf ("</CENTER>\n");
     printf ("</body></HTML>\n");
     return 0;
}
