/*
   Copyright (C) 2000, 2001 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   Derived from a SMTPclient:

       SMTPclient -- simple SMTP client

       Copyright (C) 1997 Ralf S. Engelschall, All Rights Reserved.
       rse@engelschall.com
       www.engelschall.com

   Kern Sibbald, July 2001
    
 */

#include "apc.h"
#undef main
#define my_name_is(x)
#define bstrdup(x) strdup(x)
UPSINFO myUPS;
UPSINFO *core_ups = &myUPS;

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

    while (fgets(buf, sizeof(buf), rfp)) {
	buf[strlen(buf)-1] = 0;
        Dmsg2(10, "%s --> %s\n", mailhost, buf);
        if (!isdigit((int)buf[0]) || buf[0] > '3') {
            Dmsg1(10, "unexpected reply: %s", buf);
	    exit(1);
	}
        if (buf[4] != '-')
	    break;
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
"Usage: smtp [-f from] [-h mailhost] [-s subject] [-c copy] [recepient ...]\n"
"       -c          cc\n"
"       -dnn        set debug level to nn\n"
"       -h          set the mailhost:port\n"
"       -s          subject\n"
"       -?          print this message.\n"  
"\n");

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

   my_name_is("smtp");

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
      Dmsg0(0, "No recipient given.\n");
      usage();
      exit(1);
   }

   /*
    *  Determine SMTP server
    */
   if (mailhost == NULL) {
      if ((cp = getenv("SMTPSERVER")) != NULL)
	 mailhost = cp;
      else
         mailhost = "localhost";
   }

   /*
    *  Find out my own host name for HELO; 
    *  if possible, get the FQDN.
    */
   if (gethostname(my_hostname, sizeof(my_hostname) - 1) < 0) {
      Dmsg1(0, "gethostname error: ERR=%s", strerror(errno));
      exit(1);
   }
   if ((hp = gethostbyname(my_hostname)) == NULL) {
      Dmsg2(0, "Error unknown host %s: ERR=%s\n", my_hostname,
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
         sprintf(buf, "userid-%d@%s", getuid(), my_hostname);
      } else {
         sprintf(buf, "%s@%s", pwd->pw_name, my_hostname);
      }
      from_addr = bstrdup(buf);
   }
   Dmsg1(20, "From addr=%s\n", from_addr);

   /*
    *  Connect to smtp daemon on mailhost.
    */
   if ((hp = gethostbyname(mailhost)) == NULL) {
      Dmsg2(0, "Error unknown mail host %s: ERR=%s\n", mailhost,
	 strerror(errno));
      exit(1);
   }
   if (hp->h_addrtype != AF_INET) {
      Dmsg1(0, "unknown address family: %d", hp->h_addrtype);
      exit(1);
   }
   memset((char *)&sin, 0, sizeof(sin));
   memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;
   sin.sin_port = htons(mailport);
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      Dmsg1(0, "socket error: ERR=%s", strerror(errno));
      exit(1);
   }
   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      Dmsg1(0, "connect error: ERR=%s", strerror(errno));
      exit(1);
   }
   Dmsg0(20, "Connected\n");
   if ((r = dup(s)) < 0) {
      Dmsg1(0, "dup error: ERR=%s", strerror(errno));
      exit(1);
   }
   if ((sfp = fdopen(s, "w")) == 0) {
      Dmsg1(0, "fdopen: ERR=%s", strerror(errno));
      exit(1);
   }
   if ((rfp = fdopen(r, "r")) == 0) {
      Dmsg1(0, "fdopen: ERR=%s", strerror(errno));
      exit(1);
   }

   /* 
    *  Give out SMTP headers.
    */
   get_response(); /* banner */
   chat("helo %s\r\n", my_hostname);
   chat("mail from: <%s>\r\n", from_addr);

   for (i = 0; i < argc; i++) {
      Dmsg1(20, "rcpt to: %s\n", argv[i]);
      chat("rcpt to: <%s>\r\n", argv[i]);
   }

   if (cc_addr) {
      chat("rcpt to: <%s>\r\n", cc_addr);
   }
   Dmsg0(20, "Data\n");
   chat("data\r\n");

   /* 
    *  Give out Message header. 
    */
   fprintf(sfp, "From: %s\r\n", from_addr);
   if (subject)
      fprintf(sfp, "Subject: %s\r\n", subject);

   if (reply_addr)
      fprintf(sfp, "Reply-To: %s\r\n", reply_addr);
   if (err_addr)
      fprintf(sfp, "Errors-To: %s\r\n", err_addr);
   if ((pwd = getpwuid(getuid())) == 0) {
      fprintf(sfp, "Sender: userid-%d@%s\r\n", getuid(), my_hostname);
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

   fprintf(sfp, "\r\n");

   /* 
    *  Give out Message body.
    */
   while (fgets(buf, sizeof(buf), stdin)) {
      buf[strlen(buf)-1] = 0;
      if (strcmp(buf, ".") == 0) { /* quote lone dots */
         fprintf(sfp, "..\r\n");
      } else { /* pass thru mode */
         fprintf(sfp, "%s\r\n", buf);
      }
   }

   /* 
    *  Give out SMTP end.
    */
   chat(".\r\n");
   chat("quit\r\n");

   /* 
    *  Die gracefully ...
    */
   exit(0);
}
