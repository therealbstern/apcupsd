/*
 *  Program to print the full status output from apcupsd
 *
 *   Kern Sibbald, 17 November 1999
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgiconfig.h"
#include "cgilib.h"
#include "upsfetch.h"

static char   monhost[256];

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
	strncpy (monhost, value, sizeof(monhost));
	monhost[sizeof(monhost) - 1] = '\0';
    }
}

int main(int argc, char **argv) 
{
    char   answer[256];

    strcpy (monhost, "127.0.0.1");  /* default host */

    printf ("Content-type: text/html\n");
    printf ("Pragma: no-cache\n\n");

    printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n");
    printf("  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    printf("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">\n");
    printf ("<head>\n");
    printf ("<title>APCUPSD STATUS Output Page</title>\n");
    printf ("<meta http-equiv=\"Pragma\" content=\"no-cache\" />\n");
    printf ("<style type=\"text/css\" id=\"internalStyle\">\n");
    printf ("  body {color: black; background: #ffffff}\n");
    printf ("</style>\n");
    printf ("</head>\n");
    printf ("<body>\n");

    if (!extractcgiargs()) {
        printf("<p>Unable to extract cgi arguments!</p>\n");
	html_finish();
	exit (0);
    }

    if (!checkhost(monhost)) {
        printf ("<p>Access to %s host is not authorized.</p>\n", monhost);
	html_finish();
	exit (0);
    }

    /* check if host is available */
    if (getupsvar (monhost, "date", answer, sizeof(answer)) <= -1)  {
        printf ("<p>Unable to communicate with the UPS on %s.</p>\n", monhost);
	html_finish();
	exit (0);
    }
    printf ("<blockquote><pre>");

    html_puts (statbuf);

    printf ("</pre></blockquote>\n");
    html_finish();
    exit(0);
}
