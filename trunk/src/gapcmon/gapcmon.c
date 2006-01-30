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

/* **************************************************************************
 * Hash Tables useage
 *                     key          value          key-free  value-free
 * pht_Status      Datafield.Name   Data.Ptr        yes       yes      
 * pht_Widgets     Widget.Name      *widget.ptr     yes       no
 * 
 * apcupsd status output is parsed and placed (replaced) into pht_Status
 *         data pointers for the custom barchart is also contained in pht_Status 
 * any widget that might need its value updated is inserted into pht_Widgets
 * 
 * String Arrays:
 * pach_status and pach_events contain the un-altered text received from NIS 
 *      during the last network io (every 30 seconds).
 * 
 * *****************************************
 * indent prog.c -o prog.c -gnu -l90 -lc90
 * *****************************************
*/


#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>

#include "gapcmon.h"

int main (int argc, char *argv[]);
static void gapc_text_view_prepend (GtkWidget * view, gchar * s);
static void gapc_text_view_append (GtkWidget * view, gchar * s);
static GtkWidget *gapc_create_scrolled_text_view (GtkWidget * box);
static gint gapc_parse_args (gint argc, gchar ** argv, PGAPC_CONFIG pcfg);
static void cb_button_quit (GtkButton * button, gpointer gp);
static void cb_config_save (GtkButton * button, gpointer gp);
static void cb_refresh_button (GtkButton * button, gpointer gp);
static gboolean cb_refresh_timer (gpointer gp);
static gboolean cb_first_refresh_timer (gpointer gp);

static gint gapc_monitor_update (PGAPC_CONFIG pcfg);
static gint gapc_text_view_clear_buffer (GtkWidget * view);
static gint gapc_update_hashtable (PGAPC_CONFIG pcfg, gchar * pch_unparsed);
static GtkWidget *gapc_create_user_interface (PGAPC_CONFIG pcfg);
static gpointer *gapc_network_thread (PGAPC_CONFIG pcfg);
static gboolean cb_h_bar_chart_exposed (GtkWidget * widget, GdkEventExpose * event,
					gpointer data);
static GtkWidget *gapc_create_h_barchart (PGAPC_CONFIG pcfg, GtkWidget * vbox,
					  gchar * pch_hbar_name, gdouble d_percent,
					  gchar * pch_text);

static gint gapc_create_notebook_page_overview (GtkWidget * notebook, PGAPC_CONFIG pcfg);
static gint gapc_create_notebook_page_information (GtkWidget * notebook,
						   PGAPC_CONFIG pcfg);
static gint gapc_create_notebook_page_text_report (GtkWidget * notebook,
						   PGAPC_CONFIG pcfg, gchar * pchTitle,
						   gchar * pchTab, gchar * pchKey);
static gint gapc_create_notebook_page_configuration (GtkWidget * notebook,
						     PGAPC_CONFIG pcfg);
static gint gapc_create_notebook_page_about (GtkWidget * notebook, PGAPC_CONFIG pcfg);

static gint gapc_config_get_values (PGAPC_CONFIG pcfg);
static gint gapc_config_save_values (PGAPC_CONFIG pcfg);
static gint gapc_config_load_values (PGAPC_CONFIG pcfg);

static void gapc_app_log_error (gchar * pch_func, gchar * pch_topic, gchar * pch_emsg);
static gboolean cb_application_message (gpointer pch);

static gboolean cb_network_message (gpointer pch);
static void gapc_net_log_error (gchar * pch_func, gchar * pch_topic,
				GnomeVFSResult result);
static gint gapc_net_read_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes);
static gint gapc_net_write_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes);
static gint gapc_net_recv (GnomeVFSSocket * psocket, gchar * buff, gint maxlen);
static gint gapc_net_send (GnomeVFSSocket * v_socket, gchar * buff, gint len);
static GnomeVFSInetConnection *gapc_net_open (gchar * pch_host, gint i_port);
static void gapc_net_close (GnomeVFSInetConnection * connection,
			    GnomeVFSSocket * v_socket);
static gint gapc_net_transaction_service (PGAPC_CONFIG pcfg, gchar * cp_cmd,
					  gchar ** pch);


/*
 * save the contents of configuration values to filesystem.
 * creating a new file if needed.
 * return FALSE on error
 *        TRUE  on sucess
 */
static gint
gapc_config_save_values (PGAPC_CONFIG pcfg)
{
  GIOChannel *gioc = NULL;
  GIOStatus gresult = 0;
  GError *gerror = NULL;
  GString *gs_config_values = NULL;


  g_return_val_if_fail (pcfg, FALSE);	/* error exit */
  g_return_val_if_fail (pcfg->pch_key_filename, FALSE);	/* error exit */

  gs_config_values = g_string_new (NULL);
  g_string_append_printf (gs_config_values, "%s\n%d\n", pcfg->pch_host, pcfg->i_port);

  gioc = g_io_channel_new_file (pcfg->pch_key_filename, "w", &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_save_values", "open config file failed",
			  gerror->message);
      g_error_free (gerror);
      gerror = NULL;

      return FALSE;
    }

  gresult =
    g_io_channel_write_chars (gioc, gs_config_values->str, gs_config_values->len, NULL,
			      &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_save_values",
			  "GIO write chars failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;

      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      g_string_free (gs_config_values, TRUE);
      return FALSE;
    }

  gresult = g_io_channel_shutdown (gioc, TRUE, &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_save_values",
			  "GIO channel shutdown failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }

  g_io_channel_unref (gioc);

  g_string_free (gs_config_values, TRUE);

  return TRUE;
}

/*
 * Loads the contents of config file into configuration struct.
 * return FALSE on defaults being used.
 *        TRUE  config file values used.
 */
