/* cgilib - common routines for CGI programs

   Copyright (C) 1999  Russell Kroll <rkroll@exploits.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgilib.h"
#include "cgiconfig.h"

/* 
 * Attempts to extract the arguments from QUERY_STRING
 * it does a "callback" to the parsearg() routine for
 * each argument found.
 * 
 * Returns 0 on failure
 *	   1 on success
 */
int extractcgiargs()
{
	char	*query, *ptr, *eq, *varname, *value, *amp;

        query = getenv ("QUERY_STRING");
	if (query == NULL)
		return 0;	/* not run as a cgi script! */

	/* varname=value&varname=value&varname=value ... */

	ptr = query;

	while (ptr) {
		varname = ptr;
                eq = strchr (varname, '=');
		if (!eq) {
			return 0;     /* Malformed string argument */
		}
		
                *eq = '\0';
		value = eq + 1;
                amp = strchr (value, '&');
		if (amp) {
			ptr = amp + 1;
                        *amp = '\0';
		}
		else
			ptr = NULL;
	
		parsearg (varname, value);
	}

	return 1;
}

/* 
 * Checks if the host to be monitored is in xxx/hosts.conf
 * Returns:
 *	    0 on failure
 *	    1 on success (default if not hosts.conf file)
 */
int checkhost(const char *check)
{
    FILE    *hostlist;
    char    fn[256], buf[256], addr[256];

    snprintf(fn, sizeof(fn), "%s/hosts.conf", CONFPATH);
    hostlist = fopen(fn, "r");

    if (hostlist == NULL)
	return 1;		/* default to allow */

    while (fgets(buf, (size_t) sizeof(buf), hostlist)) {
        if (strncmp("MONITOR", buf, 7) == 0) {
            sscanf (buf, "%*s %s", addr);
	    if (strncmp(addr, check, strlen(check)) == 0) {
		(void) fclose (hostlist);
		return 1;	/* allowed */
	    }
	}
    }
    (void) fclose (hostlist);
    return 0;		    /* denied */
}	

/*
 * Output a string taking care to assure that any html meta characters
 * are output properly.
 *
 * Note: XHTML added the meta character &apos;, but for backwards compatibility
 * with HTML 4.0, output it as &#39;
 */
void html_puts(const unsigned char *p)
{
    while (*p != '\0') {
        if (*p >= 0x7f) {
            printf ("&#%d;", (int) *p);
        } else {
            switch (*p) {
                case '\"':
                    (void) fputs("&quot;", stdout);
                    break;
                case '&':
                    (void) fputs("&amp;", stdout);
                    break;
                case '\'':
                    (void) fputs("&#39;", stdout);
                    break;
                case '<':
                    (void) fputs("&lt;", stdout);
                    break;
                case '>':
                    (void) fputs("&gt;", stdout);
                    break;
                default:
                    (void) putchar(*p);
                    break;
            }
        }
        p++;
    }
}


/*
 * Print the standard http header, html header which is common to all
 * html pages.
 */
void html_begin(const char *title, int refresh)
{
    puts ("Content-type: text/html");
    puts ("Pragma: no-cache\n");

    puts("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"");
    puts("  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
    puts("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">");
    puts ("<head>");
    fputs ("<title>", stdout);
    html_puts(title);
    puts ("</title>");
    puts (" <meta http-equiv=\"Pragma\" content=\"no-cache\" />");
    if (refresh != 0) {
        printf (" <meta http-equiv=\"Refresh\" content=\"%d\" />\n", refresh);
    }

#ifdef EMBEDDED_STYLESHEET
    puts ("<style type=\"text/css\" id=\"internalStyle\">");
    puts ("  body {color: black; background: white}");
    puts ("  div.Center {text-align: center}");
    puts ("  img {border-style: none}");
    puts ("  pre {text-align: left}");
    puts ("  strong {color: red}");
    puts ("  table.Outer {color: black; background: #60a0a0; empty-cells: show; border: solid #60a0a0}");
    puts ("  th.Outer {color: black; background: #60b0b0}");
    puts ("  .Title { font-size: 18pt; }");
    puts ("  .SubTitle { font-size: 12pt; }");
    puts ("  .Empty {color: black; background: lime}");
    puts ("  .Fault {color: black; background: red}");
    puts ("  .Label {color: black; background: aqua}");
    puts ("  .Normal {color: black; background: lime}");
    puts ("  .Warning {color: black; background: yellow}");
    puts ("</style>");
#else
    puts (" <link href=\"apcupsd.css\" rel=\"stylesheet\" type=\"text/css\" />");
#endif
    puts ("</head>");
    puts ("<body>");
}

/*
 * Print the standard footer which appears on every html page and close out
 * the html document.
 */
void html_finish(void)
{
#ifdef VALIDATE_HTML
    puts ("<hr /><div><small>");
    puts ("<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">");
    puts ("<img style=\"float:right\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\" height=\"31\" width=\"88\"/></a>");
    puts ("<a href=\"http://validator.w3.org/check/referer\">");
    puts ("<img style=\"float:right\" src=\"http://www.w3.org/Icons/valid-xhtml10\" alt=\"Valid XHTML 1.0!\" height=\"31\" width=\"88\"/></a>");
    puts ("</small></div>");
#endif
    puts ("</body></html>");
}
