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
#include <sys/types.h>

#include "cgiconfig.h"
#include "cgilib.h"
#include "upsfetch.h"

#ifndef DEFAULT_REFRESH
#define DEFAULT_REFRESH 30
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static char   monhost[MAXHOSTNAMELEN] = "127.0.0.1";
static int    refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
        strncpy (monhost, value, sizeof(monhost));
        monhost[sizeof(monhost) - 1] = '\0';

    } else if (strcmp(var, "refresh") == 0) {
        refresh = atoi(value);
        if (refresh < 0) {
            refresh = DEFAULT_REFRESH;
        }
    }
}

int main(int argc, char **argv) 
{
    char   answer[256];

    (void) extractcgiargs();

    printf ("Content-type: text/html\n");
    printf ("Pragma: no-cache\n\n");

    printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n");
    printf("  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    printf("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">\n");
    printf ("<head>\n");
    printf ("<title>APCUPSD STATUS Output Page</title>\n");
    printf ("<meta http-equiv=\"Pragma\" content=\"no-cache\" />\n");
    if (refresh != 0) {
        printf ("<meta http-equiv=\"Refresh\" content=\"%d\" />\n", refresh);
    }
    printf ("<style type=\"text/css\" id=\"internalStyle\">\n");
    printf ("  body {color: black; background: #ffffff}\n");
    printf ("</style>\n");
    printf ("</head>\n");
    printf ("<body>\n");

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