static gint
gapc_config_load_values (PGAPC_CONFIG pcfg)
{
  GIOChannel *gioc = NULL;
  GIOStatus gresult = 0;
  GError *gerror = NULL;
  gsize gLen = 0, gPos = 0;
  gchar *pch_host = NULL, *pch_port = NULL;

  g_return_val_if_fail (pcfg, FALSE);	/* error exit */
  g_return_val_if_fail (pcfg->pch_key_filename, FALSE);	/* error exit */


  gioc = g_io_channel_new_file (pcfg->pch_key_filename, "r", &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_load_values", "open config file failed",
			  gerror->message);
      g_error_free (gerror);
      gerror = NULL;
      if (pcfg->pch_host != NULL)
           g_free (pcfg->pch_host);
      pcfg->pch_host = g_strdup ("localhost");
      pcfg->i_port = 3551;

      return FALSE;
    }

  gresult = g_io_channel_read_line (gioc, &pch_host, &gLen, &gPos, &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_load_values",
			  "read hostname failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;

      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      if (pcfg->pch_host != NULL)
          g_free (pcfg->pch_host);
      pcfg->pch_host = g_strdup ("localhost");
      pcfg->i_port = 3551;
      return FALSE;
    }
  pch_host[gPos] = 0;
  if (pcfg->pch_host != NULL)
    g_free (pcfg->pch_host);

  pcfg->pch_host = pch_host;

  gresult = g_io_channel_read_line (gioc, &pch_port, &gLen, &gPos, &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_load_values",
			  "read port number failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;

      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      pcfg->i_port = 3551;
      return FALSE;
    }

  pcfg->i_port = (gint) g_strtod (pch_port, NULL);

  gresult = g_io_channel_shutdown (gioc, TRUE, &gerror);
  if (gerror != NULL)
    {
      gapc_app_log_error ("gapc_config_load_values",
			  "GIO channel shutdown failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }

  g_io_channel_unref (gioc);


  return TRUE;
}

/*
 * Collects either the save config values or the defaults.  Writes a
 * new configuration file if defaults are used.
 * return FALSE on defaults being used.
 *        TRUE  config file values used.
 * push host/port values back to caller
 */
static gint
gapc_config_get_values (PGAPC_CONFIG pcfg)
{
  GString *gs_filename = NULL;
  gint i_create_file = 0;


  g_return_val_if_fail (pcfg, FALSE);	/* error exit */

  if (pcfg->pch_key_filename != NULL)	
    g_free (pcfg->pch_key_filename);

  gs_filename = g_string_new (GAPC_CONFIG_FILE);
  gs_filename = g_string_prepend (gs_filename, g_get_home_dir ());
  pcfg->pch_key_filename = g_strdup (gs_filename->str);
  g_string_free (gs_filename, TRUE);


  i_create_file = gapc_config_load_values (pcfg);

  if (i_create_file != TRUE)
    gapc_config_save_values (pcfg);

  return i_create_file;
}

/*
 * Handle the recording of application error messages
 * return FALSE to terminate timer that called us.
 */
static gboolean
cb_application_message (gpointer pch)
{

  g_return_val_if_fail (pch, FALSE);	/* error exit */

  g_message (pch);
  g_free (pch);

  return FALSE;
}

/*
 * Handle the recording of network error messages
 * return FALSE to terminate timer that called us.
 */
static gboolean
cb_network_message (gpointer pch)
{

  g_return_val_if_fail (pch, FALSE);	/* error exit */

  g_warning (pch);
  g_free (pch);

  return FALSE;
}

/* 
 *  capture the current application related error values 
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this may be a background thread)
 */
static void
gapc_app_log_error (gchar * pch_func, gchar * pch_topic, gchar * pch_emsg)
{
  gchar *pch = NULL;

  pch = g_strdup_printf ("%s(%s) emsg=%s", pch_func, pch_topic, pch_emsg);

  g_timeout_add (100, cb_application_message, pch);

  return;
}

/* 
 *  capture the current network related error values 
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this is a background thread)
 */
static void
gapc_net_log_error (gchar * pch_func, gchar * pch_topic, GnomeVFSResult result)
{
  gchar *pch = NULL;

  pch = g_strdup_printf ("%s(%s) emsg=%s",
			 pch_func, pch_topic, gnome_vfs_result_to_string (result));

  g_timeout_add (100, cb_network_message, pch);

  return;
}

/*
 * Read nbytes from the network. It is possible that the 
 * total bytes requires several read requests.
 * returns -1 on error, or number of bytes read
 */
static gint
gapc_net_read_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes)
{
  GnomeVFSResult result = GNOME_VFS_OK;
  GnomeVFSFileSize nread = 0, nleft = 0, nread_total = 0;

  g_return_val_if_fail (ptr, -1);

  nleft = nbytes;

  while (nleft > 0)
    {
      result = gnome_vfs_socket_read (psocket, ptr, nleft, &nread, NULL);

      if (result != GNOME_VFS_OK)
	{
	  gapc_net_log_error ("read_nbytes", "read from network failed", result);
	  return (-1);		/* error, or EOF */
	}

      nread_total += nread;
      nleft -= nread;
      ptr += nread;
    }

  return (nread_total);		/* return >= 0 */
}

/*
 * Write nbytes to the network.  It may require several writes.
 * returns -1 on error, or number of bytes written
 */
