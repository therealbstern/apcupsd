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
#include "cgilib.h"
#include "upsfetch.h"

static char   monhost[256];

void parsearg(char *var, char *value) 
{
    if (!strcmp(var, "host")) {
	strncpy (monhost, value, sizeof(monhost));
    }
}

static void finish(void)
{
#ifdef VALIDATE_HTML
    printf ("<hr /><div><small>\n");
    printf ("<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">\n");
    printf ("<img style=\"float:right\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\" height=\"31\" width=\"88\"/></a>\n");
    printf("<a href=\"http://validator.w3.org/check/referer\">\n");
    printf("<img style=\"float:right\" src=\"http://www.w3.org/Icons/valid-xhtml10\" alt=\"Valid XHTML 1.0!\" height=\"31\" width=\"88\"/></a>\n");
    printf ("</small></div>\n");
#endif
    printf ("</body></html>\n");
    exit(0);
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
	finish();
    }

    if (!checkhost(monhost)) {
        printf ("<p>Access to %s host is not authorized.</p>\n", monhost);
	finish();
    }

    /* check if host is available */
    if (getupsvar (monhost, "date", answer, sizeof(answer)) <= 0)  {
        printf ("<p>Host %s is not available.</p>\n", monhost);
	finish();
    }
    printf ("<blockquote><pre>");

    html_puts (statbuf);

    printf ("</pre></blockquote>\n");
    finish();
    exit(0);
}
