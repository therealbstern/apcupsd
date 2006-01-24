/* gapcmon.c *****************************************************************************
 * Standalone GTK+ Monitor for APCUPSD package
 * 
 * Created Jan 14, 2006
 * James Scott, Jr. <skoona@users.sourceforge.net>
 * 
 * Network utility routines, adapted from package: apcupsd, 
 * file ./src/lib/apclibnis.c
 *
 * 
*/

/*
 * Copyright (C) 2006 James Scott, Jr.   <skoona@users.sourceforge.net>
 * Copyright (C) 1999 Kern Sibbald
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <glib.h>
#include <gtk/gtk.h>


#include "gapcmon.h"

/* private to net_record_error()  */
static int   net_errno  = 0;              /* error number */
static char *net_errno_text = NULL;       /* pointer to system error message */
static char *net_errmsg = NULL;           /* pointer to user error message */
static GtkWidget *wStatus_bar;
static guint i_err_context = 0;               /* status bar message context */

int main( int   argc, char *argv[] );
static void gapc_text_view_prepend(GtkWidget *view, gchar *s);
static void gapc_text_view_append(GtkWidget *view, gchar *s);
static GtkWidget * gapc_create_scrolled_text_view(GtkWidget *box );
static int gapc_parse_args (int argc, char **argv, PGAPC_CONFIG pcfg);
static void cb_exit ( GtkWindow *win, gpointer gp );
static void cb_config_save ( GtkButton *button, gpointer gp );
static void cb_refresh_button ( GtkButton *button, gpointer gp );
static gboolean cb_refresh_timer( gpointer gp );

static gint gapc_monitor_update (PGAPC_CONFIG pcfg);
static gint gapc_text_view_clear_buffer (GtkWidget *view);
static gint gapc_update_hashtable (PGAPC_CONFIG pcfg, gchar *pch_unparsed );
static GtkWidget * gapc_create_user_interface ( PGAPC_CONFIG pcfg );
static gpointer *gapc_network_thread(PGAPC_CONFIG pcfg);

static void net_record_error ( char *e_text, int e_num );
static int  read_nbytes(int fd, char *ptr, int nbytes);
static int  write_nbytes(int fd, char *ptr, int nbytes);
static int  net_recv(int sockfd, char *buff, int maxlen);
static int  net_send(int sockfd, char *buff, int len);
static int  net_open(char *host, char *service, int port);
static void net_close(int sockfd);
static int  net_svcreq ( PGAPC_CONFIG pcfg, char *cp_cmd, char **pch );

/* capture the current network related error values */
static void net_record_error ( char *e_text, int e_num )
{
    gchar *pch = NULL;
    
    net_errno = e_num;

    if ( net_errmsg )
        free (net_errmsg);

    if ( net_errno_text )
        free (net_errno_text);   
        
    net_errmsg = strdup (e_text);
            
    net_errno_text = strdup ( strerror (e_num) );
    net_errno_text[strlen(net_errno_text)] = 0;
    net_errno_text[strlen(net_errno_text)-1] = 0;    

    pch = g_strdup_printf ("%s,[%s]", net_errmsg, net_errno_text);
    
    gtk_statusbar_pop (GTK_STATUSBAR(wStatus_bar), i_err_context);
    gtk_statusbar_push (GTK_STATUSBAR(wStatus_bar), i_err_context, pch);
    
    return ;
}

/*
 * Read nbytes from the network.
 * It is possible that the total bytes require in several
 * read requests
 */

static int read_nbytes(int fd, char *ptr, int nbytes)
{
   int nleft, nread;

   nleft = nbytes;

   while (nleft > 0) 
   {
      do {
          nread = read(fd, ptr, nleft);
         } while (nread == -1 && (errno == EINTR || errno == EAGAIN));

      if (nread <= 0) 
      {
         net_record_error ("read_nbytes:read error", errno);
         return (nread);           /* error, or EOF */
      }

      nleft -= nread;
      ptr += nread;
   }

   return (nbytes - nleft);        /* return >= 0 */
}

/*
 * Write nbytes to the network.
 * It may require several writes.
 */
static int write_nbytes(int fd, char *ptr, int nbytes)
{
   int nleft, nwritten;

   nleft = nbytes;
   while (nleft > 0) 
   {
      nwritten = write(fd, ptr, nleft);

      if (nwritten <= 0) 
      {
         net_record_error ("write_nbytes:write errors", errno);
         return (nwritten);        /* error */
      }

      nleft -= nwritten;
      ptr += nwritten;
   }

   return (nbytes - nleft);
}

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on hard end of file (i.e. network connection close)
 * Returns -2 on error
 */