static gint
gapc_net_write_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes)
{
  GnomeVFSFileSize nwritten = 0, nwritten_total = 0;
  GnomeVFSResult result = GNOME_VFS_OK;
  gint nleft = 0;

  g_return_val_if_fail (ptr, -1);

  nleft = nbytes;
  while (nleft > 0)
    {
      result = gnome_vfs_socket_write (psocket, ptr, nleft, &nwritten, NULL);

      if (result != GNOME_VFS_OK)
	{
	  gapc_net_log_error ("write_nbytes", "write to network failed", result);
	  return (-1);		/* error */
	}

      nwritten_total += nwritten;
      nleft -= nwritten;
      ptr += nwritten;
    }

  return (nwritten_total);
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
static gint
gapc_net_recv (GnomeVFSSocket * psocket, gchar * buff, gint maxlen)
{
  gint nbytes = 0;
  gshort pktsiz = 0;

  g_return_val_if_fail (buff, -1);

  /* get data size -- in short */
  nbytes = gapc_net_read_nbytes (psocket, (gchar *) & pktsiz, sizeof (gshort));
  if (nbytes < 0)
    return -1;			/* assume hard EOF received */

  if (nbytes != sizeof (gshort))
    return -2;

  pktsiz = g_ntohs (pktsiz);	/* decode no. of bytes that follow */
  if (pktsiz > maxlen)
    return -2;

  if (pktsiz == 0)
    return 0;			/* soft EOF */

  /* now read the actual data */
  nbytes = gapc_net_read_nbytes (psocket, buff, pktsiz);
  if (nbytes < 0)
    return -2;

  if (nbytes != pktsiz)
    return -2;

  return (nbytes);		/* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
static gint
gapc_net_send (GnomeVFSSocket * v_socket, gchar * buff, gint len)
{
  gint rc = 0;
  gshort pktsiz = 0;

  g_return_val_if_fail (buff, -1);

  /* send short containing size of data packet */
  pktsiz = g_htons ((gshort) len);
  rc = gapc_net_write_nbytes (v_socket, (gchar *) & pktsiz, sizeof (gshort));
  if (rc != sizeof (gshort))
    return -1;

  /* send data packet */
  rc = gapc_net_write_nbytes (v_socket, buff, len);
  if (rc != len)
    return -1;

  return rc;
}

/*     
 * Open a TCP connection to the UPS network server or host:port
 * Returns -1 on error
 * Returns connection id value
 */
static GnomeVFSInetConnection *
gapc_net_open (gchar * pch_host, gint i_port)
{
  GnomeVFSResult result = GNOME_VFS_OK;
  GnomeVFSInetConnection *connection = NULL;

  g_return_val_if_fail (pch_host, NULL);

  result = gnome_vfs_inet_connection_create (&connection, pch_host, i_port, NULL);
  if (result != GNOME_VFS_OK)
    {
      gapc_net_log_error ("net_open", "create inet connection failed", result);
      return NULL;
    }

  return connection;
}

/* Close the network connection */
static void
gapc_net_close (GnomeVFSInetConnection * connection, GnomeVFSSocket * v_socket)
{
  gshort pktsiz = 0;

  /* send EOF sentinel */
  gapc_net_write_nbytes (v_socket, (gchar *) & pktsiz, sizeof (gshort));
  gnome_vfs_inet_connection_destroy (connection, NULL);

  return;
}


/*
 * performs a complete NIS transaction by sending cmd and 
 * loading each result line into the pch array.
 * also, refreshes status key/value pairs in hastable.
 * return error = -1,  or number of lines read from network
 */
static gint
gapc_net_transaction_service (PGAPC_CONFIG pcfg, gchar * cp_cmd, gchar ** pch)
{
  gint n = 0, iflag = 0, i_port = 0;
  gchar recvline[GAPC_MAX_TEXT];
  GnomeVFSInetConnection *v_connection = NULL;
  GnomeVFSSocket *v_socket = NULL;

  g_return_val_if_fail (pcfg, -1);
  g_return_val_if_fail (pcfg->pch_host, -1);

  i_port = pcfg->i_port;
  v_connection = gapc_net_open (pcfg->pch_host, i_port);
  if (v_connection == NULL)
    return -1;

  v_socket = gnome_vfs_inet_connection_to_socket (v_connection);
  if (v_socket == NULL)
    {
      gapc_net_log_error ("transaction_service", "connect to socket for io failed",
			  GNOME_VFS_OK);
      gnome_vfs_inet_connection_destroy (v_connection, NULL);
      return -1;
    }

  n = gapc_net_send (v_socket, cp_cmd, g_utf8_strlen (cp_cmd, -1));
  if (n < 0)
    {
      gapc_net_close (v_connection, v_socket);
      return -1;
    }

  /* clear current data */
  for (iflag = 0; iflag < GAPC_MAX_ARRAY - 1; iflag++)
    {
      g_free (pch[iflag]);
      pch[iflag] = NULL;
    }

  iflag = 0;
  while ((n = gapc_net_recv (v_socket, recvline, sizeof (recvline))) > 0)
    {
      recvline[n] = 0;
      pch[iflag++] = g_strdup (recvline);

      if (g_str_equal (cp_cmd, "status") && iflag > 1)
	gapc_update_hashtable (pcfg, recvline);

      if (iflag > (GAPC_MAX_ARRAY - 2))
	break;
    }

  gapc_net_close (v_connection, v_socket);

  return iflag;			/* count of records received */
}

/*
 * parses received line of text into key/value pairs to be inserted
 * into the status hashtable.
 */
static gint
gapc_update_hashtable (PGAPC_CONFIG pcfg, gchar * pch_unparsed)
{
  gchar *pch_in = NULL;
  gchar *pch = NULL;
  gchar *pch_end = NULL;
  gint ilen = 0;


  g_return_val_if_fail (pch_unparsed, -1);

  /* unparsed contains - keystring : keyvalue nl */
  pch_in = g_strdup (pch_unparsed);
  pch_end = g_strrstr (pch_in, "\n");
  if (pch_end != NULL)
    *pch_end = 0;

  ilen = g_utf8_strlen (pch_in, -1);

  pch = g_strstr_len (pch_in, ilen, ":");
  *pch = 0;
  pch_in = g_strchomp (pch_in);
  pch++;
  pch = g_strstrip (pch);

  g_hash_table_replace (pcfg->pht_Status, g_strdup (pch_in), g_strdup (pch));

  g_free (pch_in);

  return ilen;
}

/* ***************************************************************************
 *  Implements a Horizontal Bar Chart...
 *  - data value has a range of 0.0 to 1.0 for 0-100% display
 *  - in chart text is limited to about 30 chars
 */
static gboolean
cb_h_bar_chart_exposed (GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
  PGAPC_BAR_H pbar = data;
  gint i_percent = 0;
  PangoLayout *playout = NULL;

  pbar->rect.x = 0;
  pbar->rect.y = 0;
  pbar->rect.width = widget->allocation.width;
  pbar->rect.height = widget->allocation.height;

  /* scale up the less than zero data value */
  i_percent =
    (gint) ((gdouble) (widget->allocation.width / 100.0) *
	    (gdouble) (pbar->d_value * 100.0));

  /* the frame of the chart */
  gtk_paint_box (widget->style, widget->window, GTK_WIDGET_STATE (widget),	// GTK_STATE_PRELIGHT, 
		 GTK_SHADOW_ETCHED_IN,
		 &pbar->rect,
		 widget,
		 "gapc_hbar_frame",
		 0, 0, widget->allocation.width - 1, widget->allocation.height - 1);

  /* the scaled value */
  gtk_paint_box (widget->style, widget->window, GTK_STATE_SELECTED,	// GTK_STATE_PRELIGHT, 
		 GTK_SHADOW_OUT,
		 &pbar->rect,
		 widget,
		 "gapc_hbar_value", 1, 1, i_percent, widget->allocation.height - 4);

  if (pbar->c_text[0])
    {
      gint x = 0, y = 0;

      playout = gtk_widget_create_pango_layout (widget, pbar->c_text);
      pango_layout_set_markup (playout, pbar->c_text, -1);

      pango_layout_get_pixel_size (playout, &x, &y);
      x = (widget->allocation.width - x) / 2;
      y = (widget->allocation.height - y) / 2;

      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			TRUE,
			&pbar->rect,
			widget,
			"gapc_hbar_text", (pbar->b_center_text) ? x : 6, y, playout);

      g_object_unref (playout);
    }


  return TRUE;
}

