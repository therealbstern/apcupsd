/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

/*
   Derived from a SMTPclient:

       SMTPclient -- simple SMTP client

       Copyright (C) 1997 Ralf S. Engelschall, All Rights Reserved.
       rse@engelschall.com
       www.engelschall.com

   Kern Sibbald, July 2001
  
   Version $Id: smtp.c,v 1.5 2004-06-20 18:51:52 kerns Exp $
    
 */

#define APCUPSD
#ifdef APCUPSD

#include "apc.h"
#undef main
#define my_name_is(x, y, z)
#define bstrdup(x) strdup(x)
UPSINFO myUPS;
UPSINFO *core_ups = &myUPS;
#define Pmsg2 Dmsg2
#define Pmsg1 Dmsg1
#define Pmsg0 Dmsg0
#define MY_NAME "smtp"

#else

#include "bacula.h"
#include "jcr.h"
#define MY_NAME "bsmtp"

#endif


#ifndef MAXSTRING
#define MAXSTRING 254
#endif

static FILE *sfp;
static FILE *rfp;

static char *from_addr = NULL;
static char *cc_addr = NULL;
static char *subject = NULL;
static char *err_addr = NULL;
static char *mailhost = NULL;
static char *reply_addr = NULL;
static int mailport = 25;
static char my_hostname[MAXSTRING];


/*
 *  examine message from server 
 */
static void get_response(void)
{
    char buf[MAXSTRING];

    Dmsg0(50, "Calling fgets on read socket rfp.\n");
    while (fgets(buf, sizeof(buf), rfp)) {
	buf[strlen(buf)-1] = 0;
        Dmsg2(10, "%s --> %s\n", mailhost, buf);
        if (!isdigit((int)buf[0]) || buf[0] > '3') {
            Pmsg2(0, "Fatal malformed reply from %s: %s\n", mailhost, buf);
	    exit(1);
	}
        if (buf[3] != '-') {
	    break;
	}
    }
    return;
}

/*
 *  say something to server and check the response
 */
static void chat(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(sfp, fmt, ap);
    if (debug_level >= 10) {
       fprintf(stdout, "%s --> ", my_hostname); 
       vfprintf(stdout, fmt, ap);
    }
    va_end(ap);
  
    fflush(sfp);
    if (debug_level >= 10) {
       fflush(stdout);
    }
    get_response();
}


static void usage()
{
   fprintf(stderr,
"\n"
"Usage: %s [-f from] [-h mailhost] [-s subject] [-c copy] [recepient ...]\n"
"       -c          set the Cc: field\n"
"       -dnn        set debug level to nn\n"
"       -f          set the From: field\n"
"       -h          use mailhost:port as the SMTP server\n"
"       -s          set the Subject: field\n"
"       -?          print this message.\n"  
"\n", MY_NAME);

   exit(1);
}


/*********************************************************************
 *
 *  Program to send email
 */