static int net_recv(int sockfd, char *buff, int maxlen)
{
   int nbytes;
   short pktsiz;

   /* get data size -- in short */
   if ((nbytes = read_nbytes(sockfd, (char *)&pktsiz, sizeof(short))) <= 0) 
   {
      /* probably pipe broken because client died */
      return -1;                   /* assume hard EOF received */
   }
   if (nbytes != sizeof(short))
      return -2;

   pktsiz = ntohs(pktsiz);         /* decode no. of bytes that follow */
   if (pktsiz > maxlen) 
   {
      net_record_error ("net_recv: record length too large.", errno);      
      return -2;
   }
   if (pktsiz == 0)
      return 0;                    /* soft EOF */

   /* now read the actual data */
   if ((nbytes = read_nbytes(sockfd, buff, pktsiz)) <= 0) 
   {
      net_record_error ("net_recv: read_nbytes error.", errno);     
      return -2;
   }
   if (nbytes != pktsiz) 
   {
      net_record_error ("net_recv: error in read_nbytes.", errno);
      return -2;
   }

   return (nbytes);                /* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
static int net_send(int sockfd, char *buff, int len)
{
   int rc;
   short pktsiz;

   
   /* send short containing size of data packet */
   pktsiz = htons((short)len);
   rc = write_nbytes(sockfd, (char *)&pktsiz, sizeof(short));
   if (rc != sizeof(short)) 
   {
      net_record_error ("net_send: write_nbytes error of length prefix.", errno);
      return -1;
   }

   /* send data packet */
   rc = write_nbytes(sockfd, buff, len);
   if (rc != len) 
   {
      net_record_error ("net_send: write_nbytes error.", errno);
      return -1;
   }

   return rc;
}

/*     
 * Open a TCP connection to the UPS network server
 * Returns -1 on error
 * Returns socket file descriptor otherwise
 */
static int net_open(char *host, char *service, int port)
{
   int sockfd;
   struct hostent *hp;
   unsigned int inaddr;            /* Careful here to use unsigned int for */
                                   /* compatibility with Alpha */
   struct sockaddr_in tcp_serv_addr;  /* socket information */
   char net_errbuf[256];              /* error message buffer for messages */

   /* 
    * Fill in the structure serv_addr with the address of
    * the server that we want to connect with.
    */
   memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
   tcp_serv_addr.sin_family = AF_INET;
   tcp_serv_addr.sin_port = htons(port);

   if ((inaddr = inet_addr(host)) != INADDR_NONE) 
   {
      tcp_serv_addr.sin_addr.s_addr = inaddr;
   } 
   else 
   {
      if ((hp = gethostbyname(host)) == NULL) 
      {
         net_record_error ("tcp_open: hostname error.", errno);
         return -1;
      }

      if (hp->h_length != sizeof(inaddr) || hp->h_addrtype != AF_INET) 
      {
         net_record_error ("tcp_open: funny gethostbyname value.", errno);
         return -1;
      }

      tcp_serv_addr.sin_addr.s_addr = *(unsigned int *)hp->h_addr;
   }


   /* Open a TCP socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
   {
      net_record_error ("tcp_open: cannot open stream socket.", errno);
      return -1;
   }

   /* connect to server */
   if (connect(sockfd, (struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr)) < 0) 
   {
      snprintf(net_errbuf, sizeof(net_errbuf),
         "tcp_open: cannot connect to server %s on port %d.ERR=%s", 
         host, port, strerror(errno));
         
      net_record_error (net_errbuf, errno );
      
      close(sockfd);
      return -1;
   }

   return sockfd;
}

/* Close the network connection */
static void net_close(int sockfd)
{
   short pktsiz = 0;

   /* send EOF sentinel */
   write_nbytes(sockfd, (char *)&pktsiz, sizeof(short));
   close(sockfd);
}