/* 
 * callback for QUIT push button on main window
 */
static void
cb_button_quit (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  /* Stop the thread  and Refresh Timer */
  pcfg->b_run = FALSE;

  gdk_flush ();

  gtk_main_quit ();
}

/* 
 * callback for APPLY push button on config page
 */
static void
cb_config_apply (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  gchar *pch = NULL;
  GtkEntry *ef = NULL;


  /* get current value of entry-fields */
  ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerAddress");
  if (pcfg->pch_host != NULL)
    g_free (pcfg->pch_host);

  pch = (gchar *) gtk_entry_get_text (ef);
  pcfg->pch_host = g_strdup (pch);


  ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerPort");
  pch = (gchar *) gtk_entry_get_text (ef);
  pcfg->i_port = (gint) g_strtod (pch, NULL);

  return;
}

/* 
 * callback for SAVE push button on config page
 */
static void
cb_config_save (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);

  cb_config_apply (button, gp);	/* use existing code */

  if (gapc_config_save_values (pcfg))
    gtk_statusbar_push (GTK_STATUSBAR (w),
			pcfg->i_info_context, "Configuration Saved...");
  return;
}

/* 
 * callback for REFRESH push button on main page
 */
static void
cb_refresh_button (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  if (!pcfg->b_data_available)
    return;

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);
  if (!gapc_monitor_update (pcfg))
    gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			"Refresh Button Skipped...  Thread is busy");

  return;
}

/* 
 * timer service routine for IPL refresh.  used to overcome
 * the multi-threaded startup delay.  very short
 */
static gboolean
cb_first_refresh_timer (gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  if (!pcfg->b_data_available)
    return TRUE;

  gdk_flush ();
  gdk_threads_enter ();
  if (!gapc_monitor_update (pcfg))
    {
      w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
      gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);
      gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			  "Initial Update Not Ready... Please Standby!");
      gdk_threads_leave ();
      return TRUE;		/* try again */
    }

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);
  gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
		      "Initial Update Completed...");

  gdk_threads_leave ();
  return FALSE;			/* this will terminate the timer */
}

/* 
 * timer service routine for normal data display refreshs
 */
static gboolean
cb_refresh_timer (gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  if (!pcfg->b_run)
    return FALSE;

  if (!pcfg->b_data_available)
    return TRUE;

  gdk_flush ();
  gdk_threads_enter ();
  if (!gapc_monitor_update (pcfg))
    {
      w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
      gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);
      gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			  "Automatic Update Skipped...  Thread is busy");
      gdk_threads_leave ();
      return TRUE;
    }

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);

  gdk_threads_leave ();
  return TRUE;
}

static gint
gapc_text_view_clear_buffer (GtkWidget * view)
{
  GtkTextIter start, end;
  GtkTextBuffer *buffer = NULL;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_delete (buffer, &start, &end);

  return FALSE;
}

static void
gapc_text_view_prepend (GtkWidget * view, gchar * s)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_start_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, s, -1);
}

static void
gapc_text_view_append (GtkWidget * view, gchar * s)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, s, -1);
}


