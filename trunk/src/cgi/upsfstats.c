/*
 *  Program to print the full status output from apcupsd
 *
 *   Kern Sibbald, 17 November 1999
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "cgiconfig.h"
#include "upsfetch.h"

char   monhost[256];
char   answer[256];

extern int extractcgiargs();
extern int checkhost(char *host);

void parsearg(char var[255], char value[255]) 
{
    if (!strcmp(var, "host")) {
	strncpy (monhost, value, sizeof(monhost));
    }
}

static void bailout()
{
    printf ("</BODY></HTML>\n");
    exit(0);
}


int main() 
{
    strcpy (monhost, "127.0.0.1");  /* default host */

    printf ("Content-type: text/html\n");
    printf ("\n");

    printf ("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
    printf ("<HTML>\n");
    printf ("<HEAD><TITLE>APCUPSD STATUS Output Page</TITLE></HEAD>\n");
    printf ("<BODY BGCOLOR=\"#FFFFFF\">\n");

    if (!extractcgiargs()) {
        printf("Unable to extract cgi arguments!\n");
	bailout();
    }

    if (!checkhost(monhost)) {
        printf ("Access to %s host is not authorized.\n", monhost);
	bailout();
    }

    /* check if host is available */
    if (getupsvar (monhost, "date", answer, sizeof(answer)) <= 0)  {
        printf ("Host %s is not available.\n", monhost);
	bailout();
    }
    printf ("<blockquote><pre>");

    printf (statbuf);

    printf ("</pre></blockquote>\n");

#ifdef VALIDATE_HTML
    printf ("<div><small>\n");
    printf ("<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">\n");
    printf ("<img style=\"float:right\" src=\"/icons/vcss\" alt=\"Valid CSS!\" height=\"31\" width=\"88\"/></a>\n");
    printf("<a href=\"http://validator.w3.org/check/referer\">\n");
    printf("<img style=\"float:right\" src=\"/icons/valid-xhtml10\" alt=\"Valid XHTML 1.0!\" height=\"31\" width=\"88\"/></a>\n");
    printf ("</small></div>\n");
#endif
    printf ("</BODY></HTML>\n");

    exit(0);
}