static int net_svcreq ( PGAPC_CONFIG pcfg, char *cp_cmd, char **pch )
{
   int sockfd, n, i_port;
   int iflag = 0;
   char recvline[256];

   i_port = atoi(pcfg->pch_port);
   if ((sockfd = net_open( pcfg->pch_host, NULL, i_port)) < 0)
        return -1;
   
       if ( (net_send(sockfd, cp_cmd, strlen (cp_cmd) )) < 0 )
             return -1;
                 
       /* clear current data */      
       for ( iflag = 0; iflag < GAPC_MAX_ARRAY-1; iflag++)
       {
          g_free (pch[iflag]);
          pch[iflag] = NULL;                          
       }
             
       iflag = 0;    
       while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) 
       {
          recvline[n] = 0;
          pch[iflag++] = g_strdup (recvline);

          if (g_str_equal (cp_cmd, "status") && iflag > 1 )
              gapc_update_hashtable (pcfg, recvline );

          if (iflag > (GAPC_MAX_ARRAY - 2))
             break;
       }
       if (iflag > 0)
       {
            if (pch[iflag+1])
                g_free (pch[iflag+1]);
                
            pch[iflag+1] = NULL;                
       }
               
   net_close(sockfd);   
   
   return iflag; /* record */
}


static gint gapc_update_hashtable (PGAPC_CONFIG pcfg, gchar *pch_unparsed )
{
    gchar *pch_in = NULL;
    gchar *pch = NULL;
    gint  ilen = 0;
    
    
    /* unparsed contains - keystring : keyvalue nl */
    ilen = strlen (pch_unparsed);
    pch_in = g_strdup (pch_unparsed);
    pch_in[ilen--] = 0;    
    pch_in[ilen--] = 0;        
    pch = g_strstr_len ( pch_in, ilen, ":");
    *pch = 0;
    pch_in = g_strchomp ( pch_in);
    pch++;
    pch = g_strstrip(pch);
    
    g_hash_table_replace (pcfg->pht_Status, g_strdup (pch_in), g_strdup (pch) );
   
    g_free (pch_in);
    
    return ilen;
}

static void cb_exit ( GtkWindow *win, gpointer gp )
{
    PGAPC_CONFIG pcfg = gp;
    
    pcfg->b_run = FALSE; /* Stop the thread */
    gtk_timeout_remove (pcfg->ui_refresh_timer);        
    g_hash_table_destroy (pcfg->pht_Status);
    g_hash_table_destroy (pcfg->pht_Widgets);    
    gtk_main_quit();
    
}

static void cb_config_apply (GtkButton *button, gpointer gp)
{
    PGAPC_CONFIG pcfg = gp;
    gchar       *pch = NULL;
    GtkEntry    *ef = NULL;
    gdouble      i_x = 0;
    
       
    /* get current value of entry-fields */
    ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerAddress");    
    if (pcfg->pch_host != NULL )
        g_free (pcfg->pch_host);

    pch = (gchar *)gtk_entry_get_text ( ef );
    pcfg->pch_host = strdup (pch);

    
    ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerPort");
    if (pcfg->pch_port != NULL )
        g_free (pcfg->pch_port);
        
    pch = (gchar *)gtk_entry_get_text ( ef );
    pcfg->pch_port = strdup (pch);

    
    ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerCycle");
    i_x = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(ef) );    
    g_ascii_dtostr ( pcfg->pch_interval, sizeof(pcfg->pch_interval), i_x);
    pcfg->i_update_cycle = i_x * GAPC_THREAD_CYCLE ;
            

}

static void cb_config_save (GtkButton *button, gpointer gp)
{
    PGAPC_CONFIG pcfg = gp;
    FILE        *fp   = NULL;        
    gchar       ch[512];
    
    
    cb_config_apply ( button, gp);     /* use existing code */

    fp = fopen (pcfg->pch_config_file, "w");
    if (fp == NULL)     
        return;
        
    g_snprintf ( ch, sizeof(ch), "%s\n", pcfg->pch_host );
    fputs ( ch, fp );
    g_snprintf ( ch, sizeof(ch), "%s\n", pcfg->pch_port );
    fputs ( ch, fp );
    g_snprintf ( ch, sizeof(ch), "%s\n", pcfg->pch_interval );
    fputs ( ch, fp );
    
    fclose (fp);
}

static void cb_refresh_button ( GtkButton *button, gpointer gp )
{
    PGAPC_CONFIG pcfg = gp;
   
    if ( !gapc_monitor_update ( pcfg) )
    {
        gtk_statusbar_pop (GTK_STATUSBAR(wStatus_bar), pcfg->i_info_context);    
        gtk_statusbar_push (GTK_STATUSBAR(wStatus_bar), pcfg->i_info_context,
                            "Refresh Button Skipped...  Thread is busy");                           
    }

    return;        
}