static GtkWidget *
gapc_create_scrolled_text_view (GtkWidget * box)
{
  PangoFontDescription *font_desc;
  GtkWidget *scrolled, *view;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (box), scrolled);

  view = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled), view);

  /* Change default font throughout the widget */
  font_desc = pango_font_description_from_string ("Monospace 9");
  gtk_widget_modify_font (view, font_desc);
  pango_font_description_free (font_desc);

  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 5);

  return view;
}

/*
 * returns TRUE if helps was offered, else FALSE if input was all ok.
 */
static int
gapc_parse_args (gint argc, gchar ** argv, PGAPC_CONFIG pcfg)
{
  gchar *pch_host = NULL;
  gchar *pch_port = NULL;
  gchar *pch = NULL;
  gboolean b_syntax = FALSE;
  GString *gs_parm1 = NULL, *gs_parm2 = NULL;


  gs_parm1 = g_string_new (GAPC_GROUP_KEY);
  gs_parm2 = g_string_new (GAPC_GROUP_KEY);

  /* *********************************************************** *
   *  Get user input
   *   - default to known values
   *   - check config file for saved values -- careful not to override cmdline
   */
  while (--argc > 0)		/* ADJUST COUNTER HERE */
    {
      g_string_assign (gs_parm1, argv[argc]);
      if (argv[argc + 1] != NULL)
	g_string_assign (gs_parm2, argv[argc + 1]);

      pch = g_strstr_len (gs_parm1->str, 6, "-host");
      if (pch != NULL)
	{
	  if (gs_parm2->len > 2)
	    {
	      pch_host = g_strdup_printf ("%s", gs_parm2->str);
	      continue;
	    }
	  g_print ("\ngapcmon: Error :>Invalid host name or address !\n\n");
	  b_syntax = TRUE;
	}

      pch = g_strstr_len (gs_parm1->str, 6, "-port");
      if (pch != NULL)
	{
	  if (gs_parm2->len > 2)
	    {
	      pch_port = g_strdup_printf ("%s", gs_parm2->str);
	      continue;
	    }
	  g_print ("\ngapcmon: Error :>Invalid port number !\n\n");
	  b_syntax = TRUE;
	}

      pch = g_strstr_len (gs_parm1->str, 6, "-help");
      if ((pch != NULL) || b_syntax)
	{
	  g_print
	    ("\nsyntax: gapcmon [--help] [--host server.mynetwork.net|127.0.0.1] [--port 9999]\n"
	     "where:  --help, this message\n"
	     "        --host, the domain name or ip address of the server\n"
	     "        --port, the port number of the apcupsd nis service\n"
	     "        null, defaults to localhost and port 3551\n\n"
	     "Skoona@Users.SourceForge.Net (GPL) 2006 \n\n");

	  g_string_free (gs_parm1, TRUE);
	  g_string_free (gs_parm2, TRUE);
	  return TRUE;		/* trigger exit */
	}
    }

  /* *********************************************************
   * Apply defaults
   */
  gapc_config_get_values (pcfg);

  if (pch_host != NULL)		/* if cmdline value is present */
    {
      g_free (pcfg->pch_host);
      pcfg->pch_host = pch_host;
    }
  if (pch_port != NULL)
    {
      pcfg->i_port = (gint) g_strtod (pch_port, NULL);
      g_free (pch_port);
    }

  g_string_free (gs_parm1, TRUE);
  g_string_free (gs_parm2, TRUE);

  return FALSE;
}

/*
 * Worker thread for network communications.
 */
static gpointer *
gapc_network_thread (PGAPC_CONFIG pcfg)
{
  gint s = 0, e = 0, x = 0;

  while (pcfg->b_run)
    {
      g_mutex_lock (pcfg->gm_update);

      if (pcfg->b_run)
	s = gapc_net_transaction_service (pcfg, "status", pcfg->pach_status);

      if (pcfg->b_run)
	e = gapc_net_transaction_service (pcfg, "events", pcfg->pach_events);

      g_mutex_unlock (pcfg->gm_update);

    x = s + e;
    if ( x > 0 )
      pcfg->b_data_available = TRUE;
    else
      pcfg->b_data_available = FALSE;      


      if (pcfg->b_run)
	g_usleep (GAPC_THREAD_CYCLE / 2);
      if (pcfg->b_run)
	g_usleep (GAPC_THREAD_CYCLE / 2);
    }

  g_thread_exit (pcfg);

  return NULL;
}

/*
 * main data updating routine.
 * -- collects and pushes data to all ui
 */