int main (int argc, char *argv[])
{
    char buf[MAXSTRING];
    struct sockaddr_in sin;
    struct hostent *hp;
    int s, r, i, ch;
    struct passwd *pwd;
    char *cp, *p;
    time_t now = time(NULL);
    struct tm tm;

   my_name_is(argc, argv, "bsmtp");

   while ((ch = getopt(argc, argv, "c:d:f:h:r:s:?")) != -1) {
      switch (ch) {
      case 'c':                    
         Dmsg1(20, "cc=%s\n", optarg);
	 cc_addr = optarg;
	 break;

      case 'd':                    /* set debug level */
	 debug_level = atoi(optarg);
	 if (debug_level <= 0) {
	    debug_level = 1; 
	 }
         Dmsg1(20, "Debug level = %d\n", debug_level);
	 break;

      case 'f':                    /* from */
	 from_addr = optarg;
	 break;

      case 'h':                    /* smtp host */
         Dmsg1(20, "host=%s\n", optarg);
         p = strchr(optarg, ':');
	 if (p) {
	    *p++ = 0;
	    mailport = atoi(p);
	 }
	 mailhost = optarg;
	 break;

      case 's':                    /* subject */
         Dmsg1(20, "subject=%s\n", optarg);
	 subject = optarg;
	 break;

      case 'r':                    /* reply address */
	 reply_addr = optarg;
	 break;

      case '?':
      default:
	 usage();

      }  
   }
   argc -= optind;
   argv += optind;

   if (argc < 1) {
      Pmsg0(0, "Fatal error: no recipient given.\n");
      usage();
      exit(1);
   }

   /*
    *  Determine SMTP server
    */
   if (mailhost == NULL) {
      if ((cp = getenv("SMTPSERVER")) != NULL) {
	 mailhost = cp;
      } else {
         mailhost = "localhost";
      }
   }

   /*
    *  Find out my own host name for HELO; 
    *  if possible, get the fully qualified domain name
    */
   if (gethostname(my_hostname, sizeof(my_hostname) - 1) < 0) {
      Pmsg1(0, "Fatal gethostname error: ERR=%s\n", strerror(errno));
      exit(1);
   }
   if ((hp = gethostbyname(my_hostname)) == NULL) {
      Pmsg2(0, "Fatal gethostbyname for myself failed \"%s\": ERR=%s\n", my_hostname,
	 strerror(errno));
      exit(1);
   }
   strcpy(my_hostname, hp->h_name);
   Dmsg1(20, "My hostname is: %s\n", my_hostname);

   /*
    *  Determine from address.
    */
   if (from_addr == NULL) {
      if ((pwd = getpwuid(getuid())) == 0) {
         sprintf(buf, "userid-%d@%s", (int)getuid(), my_hostname);
      } else {
         sprintf(buf, "%s@%s", pwd->pw_name, my_hostname);
      }
      from_addr = bstrdup(buf);
   }
   Dmsg1(20, "From addr=%s\n", from_addr);

   /*
    *  Connect to smtp daemon on mailhost.
    */
hp:
   if ((hp = gethostbyname(mailhost)) == NULL) {
      Pmsg2(0, "Error unknown mail host \"%s\": ERR=%s\n", mailhost,
	 strerror(errno));
      if (strcasecmp(mailhost, "localhost") != 0) {
         Pmsg0(0, "Retrying connection using \"localhost\".\n");
         mailhost = "localhost";
	 goto hp;
      }
      exit(1);
   }

   if (hp->h_addrtype != AF_INET) {
      Pmsg1(0, "Fatal error: Unknown address family for smtp host: %d\n", hp->h_addrtype);
      exit(1);
   }
   memset((char *)&sin, 0, sizeof(sin));
   memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;
   sin.sin_port = htons(mailport);
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      Pmsg1(0, "Fatal socket error: ERR=%s\n", strerror(errno));
      exit(1);
   }
   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      Pmsg2(0, "Fatal connect error to %s: ERR=%s\n", mailhost, strerror(errno));
      exit(1);
   }
   Dmsg0(20, "Connected\n");
   if ((r = dup(s)) < 0) {
      Pmsg1(0, "Fatal dup error: ERR=%s\n", strerror(errno));
      exit(1);
   }
   if ((sfp = fdopen(s, "w")) == 0) {
      Pmsg1(0, "Fatal fdopen error: ERR=%s\n", strerror(errno));
      exit(1);
   }
   if ((rfp = fdopen(r, "r")) == 0) {
      Pmsg1(0, "Fatal fdopen error: ERR=%s\n", strerror(errno));
      exit(1);
   }

   /* 
    *  Send SMTP headers
    */
   get_response(); /* banner */
   chat("helo %s\r\n", my_hostname);
   chat("mail from:<%s>\r\n", from_addr);

   for (i = 0; i < argc; i++) {
      Dmsg1(20, "rcpt to: %s\n", argv[i]);
      chat("rcpt to:<%s>\r\n", argv[i]);
   }

   if (cc_addr) {
      chat("rcpt to:<%s>\r\n", cc_addr);
   }
   Dmsg0(20, "Data\n");
   chat("data\r\n");

   /* 
    *  Send message header
    */
   fprintf(sfp, "From: %s\r\n", from_addr);
   if (subject) {
      fprintf(sfp, "Subject: %s\r\n", subject);
   }
   if (reply_addr) {
      fprintf(sfp, "Reply-To: %s\r\n", reply_addr);
   }
   if (err_addr) {
      fprintf(sfp, "Errors-To: %s\r\n", err_addr);
   }
   if ((pwd = getpwuid(getuid())) == 0) {
      fprintf(sfp, "Sender: userid-%d@%s\r\n", (int)getuid(), my_hostname);
   } else {
      fprintf(sfp, "Sender: %s@%s\r\n", pwd->pw_name, my_hostname);
   }

   fprintf(sfp, "To: %s", argv[0]);
   for (i = 1; i < argc; i++) {
      fprintf(sfp, ",%s", argv[i]);
   }

   fprintf(sfp, "\r\n");
   if (cc_addr) {
      fprintf(sfp, "Cc: %s\r\n", cc_addr);
   }

   /* Add RFC822 date */
   localtime_r(&now, &tm);
   strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", &tm);
   fprintf(sfp, "Date: %s\r\n", buf);

   fprintf(sfp, "\r\n");

   /* 
    *  Send message body 
    */
   while (fgets(buf, sizeof(buf), stdin)) {
      buf[strlen(buf)-1] = 0;
      if (strcmp(buf, ".") == 0) { /* quote lone dots */
         fprintf(sfp, "..\r\n");
      } else {			   /* pass body through unchanged */
         fprintf(sfp, "%s\r\n", buf);
      }
   }

   /* 
    *  Send SMTP quit command
    */
   chat(".\r\n");
   chat("quit\r\n");

   /* 
    *  Go away gracefully ...
    */
   exit(0);
}