static gboolean cb_refresh_timer( gpointer gp )
{
    PGAPC_CONFIG pcfg = gp;

    
    if ( !gapc_monitor_update ( pcfg) )
    {
        gtk_statusbar_pop (GTK_STATUSBAR(wStatus_bar), pcfg->i_info_context);    
        gtk_statusbar_push (GTK_STATUSBAR(wStatus_bar), pcfg->i_info_context,
                            "Automatic Update Skipped...  Thread is busy");                           
    }
       
    return TRUE;
}

static gint gapc_text_view_clear_buffer (GtkWidget *view)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer = NULL;
    
    buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_delete (buffer, &start, &end);
    
    return FALSE;
}

static void gapc_text_view_prepend(GtkWidget *view, gchar *s)
    {
    GtkTextIter     iter;
    GtkTextBuffer   *buffer;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_start_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, s, -1);
    }

static void gapc_text_view_append(GtkWidget *view, gchar *s)
    {
    GtkTextIter     iter;
    GtkTextBuffer   *buffer;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, s, -1);
    }


static GtkWidget * gapc_create_scrolled_text_view(GtkWidget *box )
{
    PangoFontDescription *font_desc;    
    GtkWidget   *scrolled,
                *view;

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_ETCHED_IN);   
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolled), 
                                    GTK_POLICY_AUTOMATIC, 
                                    GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(box), scrolled );            

    view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled), view);
    
    /* Change default font throughout the widget */
    font_desc = pango_font_description_from_string ("Monospace 9");
    gtk_widget_modify_font (view, font_desc);
    pango_font_description_free (font_desc);
    
    gtk_text_view_set_left_margin ( GTK_TEXT_VIEW(view), 5 );

    return view;
}

static int gapc_parse_args (int argc, char **argv, PGAPC_CONFIG pcfg)
{
    FILE        *fp       = NULL;    
    gchar       *pch_host = NULL;
    gchar       *pch_port = NULL;
    gchar       *pch      = NULL;    
    gchar       *pch_interval = NULL;
    gchar       ch_buffer[512];    
    gboolean    b_syntax = FALSE;

    /* *********************************************************** *
     *  Get user input
     *   - default to know values
     *   - check config file for saved values -- careful not to override cmdline
     */
    while ( --argc > 0 )           /* ADJUST COUNTER HERE */
    {
        pch = strstr ( argv[argc], "-host" );
        if ( pch != NULL)
        {
            if ( argc >= 1 )
            {
                 if ( (strlen (argv[argc+1])) > 4 )
                 {
                       pch_host = g_strdup ( argv[argc+1] );
                       continue;                       
                 }
                 g_print ("\ngapcmon: Error :>Invalid host name or address !\n\n" );
            }
            b_syntax = TRUE;
        }

        pch = strstr ( argv[argc], "-port" );
        if ( pch != NULL)
        {
            if ( argc >= 1 )
            {
                 if ( (strlen (argv[argc+1])) > 2 )
                 {
                       pch_port = g_strdup ( argv[argc+1] );
                       continue;                       
                 }
                 g_print ("\ngapcmon: Error :>Invalid port number !\n\n" );
            }
            b_syntax = TRUE;
        }

        pch = strstr ( argv[argc], "-help" );
        if ( (pch != NULL) || b_syntax )
        {
            g_print ("\nsyntax: gapcmon [--help] [--host server.mynetwork.net|127.0.0.1] [--port 9999]\n"
                     "where:  --help, this message\n"
                     "        --host, the domain name or ip address of the server\n"
                     "        --port, the port number of the apcupsd nis service\n"                     
                     "        null, defaults to localhost and port 3551\n\n"
                     "Skoona@Users.SourceForge.Net (GPL) 2006 \n\n"
                    );                    
            return  TRUE;  /* trigger exit */
        }
    }
    
    
   /* *********************************************************
   * Apply defaults
   */
   pcfg->pch_config_file = g_strdup_printf ("%s/.gapcmon.config.user", getenv("HOME") );
   if ( g_file_test ( pcfg->pch_config_file, G_FILE_TEST_IS_REGULAR) )
   {
      fp = fopen (pcfg->pch_config_file, "r");
      if (fp != NULL)     
      {
        if ( fgets ( ch_buffer, sizeof(ch_buffer), fp) != NULL )
            if (pch_host == NULL )
            {
                strtok (ch_buffer, "\n");
                pch_host = g_strdup (ch_buffer);     
            }
        if ( fgets ( ch_buffer, sizeof(ch_buffer), fp) != NULL )
            if (pch_port == NULL )
            {
                strtok (ch_buffer, "\n");
                pch_port = g_strdup (ch_buffer);     
            }                
        if ( fgets ( ch_buffer, sizeof(ch_buffer), fp) != NULL )
            {
                strtok (ch_buffer, "\n");
                pch_interval = g_strdup (ch_buffer);                
            }                
            
        fclose (fp);
      }      
   }
   else
   {
         if (  pch_host == NULL  )
               pch_host = g_strdup ("localhost");

         if (  pch_port == NULL  )
               pch_port = g_strdup ("3551");
         
         pch_interval  = g_strdup ("5");         
    }

    if (  pch_interval == NULL  )
               pch_interval = g_strdup ("5");
   
    pcfg->pch_host = pch_host;
    pcfg->pch_port = pch_port;
    pcfg->pch_interval = pch_interval;    
    
    return FALSE;
}    