static gint
gapc_monitor_update (PGAPC_CONFIG pcfg)
{
  gint i_x = 0;
  GtkWidget *win = NULL, *w = NULL;
  gchar *pch = NULL, *pch1 = NULL, *pch2 = NULL,
    *pch3 = NULL, *pch4 = NULL, *pch5 = NULL, *pch6 = NULL;
  gdouble dValue = 0.00, dScale = 0.0, dtmp = 0.0;
  gchar ch_buffer[256];
  PGAPC_BAR_H pbar = NULL;

  if (!pcfg->b_data_available)
    return  FALSE;

  if (!g_mutex_trylock (pcfg->gm_update))
    return FALSE;		/* thread must be busy */


  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusPage");
  gapc_text_view_clear_buffer (GTK_WIDGET (w));
  for (i_x = 1; pcfg->pach_status[i_x] != NULL; i_x++)
    gapc_text_view_append (GTK_WIDGET (w), pcfg->pach_status[i_x]);

  w = g_hash_table_lookup (pcfg->pht_Widgets, "EventsPage");
  gapc_text_view_clear_buffer (GTK_WIDGET (w));
  for (i_x = 0; pcfg->pach_events[i_x] != NULL; i_x++)
    gapc_text_view_prepend (GTK_WIDGET (w), pcfg->pach_events[i_x]);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "UtilityAC");
  pch = g_hash_table_lookup (pcfg->pht_Status, "LINEV");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "HITRANS");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dScale = g_strtod (pch1, NULL);
  dValue /= dScale;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar1");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s from Utility", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar1-Widget");
  gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryVoltage");
  pch = g_hash_table_lookup (pcfg->pht_Status, "BATTV");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "NOMBATTV");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dScale = g_strtod (pch1, NULL);
  dScale *= 1.2;
  dValue /= dScale;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar2");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s DC on Battery", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar2-Widget");
  gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryCharge");
  pch = g_hash_table_lookup (pcfg->pht_Status, "BCHARGE");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dValue /= 100.0;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar3");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s Battery Charge", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar3-Widget");
  gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);


  win = g_hash_table_lookup (pcfg->pht_Widgets, "UPSLoad");
  pch = g_hash_table_lookup (pcfg->pht_Status, "LOADPCT");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dtmp = dValue /= 100.0;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar4");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text),
	      "<span foreground=\"yellow\" style=\"italic\" >%s</span>", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar4-Widget");
  gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);


  win = g_hash_table_lookup (pcfg->pht_Widgets, "TimeRemaining");
  pch = g_hash_table_lookup (pcfg->pht_Status, "TIMELEFT");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dScale = dValue / (1 - dtmp);
  dValue /= dScale;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar5");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s Remaining", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar5-Widget");
  gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);


  win = g_hash_table_lookup (pcfg->pht_Widgets, "TitleStatus");
  pch = g_hash_table_lookup (pcfg->pht_Status, "STATUS");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "UPSNAME");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "HOSTNAME");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\" size=\"large\">"
	      "Monitored UPS %s on %s is %s" "</span>", pch1, pch2, pch);
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "SoftwareInformation");
  pch = g_hash_table_lookup (pcfg->pht_Status, "VERSION");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "CABLE");
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "UPSMODE");
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "STARTTIME");
  pch5 = g_hash_table_lookup (pcfg->pht_Status, "STATUS");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\">"
	      "%s\n%s\n%s\n%s\n%s\n%s" "</span>", pch, pch1, pch2, pch3, pch4, pch5);
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "PerformanceSummary");
  pch = g_hash_table_lookup (pcfg->pht_Status, "SELFTEST");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "NUMXFERS");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "LASTXFER");
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "XONBATT");
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "XOFFBATT");
  pch5 = g_hash_table_lookup (pcfg->pht_Status, "TONBATT");
  pch6 = g_hash_table_lookup (pcfg->pht_Status, "CUMONBATT");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\">"
	      "%s\n%s\n%s\n%s\n%s\n%s\n%s"
	      "</span>",
	      pch, pch1, pch2,
	      (pch3 != NULL) ? pch3 : "N/A", (pch4 != NULL) ? pch4 : "N/A", pch5, pch6);
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "ProductInformation");
  pch = g_hash_table_lookup (pcfg->pht_Status, "APCMODEL");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "SERIALNO");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "MANDATE");
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "FIRMWARE");
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "BATTDATE");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\">"
	      "%s\n%s\n%s\n%s\n%s" "</span>", pch, pch1, pch2, pch3, pch4);
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  g_mutex_unlock (pcfg->gm_update);

  return TRUE;
}

/*
 * creates horizontal bar chart and allocates control data
 * requires cb_h_bar_chart_exposed() routine
 * return drawing area widget
 */
static GtkWidget *
gapc_create_h_barchart (PGAPC_CONFIG pcfg, GtkWidget * vbox,
			gchar * pch_hbar_name, gdouble d_percent, gchar * pch_text)
{
  PGAPC_BAR_H pbar = NULL;
  GtkWidget *drawing_area = NULL;
  gchar *pch = NULL;

  pbar = g_new0 (GAPC_BAR_H, 1);
  pbar->d_value = d_percent;
  pbar->b_center_text = FALSE;
  g_strlcpy (pbar->c_text, pch_text, sizeof (pbar->c_text));

  drawing_area = gtk_drawing_area_new ();	/* manual bargraph */
  gtk_widget_set_size_request (drawing_area, 100, 20);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (cb_h_bar_chart_exposed), (gpointer) pbar);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
  g_hash_table_insert (pcfg->pht_Status, g_strdup (pch_hbar_name), pbar);
  pch = g_strdup_printf ("%s-Widget", pch_hbar_name);
  g_hash_table_insert (pcfg->pht_Widgets, pch, drawing_area);

  return drawing_area;
}


static gint
gapc_create_notebook_page_overview (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  GtkWidget *frame, *label, *pbox, *lbox, *rbox, *pbar;
  GtkSizeGroup *gSize;
  gint i_page = 0;

  /* Create the Overview Notebook Page */
  frame = gtk_frame_new ("UPS Status Overview");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  label = gtk_label_new ("Overview");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);

  lbox = gtk_vbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (pbox), lbox);
  rbox = gtk_vbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (pbox), rbox);
  gtk_container_add (GTK_CONTAINER (pbox), gtk_vbox_new (TRUE, 2));

  gSize = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  label = gtk_label_new ("Utility AC Voltage:");
  gtk_size_group_add_widget (gSize, label);
  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), pbar, TRUE, TRUE, 4);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 0.87);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("UtilityAC"), pbar);

  label = gtk_label_new ("Battery Voltage:");
  gtk_size_group_add_widget (gSize, label);
  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), pbar, TRUE, TRUE, 4);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("BatteryVoltage"), pbar);

  label = gtk_label_new ("Battery Charge:");
  gtk_size_group_add_widget (gSize, label);
  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), pbar, TRUE, TRUE, 4);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("BatteryCharge"), pbar);

  label = gtk_label_new ("UPS Load:");
  gtk_size_group_add_widget (gSize, label);
  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), pbar, TRUE, TRUE, 4);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 0.57);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("UPSLoad"), pbar);

  label = gtk_label_new ("Time Remaining:");
  gtk_size_group_add_widget (gSize, label);
  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), pbar, TRUE, TRUE, 4);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 1.0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("TimeRemaining"), pbar);

  return i_page;
}

static gint
gapc_create_notebook_page_information (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  GtkWidget *frame, *label, *pbox, *lbox, *rbox, *table3, *gbox;
  gint i_page = 0;

  /* Create a Notebook Page */
  table3 = gtk_table_new (4, 4, FALSE);
  label = gtk_label_new ("Information");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table3, label);

  frame = gtk_frame_new ("<b><i>Software Information</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_table_attach (GTK_TABLE (table3), frame,
		    /* X direction *//* Y direction */
		    0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, TRUE, TRUE, 0);
  rbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("APCUPSD version\n"
			 "Monitored UPS name\n"
			 "Cable Driver type\n"
			 "Configuration mode\n" "Last started\n" "UPS State");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Data Only..");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("SoftwareInformation"), label);

  frame = gtk_frame_new ("<b><i>UPS Metrics</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_table_attach (GTK_TABLE (table3), frame,
		    /* X direction *//* Y direction */
		    2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);
  gbox = gtk_vbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (frame), gbox);

  gapc_create_h_barchart (pcfg, gbox, "HBar1", 62.8, "Value of 62.8");
  gapc_create_h_barchart (pcfg, gbox, "HBar2", 14.8, "Value of 14.8");
  gapc_create_h_barchart (pcfg, gbox, "HBar3", 55.8, "Value of 55.8");
  gapc_create_h_barchart (pcfg, gbox, "HBar4", 72.8, "Value of 72.8");
  gapc_create_h_barchart (pcfg, gbox, "HBar5", 92.8, "Value of 92.8");

  frame = gtk_frame_new ("<b><i>Performance Summary</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_table_attach (GTK_TABLE (table3), frame,
		    /* X direction *//* Y direction */
		    0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, TRUE, TRUE, 0);
  rbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("Selftest running\n"
			 "Number of transfers\n"
			 "Reason last transfer\n"
			 "Last transfer to battery\n"
			 "Last transfer off battery\n"
			 "Time on battery\n" "Cummulative on battery");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Data Only..");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("PerformanceSummary"), label);

  frame = gtk_frame_new ("<b><i>Product Information</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_table_attach (GTK_TABLE (table3), frame,
		    /* X direction *//* Y direction */
		    2, 3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, TRUE, TRUE, 0);
  rbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("Device\n" "Serial\n" "Manf date\n" "Firmware\n" "Batt date");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Data Only..");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("ProductInformation"), label);

  return i_page;
}

static gint
gapc_create_notebook_page_text_report (GtkWidget * notebook, PGAPC_CONFIG pcfg,
				       gchar * pchTitle, gchar * pchTab, gchar * pchKey)
{
  GtkWidget *frame, *label, *text;
  gint i_page = 0;

  frame = gtk_frame_new (pchTitle);
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  label = gtk_label_new (pchTab);
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
  text = gapc_create_scrolled_text_view (frame);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup (pchKey), text);

  return i_page;
}

static gint
gapc_create_notebook_page_configuration (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  gint i_page = 0;
  GtkWidget *frame, *label, *table2, *entry, *button;
  gchar ch_buffer[16];

  /* Create Configuration Notebook Page */
  frame = gtk_frame_new ("Configuration Options");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  label = gtk_label_new ("Config");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

  table2 = gtk_table_new (4, 4, FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table2);

  label = gtk_label_new ("Server hostname or ip address:");
  entry = gtk_entry_new_with_max_length (255);
  gtk_table_attach (GTK_TABLE (table2), label,
		    /* X direction *//* Y direction */
		    0, 1, 0, 1, GTK_FILL, 0, 0, 5);
  gtk_table_attach (GTK_TABLE (table2), entry,
		    /* X direction *//* Y direction */
		    1, 3, 0, 1, GTK_EXPAND | GTK_FILL, 0, 2, 5);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_entry_set_text (GTK_ENTRY (entry), pcfg->pch_host);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("ServerAddress"), entry);


  label = gtk_label_new ("Server port number:");
  gtk_table_attach (GTK_TABLE (table2), label,
		    /* X direction *//* Y direction */
		    0, 1, 1, 2, GTK_FILL, 0, 0, 5);
  entry = gtk_entry_new_with_max_length (10);
  gtk_table_attach (GTK_TABLE (table2), entry,
		    /* X direction *//* Y direction */
		    1, 2, 1, 2, GTK_FILL, 0, 2, 5);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  gtk_entry_set_text (GTK_ENTRY (entry),
		      g_ascii_dtostr (ch_buffer,
				      sizeof (ch_buffer), (gdouble) pcfg->i_port));
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("ServerPort"), entry);

  button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  gtk_table_attach (GTK_TABLE (table2), button,
		    /* X direction *//* Y direction */
		    1, 2, 3, 4, GTK_FILL, 0, 5, 5);
  g_signal_connect (button, "clicked", G_CALLBACK (cb_config_apply), pcfg);
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_table_attach (GTK_TABLE (table2), button,
		    /* X direction *//* Y direction */
		    2, 3, 3, 4, GTK_FILL, 0, 5, 5);
  g_signal_connect (button, "clicked", G_CALLBACK (cb_config_save), pcfg);

  return i_page;
}