static gpointer *gapc_network_thread(PGAPC_CONFIG pcfg)
{
    while ( pcfg->b_run )
    {      
        g_mutex_lock (pcfg->gm_update);      
            net_svcreq ( pcfg, "status", pcfg->pach_status );
            net_svcreq ( pcfg, "events", pcfg->pach_events );
        g_mutex_unlock (pcfg->gm_update);      
               
        g_usleep ( pcfg->i_update_cycle );
    }
        
    g_thread_exit ( NULL );
}

static gint gapc_monitor_update (PGAPC_CONFIG pcfg)
{
   gint  i_x = 0;
   GtkWidget *win = NULL, *w = NULL;
   gchar     *pch = NULL, *pch1 = NULL, *pch2 = NULL;
   gdouble    dValue = 0.00;
   gchar     ch_buffer[256];

   if ( !g_mutex_trylock (pcfg->gm_update) )
       return FALSE; /* thread must be busy */
   
   w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusPage");
   gapc_text_view_clear_buffer (GTK_WIDGET( w ));
   for (i_x = 1; pcfg->pach_status[i_x] != NULL; i_x++)        
       gapc_text_view_append(GTK_WIDGET( w ), pcfg->pach_status[i_x]);

   w = g_hash_table_lookup (pcfg->pht_Widgets, "EventsPage");
   gapc_text_view_clear_buffer (GTK_WIDGET( w ));    
   for (i_x = 0; pcfg->pach_events[i_x] != NULL; i_x++)            
       gapc_text_view_prepend(GTK_WIDGET( w ), pcfg->pach_events[i_x]);    
   
   win = g_hash_table_lookup (pcfg->pht_Widgets, "UtilityAC");
   pch = g_hash_table_lookup (pcfg->pht_Status, "LINEV");  
   gtk_progress_bar_set_text (GTK_PROGRESS_BAR(win), pch);   
   dValue = g_strtod (pch, NULL);
   dValue /= 140.0;
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(win), dValue);  

   win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryVoltage");
   pch = g_hash_table_lookup (pcfg->pht_Status, "BATTV");
   gtk_progress_bar_set_text (GTK_PROGRESS_BAR(win), pch);   
   dValue = g_strtod (pch, NULL);
   dValue /= 30.0;
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(win), dValue);  

   win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryCharge");
   pch = g_hash_table_lookup (pcfg->pht_Status, "BCHARGE");
   gtk_progress_bar_set_text (GTK_PROGRESS_BAR(win), pch);   
   dValue = g_strtod (pch, NULL);
   dValue /= 100.0;
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(win), dValue);  

   win = g_hash_table_lookup (pcfg->pht_Widgets, "UPSLoad");
   pch = g_hash_table_lookup (pcfg->pht_Status, "LOADPCT");
   gtk_progress_bar_set_text (GTK_PROGRESS_BAR(win), pch);   
   dValue = g_strtod (pch, NULL);
   dValue /= 100.0;
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(win), dValue);  
      
   win = g_hash_table_lookup (pcfg->pht_Widgets, "TimeRemaining");
   pch = g_hash_table_lookup (pcfg->pht_Status, "TIMELEFT");
   gtk_progress_bar_set_text (GTK_PROGRESS_BAR(win), pch);   
   dValue = g_strtod (pch, NULL);
   dValue /= 15.00;
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(win), dValue);  

   win = g_hash_table_lookup (pcfg->pht_Widgets, "TitleStatus");
   pch = g_hash_table_lookup (pcfg->pht_Status, "STATUS");
   pch1 = g_hash_table_lookup (pcfg->pht_Status, "UPSNAME");
   pch2 = g_hash_table_lookup (pcfg->pht_Status, "HOSTNAME");
   g_snprintf (ch_buffer, sizeof(ch_buffer), 
   "<span foreground=\"blue\" size=\"medium\">Monitored UPS %s on %s is %s</span>", 
               pch1, pch2, pch);
   gtk_label_set_markup(GTK_LABEL(win), ch_buffer);   
      
   g_mutex_unlock (pcfg->gm_update);      
      
   return TRUE;
}

static GtkWidget * gapc_create_user_interface ( PGAPC_CONFIG pcfg )
{
    GtkWidget *window, *frame, *notebook, 
              *label,  *text, 
              *button, *dbutton,
              *pbox, *lbox, *rbox,
              *entry,  *spin, *pbar, 
              *table, *table2;
    GtkAdjustment *gAdj;
    GtkSizeGroup  *gSize;
    GdkScreen   *screen   = NULL;
    gint        i_x = 0;
    gint        i_y = 0;       
        
   /* *********************************************************
   * Create main window and display
   */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_container_set_border_width (GTK_CONTAINER (window), 0);    
        gtk_window_set_title (GTK_WINDOW(window), "gapcmon UPS Monitor");
        gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    /* *********************************************************
     * Calculate window size based on current desktop sizes
     */
    screen = gtk_window_get_screen ( GTK_WINDOW(window) );
        i_x = gdk_screen_get_width (screen);
        i_x = (gint)((gfloat)i_x * 0.44);
        i_y = gdk_screen_get_height (screen);
        i_y = (gint)((gfloat)i_y * 0.34);
        gtk_widget_set_size_request (window, i_x/2, i_y);  // set minimum size
        gtk_window_set_default_size ( GTK_WINDOW(window), i_x, i_y ); // set inital size

        g_signal_connect (G_OBJECT (window), "destroy",
                          G_CALLBACK (cb_exit), pcfg);

    table = gtk_table_new (6, 4, FALSE);     
            gtk_container_add (GTK_CONTAINER (window), table);       
      
    label = gtk_label_new ("UPS Monitor");
        g_hash_table_insert (pcfg->pht_Widgets, g_strdup("TitleStatus"), label );
        gtk_table_attach (GTK_TABLE (table), label, 
                        /* X direction */          /* Y direction */
                        0, 3,                      0, 1,
                        GTK_EXPAND | GTK_FILL,     0,
                        0,                         5);
    
    /* Create a new notebook, place the position of the tabs */
    notebook = gtk_notebook_new ();
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
        gtk_notebook_set_scrollable(GTK_NOTEBOOK (notebook), TRUE);
        gtk_table_attach (GTK_TABLE (table), notebook, 
                        /* X direction */          /* Y direction */
                        0, 3,                      1, 3,
                        GTK_EXPAND | GTK_FILL,     GTK_EXPAND | GTK_FILL,
                        5,                         0);
        

    /* Create the Overview Notebook Page */
    frame = gtk_frame_new ("UPS Status Overview");
        gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
        gtk_container_set_border_width (GTK_CONTAINER (frame), 10);        
        label = gtk_label_new ("Overview");    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

        pbox = gtk_hbox_new (FALSE, 4);
        gtk_container_add (GTK_CONTAINER (frame), pbox );

        lbox = gtk_vbox_new (TRUE, 2);
        gtk_container_add (GTK_CONTAINER (pbox), lbox );
        rbox = gtk_vbox_new (TRUE, 2);
        gtk_container_add (GTK_CONTAINER (pbox), rbox );
        gtk_container_add (GTK_CONTAINER (pbox), gtk_vbox_new (TRUE, 2) );

        gSize = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

        label = gtk_label_new ("Utility AC Voltage:");            
            gtk_size_group_add_widget (gSize, label);
        pbar = gtk_progress_bar_new ();
        gtk_box_pack_start ( GTK_BOX (lbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start ( GTK_BOX (rbox), pbar, TRUE, TRUE, 4);        
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 0.87);
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("UtilityAC"), pbar );
   
        label = gtk_label_new ("Battery Voltage:");            
            gtk_size_group_add_widget (gSize, label);        
        pbar = gtk_progress_bar_new ();
        gtk_box_pack_start ( GTK_BOX (lbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start ( GTK_BOX (rbox), pbar, TRUE, TRUE, 4);        
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("BatteryVoltage"), pbar );
        
        label = gtk_label_new ("Battery Charge:");   
            gtk_size_group_add_widget (gSize, label);                 
        pbar = gtk_progress_bar_new ();
        gtk_box_pack_start ( GTK_BOX (lbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start ( GTK_BOX (rbox), pbar, TRUE, TRUE, 4);        
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);        
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("BatteryCharge"), pbar );

        label = gtk_label_new ("UPS Load:");            
            gtk_size_group_add_widget (gSize, label);
        pbar = gtk_progress_bar_new ();
        gtk_box_pack_start ( GTK_BOX (lbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start ( GTK_BOX (rbox), pbar, TRUE, TRUE, 4);        
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 0.57);        
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("UPSLoad"), pbar );        
        
        label = gtk_label_new ("Time Remaining:");   
            gtk_size_group_add_widget (gSize, label);         
        pbar = gtk_progress_bar_new ();
        gtk_box_pack_start ( GTK_BOX (lbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start ( GTK_BOX (rbox), pbar, TRUE, TRUE, 4);        
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);        
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("TimeRemaining"), pbar );

       
    /* Create a Notebook Page */
    frame = gtk_frame_new ("Power Events");
        gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
        label = gtk_label_new ("Events");    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
        text = gapc_create_scrolled_text_view (frame ); 
        g_hash_table_insert (pcfg->pht_Widgets, g_strdup("EventsPage"), text );
                
    /* Create a Notebook Page */
    frame = gtk_frame_new ("Full Status");
        gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
        label = gtk_label_new ("Status");    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
        text = gapc_create_scrolled_text_view (frame );
        g_hash_table_insert (pcfg->pht_Widgets, g_strdup("StatusPage"), text );
        
    /* Create Configuration Notebook Page */
    frame = gtk_frame_new ("Configuration Options");
        gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
        gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
        label = gtk_label_new ("Config");    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

    table2 = gtk_table_new (4, 4, FALSE);     
             gtk_container_add (GTK_CONTAINER (frame), table2);       

        label = gtk_label_new ("Server hostname or ip address:");            
        entry = gtk_entry_new_with_max_length   ( 255 );
        gtk_table_attach (GTK_TABLE (table2), label, 
                        /* X direction */          /* Y direction */
                        0, 1,                      0, 1,
                        GTK_FILL,                         0,
                        0,                         5);
        gtk_table_attach (GTK_TABLE (table2), entry, 
                        /* X direction */          /* Y direction */
                        1, 3,                      0, 1,
                        GTK_EXPAND | GTK_FILL,     0,
                        2,                         5);
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
            gtk_entry_set_text (GTK_ENTRY(entry), pcfg->pch_host);
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("ServerAddress"), entry );            


        label = gtk_label_new ("Server port number:");            
        gtk_table_attach (GTK_TABLE (table2), label, 
                        /* X direction */          /* Y direction */
                        0, 1,                      1, 2,
                        GTK_FILL,                         0,
                        0,                         5);
        entry = gtk_entry_new_with_max_length   ( 10 );
        gtk_table_attach (GTK_TABLE (table2), entry, 
                        /* X direction */          /* Y direction */
                        1, 2,                      1, 2,
                        GTK_FILL,                         0,
                        2,                         5);
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
            gtk_entry_set_text (GTK_ENTRY(entry), pcfg->pch_port);        
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("ServerPort"), entry );            

        gAdj = (GtkAdjustment *)gtk_adjustment_new ( 5.0, 1.0, 60.0, 1.0, 5.0, 4.0);
        spin = gtk_spin_button_new ( gAdj, 1, 0 );
        label = gtk_label_new ("Refresh cycle 10sec:");            
        gtk_table_attach (GTK_TABLE (table2), label, 
                        /* X direction */          /* Y direction */
                        0, 1,                      2, 3,
                        GTK_FILL,                  0,
                        0,                         5);
        gtk_table_attach (GTK_TABLE (table2), spin, 
                        /* X direction */          /* Y direction */
                        1, 2,                      2, 3,
                        GTK_FILL,                  0,
                        2,                         5);
        gtk_misc_set_alignment ( (GtkMisc *)label, 1.0, 1.0);
            gtk_spin_button_set_value ((GtkSpinButton *)spin, 
                                        g_strtod(pcfg->pch_interval, NULL));
            g_hash_table_insert (pcfg->pht_Widgets, g_strdup("ServerCycle"), spin );            
        
        button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
        gtk_table_attach (GTK_TABLE (table2), button, 
                        /* X direction */          /* Y direction */
                        1, 2,                      3, 4,
                        0,                         0,
                        0,                         5);
            g_signal_connect (button, "clicked", 
                          G_CALLBACK (cb_config_apply), pcfg);
        button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
        gtk_table_attach (GTK_TABLE (table2), button, 
                        /* X direction */          /* Y direction */
                        2, 3,                      3, 4,
                        0,                         0,
                        5,                         5);
            g_signal_connect (button, "clicked", 
                          G_CALLBACK (cb_config_save), pcfg);

    /* Control buttons */               
    dbutton = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
        g_signal_connect (dbutton, "clicked", 
                          G_CALLBACK (cb_refresh_button), pcfg);
        gtk_table_attach (GTK_TABLE (table), dbutton, 
                        /* X direction */          /* Y direction */
                        0, 1,                      3, 4,
                        0,     0,
                        0,                         2);
                          
    button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
        g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit), NULL);
        gtk_table_attach (GTK_TABLE (table), button, 
                        /* X direction */          /* Y direction */
                        2, 3,                      3, 4,
                        0,     0,
                        0,                         2);

    wStatus_bar = gtk_statusbar_new ();    
        gtk_table_attach (GTK_TABLE (table), wStatus_bar, 
                        /* X direction */          /* Y direction */
                        0, 3,                      4, 5,
                        GTK_EXPAND | GTK_FILL,     0,
                        0,                         0);
                
    i_err_context = gtk_statusbar_get_context_id ( GTK_STATUSBAR(wStatus_bar), 
                                               "Network Error Reports");
    pcfg->i_info_context = gtk_statusbar_get_context_id ( GTK_STATUSBAR(wStatus_bar), 
                                               "Informational");
                
    gtk_widget_show_all (window);    

    gtk_notebook_set_current_page ( GTK_NOTEBOOK (notebook), 0 );
    GTK_WIDGET_SET_FLAGS (dbutton, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (dbutton);
    
    return window;
}