static gint
gapc_create_notebook_page_about (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  gint i_page = 0;
  GtkWidget *vbox, *label;
  gchar *about_text = NULL;

  /* Create About page */
  vbox = gtk_vbox_new (FALSE, 4);
  label = gtk_label_new ("About");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  about_text = g_strdup_printf ("<b><big>GAPCMON Version %s</big></b>\n"
				"<b>gapcmon - gui ups monitor for APCUPSD.sourceforge.net package</b>\n"
				"<i>http://gapcmon.sourceforge.net/</i>\n\n"
				"Copyright (C) 2006 James Scott, Jr.\n"
				"skoona@users.sourceforge.net\n\n"
				"Released under the GNU Public License\n"
				"GAPCMON comes with ABSOLUTELY NO WARRANTY\n",
				GAPC_VERSION);

  label = gtk_label_new (about_text);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  g_free (about_text);

  return i_page;
}


static GtkWidget *
gapc_create_user_interface (PGAPC_CONFIG pcfg)
{
  GtkWidget *wStatus_bar;
  GtkWidget *window, *notebook, *label, *button, *dbutton, *table;
  GdkScreen *screen = NULL;
  gint i_x = 0;
  gint i_y = 0, i_page = 0;

  /* *********************************************************
   * Create main window and display
   */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);
  gtk_window_set_title (GTK_WINDOW (window), "gapcmon UPS Monitor");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* *********************************************************
   * Calculate window size based on current desktop sizes
   */
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  i_x = gdk_screen_get_width (screen);
  i_x = (gint) ((gfloat) i_x * 0.48);
  i_y = gdk_screen_get_height (screen);
  i_y = (gint) ((gfloat) i_y * 0.40);
  gtk_widget_set_size_request (window, i_x / 2, i_y);	// set minimum size
  gtk_window_set_default_size (GTK_WINDOW (window), i_x, i_y);	// set inital size
  gtk_window_set_default_icon_from_file
    ("/usr/share/icons/gnome/48x48/devices/gnome-dev-battery.png", NULL);

  table = gtk_table_new (6, 4, FALSE);
  gtk_container_add (GTK_CONTAINER (window), table);

  label = gtk_label_new ("UPS Monitor");
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("TitleStatus"), label);
  gtk_table_attach (GTK_TABLE (table), label,
		    /* X direction *//* Y direction */
		    0, 3, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 5);

  /* Create a new notebook, place the position of the tabs */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_table_attach (GTK_TABLE (table), notebook,
		    /* X direction *//* Y direction */
		    0, 3, 1, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);

  /* Create the Notebook Pages */
  gapc_create_notebook_page_overview (notebook, pcfg);
  i_page = gapc_create_notebook_page_information (notebook, pcfg);
  gapc_create_notebook_page_text_report (notebook, pcfg, "Power Events", "Events",
					 "EventsPage");
  gapc_create_notebook_page_text_report (notebook, pcfg, "Full UPS Status", "Status",
					 "StatusPage");
  gapc_create_notebook_page_configuration (notebook, pcfg);
  gapc_create_notebook_page_about (notebook, pcfg);


  /* Control buttons */
  dbutton = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  g_signal_connect (dbutton, "clicked", G_CALLBACK (cb_refresh_button), pcfg);
  gtk_table_attach (GTK_TABLE (table), dbutton,
		    /* X direction *//* Y direction */
		    0, 1, 3, 4, GTK_FILL, 0, 8, 2);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
  g_signal_connect (button, "clicked", G_CALLBACK (cb_button_quit), pcfg);
  gtk_table_attach (GTK_TABLE (table), button,
		    /* X direction *//* Y direction */
		    2, 3, 3, 4, GTK_FILL, 0, 8, 2);

  wStatus_bar = gtk_statusbar_new ();
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("StatusBar"), wStatus_bar);
  gtk_table_attach (GTK_TABLE (table), wStatus_bar,
		    /* X direction *//* Y direction */
		    0, 3, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);

  pcfg->i_info_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (wStatus_bar),
						       "Informational");

  gtk_widget_show_all (window);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i_page);
  GTK_WIDGET_SET_FLAGS (dbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (dbutton);

  return window;
}


int
main (int argc, char *argv[])
{
  PGAPC_CONFIG pcfg = NULL;
  gint i_x = 0;
  GtkWidget *window;

  /* 
   * Initialize GLib thread support, GnomeVFS, and GTK 
   */
  g_thread_init (NULL);
  gnome_vfs_init ();
  gdk_threads_init ();

  gdk_threads_enter ();
  gtk_init (&argc, &argv);

  pcfg = g_new0 (GAPC_CONFIG, 1);

  pcfg->gm_update = g_mutex_new ();

  if (gapc_parse_args (argc, argv, pcfg))
    return 1;			/* exit if user only wanted help */

  /* 
   * Create hash table for easy access to status info and widgets
   */
  pcfg->pht_Status = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  pcfg->pht_Widgets = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Create an update thread to handle network io */
  pcfg->b_run = TRUE;
  pcfg->tid_refresh =
    g_thread_create ((GThreadFunc) gapc_network_thread, pcfg, FALSE, NULL);

  window = gapc_create_user_interface (pcfg);

  /* Add a timer callback to refresh ups data 1 sec = 1/1000 */
  g_timeout_add (GAPC_REFRESH_INCREMENT, cb_refresh_timer, pcfg);

  g_timeout_add (200, cb_first_refresh_timer, pcfg);	/* one shot timer */

  /* enter the GTK main loop */
  gtk_main ();
  gdk_threads_leave ();

  g_hash_table_destroy (pcfg->pht_Status);
  g_hash_table_destroy (pcfg->pht_Widgets);

  gnome_vfs_shutdown ();

  /* cleanup big memory arrays */
  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_status[i_x])
      g_free (pcfg->pach_status[i_x]);
  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_events[i_x])
      g_free (pcfg->pach_events[i_x]);


  return (0);
}