int main( int   argc, char *argv[] )
{
    GAPC_CONFIG  cfg;
    GtkWidget *window = NULL, *spin = NULL;
    gint          i_x = 0;
    
    
    /* Initialize Thread Support and GTK */
    g_thread_init(NULL);
    gdk_threads_init();
    gtk_init (&argc, &argv);

    memset ( &cfg, 0, sizeof (cfg) );

    cfg.gm_update = g_mutex_new();    
    
    if ( gapc_parse_args (argc, argv, &cfg) )
        return 1; /* exit if user only wanted help */

    /* *****************************************************************
     * Create hash table for easy access to status info and widgets
     */
    cfg.pht_Status = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                            g_free, g_free );

    cfg.pht_Widgets = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                             g_free, NULL );
      
      
    window = gapc_create_user_interface ( &cfg );  


    /* Create an update thread to handle network io */
    spin = g_hash_table_lookup (cfg.pht_Widgets, "ServerCycle" );
    i_x = gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(spin));    
    cfg.i_update_cycle = i_x * GAPC_THREAD_CYCLE ;
    cfg.b_run = TRUE;
    cfg.tid_refresh = g_thread_create( (GThreadFunc)gapc_network_thread, &cfg, FALSE, NULL );
    
    /* Add a timer callback to refresh ups data 1 sec = 1/1000 */    
    cfg.ui_refresh_timer = gtk_timeout_add ( GAPC_REFRESH_INCREMENT, 
                                             cb_refresh_timer, &cfg);
                                             
  /* enter the GTK main loop */
    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    /* cleanup big memory arrays */
    for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
        if (cfg.pach_status[i_x])
            g_free(cfg.pach_status[i_x]);
    for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)        
        if (cfg.pach_events[i_x])    
            g_free(cfg.pach_events[i_x]);


    return(0);
}

