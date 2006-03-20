/* gp_apcmon_core.c       serial-0054-7 ****************************************
 * 
 * Information panel routines for the GNOME Panel Monitor Applet
 *              and GTK Monitor program, which works with
 * 				the APCUPSD.sourceforge.package
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

#define _GPANEL_APCMON_H_
#include "gapcmon_common.h"

/*
 * Shared Routines Outgoing
 */
extern gboolean   gapc_load_icons (PGAPC_CONFIG pcfg);
extern gint       gapc_save_preferences (PGAPC_CONFIG pcfg);
extern gint       gapc_load_preferences (PGAPC_CONFIG pcfg);
extern gpointer  *gapc_network_thread (PGAPC_CONFIG pcfg);

extern gboolean   gapc_cb_auto_refresh (gpointer gp);
extern gboolean   gapc_cb_dedicated_one_time_refresh (gpointer gp);
extern void   	  gapc_cb_refresh_button (GtkButton * button, gpointer gp);

extern gint   gapc_create_notebook_page_overview (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_information (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_configuration (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_text_report (GtkWidget * notebook, PGAPC_CONFIG pcfg, gchar * pchTitle, gchar * pchTab, gchar * pchKey);


/*
 * Shared Routines Incoming
 */
extern gboolean   gapc_change_status_icon (PGAPC_CONFIG pcfg);


/*
 * Internal Routines
 */
static void	gapc_log_app_error (gchar * pch_func, gchar * pch_topic, gchar * pch_emsg);
static void gapc_log_net_error (gchar * pch_func, gchar * pch_topic, GnomeVFSResult result);
static gint gapc_monitor_update (PGAPC_CONFIG pcfg);
static gint gapc_update_hashtable (PGAPC_CONFIG pcfg, gchar * pch_unparsed);
static gboolean   gapc_update_tooltip_msg (PGAPC_CONFIG pcfg); 

extern gboolean   gapc_cb_timer_control (gpointer gp);
static gboolean   gapc_cb_application_message (gpointer pch);
static void 	  gapc_cb_button_config_save (GtkButton * button, gpointer gp);
static void 	  gapc_cb_button_config_apply (GtkButton * button, gpointer gp);
static gboolean   gapc_cb_h_bar_chart_exposed (GtkWidget * widget, GdkEventExpose * event, gpointer data);

static gint gapc_text_view_clear_buffer (GtkWidget * view);
static void gapc_text_view_prepend (GtkWidget * view, gchar * s);
static void gapc_text_view_append (GtkWidget * view, gchar * s);

static GtkWidget *gapc_create_scrolled_text_view (GtkWidget * box);
static GtkWidget *gapc_create_h_barchart (PGAPC_CONFIG pcfg, GtkWidget * vbox, gchar * pch_hbar_name, gdouble d_percent, gchar * pch_text);

static gint gapc_net_transaction_service (PGAPC_CONFIG pcfg, gchar * cp_cmd, gchar ** pch);
static GnomeVFSInetConnection *gapc_net_open (gchar * pch_host, gint i_port);
static gint gapc_net_read_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes);
static gint gapc_net_write_nbytes (GnomeVFSSocket * psocket, gchar * ptr, gint nbytes);
static gint gapc_net_recv (GnomeVFSSocket * psocket, gchar * buff, gint maxlen);
static gint gapc_net_send (GnomeVFSSocket * v_socket, gchar * buff, gint len);
static void gapc_net_close (GnomeVFSInetConnection * connection, GnomeVFSSocket * v_socket);


/*
 * Load ICONs and set default icon
 * return TRUE if ok, FALSE otherwise
 */
extern gboolean gapc_load_icons (PGAPC_CONFIG pcfg)
{
  guint    i_x 	   = 0;
  GError   *gerror = NULL;
  gboolean b_rc    = TRUE;
  
  gchar *pch_image_names[] = {
    "/usr/share/pixmaps/online.png",
    "/usr/share/pixmaps/onbatt.png",
    "/usr/share/pixmaps/charging.png",
    "/usr/share/pixmaps/apcupsd.png",
    "/usr/share/pixmaps/unplugged.png",    
    NULL
  };

  for (i_x = 0; (pch_image_names[i_x] != NULL) && (i_x < GAPC_N_ICONS); i_x++)
    {
      pcfg->my_icons[i_x] = gdk_pixbuf_new_from_file (pch_image_names[i_x], &gerror);
      if (gerror != NULL)
	  {
	  	gchar *pch = NULL;

	  	pch = g_strdup_printf ("Get Icon=%s Failed", pch_image_names[i_x]);
	  	gapc_log_app_error ("gapc_load_icons", pch, gerror->message);
	  	g_error_free (gerror);
	  	g_free (pch);
	  	gerror = NULL;
	  	b_rc = FALSE;
	  }
    }

  pcfg->i_old_icon_index = GAPC_N_ICONS;
  pcfg->i_icon_index = GAPC_ICON_DEFAULT;
  pcfg->size = 24;  
	
  return b_rc;
}

/*
 * save the contents of configuration values to a file
 * creating a new file if needed.
 * return FALSE on error
 *        TRUE  on sucess
 */
extern gint gapc_save_preferences (PGAPC_CONFIG pcfg)
{
  GIOChannel *gioc = NULL;
  GIOStatus gresult = 0;
  GError *gerror = NULL;
  GString *gs_config_values = NULL;

  g_return_val_if_fail (pcfg, FALSE);	/* error exit */
  g_return_val_if_fail (pcfg->pch_key_filename, FALSE);	/* error exit */

  gs_config_values = g_string_new (NULL);
  g_string_append_printf (gs_config_values, "%s\n%d\n%f\n",
			  pcfg->pch_host, pcfg->i_port, pcfg->d_refresh);

  gioc = g_io_channel_new_file (pcfg->pch_key_filename, "w", &gerror);
  if (gerror != NULL)
    {
      gchar *pch = g_strdup_printf ("open config file[%s] failed", pcfg->pch_key_filename);
      gapc_log_app_error ("gapc_save_preferences", pch, gerror->message);
      g_error_free (gerror);
      gerror = NULL;
      g_string_free (gs_config_values, TRUE);      
      g_free (pch );
      return FALSE;
    }

  gresult = g_io_channel_write_chars (gioc, gs_config_values->str,
				      gs_config_values->len, NULL, &gerror);
  if (gerror != NULL)
    {
      gapc_log_app_error ("gapc_save_preferences",
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
      gapc_log_app_error ("gapc_save_preferences",
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
extern gint gapc_load_preferences (PGAPC_CONFIG pcfg)
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
      gchar *pch = g_strdup_printf ("open config file[%s] failed", pcfg->pch_key_filename);
      gapc_log_app_error ("gapc_load_preferences", pch, gerror->message);
      g_error_free (gerror);
      gerror = NULL;
      g_free (pch);
      if (pcfg->pch_host != NULL)
	  	  g_free (pcfg->pch_host);
      pcfg->pch_host = g_strdup ("localhost");
      pcfg->i_port = 3551;
      pcfg->d_refresh = 15;
      return FALSE;
    }

  gresult = g_io_channel_read_line (gioc, &pch_host, &gLen, &gPos, &gerror);
  if (gerror != NULL)
    {
      gapc_log_app_error ("gapc_load_preferences",
			  "read hostname failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;

      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      if (pcfg->pch_host != NULL)
	 	  g_free (pcfg->pch_host);
      pcfg->pch_host = g_strdup ("localhost");
      pcfg->i_port = 3551;
      pcfg->d_refresh = 15;
      return FALSE;
    }

  pch_host[gPos] = 0;
  if (pcfg->pch_host != NULL)
	  g_free (pcfg->pch_host);

  pcfg->pch_host = pch_host;

  gresult = g_io_channel_read_line (gioc, &pch_port, &gLen, &gPos, &gerror);
  if (gerror != NULL)
    {
      gapc_log_app_error ("gapc_load_preferences",
			  "read port number failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      pcfg->i_port = 3551;
      pcfg->d_refresh = 15;
      return FALSE;
    }

  pcfg->i_port = (gint) g_strtod (pch_port, NULL);

  gresult = g_io_channel_read_line (gioc, &pch_port, &gLen, &gPos, &gerror);
  if (gerror != NULL)
    {
      gapc_log_app_error ("gapc_load_preferences",
			  "read refresh value failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
      g_io_channel_shutdown (gioc, TRUE, NULL);
      g_io_channel_unref (gioc);
      pcfg->d_refresh = 15;
      return FALSE;
    }

  pcfg->d_refresh = g_strtod (pch_port, NULL);

  gresult = g_io_channel_shutdown (gioc, TRUE, &gerror);
  if (gerror != NULL)
    {
      gapc_log_app_error ("gapc_load_preferences",
			  "GIO channel shutdown failed", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }

  g_io_channel_unref (gioc);

  return TRUE;
}


/*
 * Handle the recording of application error messages
 * return FALSE to terminate timer that called us.
 */
static gboolean
gapc_cb_application_message (gpointer pch)
{

  g_return_val_if_fail (pch, FALSE);	/* error exit */

  g_message (pch);
  g_free (pch);

  return FALSE;
}


/* ***************************************************************************
 *  Implements a Horizontal Bar Chart...
 *  - data value has a range of 0.0 to 1.0 for 0-100% display
 *  - in chart text is limited to about 30 chars
 */
static gboolean
gapc_cb_h_bar_chart_exposed (GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
  PGAPC_BAR_H pbar = data;
  gint i_percent = 0;
  PangoLayout *playout = NULL;

  g_return_val_if_fail (data, FALSE);	/* error exit */

  pbar->rect.x = 0;
  pbar->rect.y = 0;
  pbar->rect.width = widget->allocation.width;
  pbar->rect.height = widget->allocation.height;

  /* scale up the less than zero data value */
  i_percent =
    (gint) ((gdouble) (widget->allocation.width / 100.0) *
	    (gdouble) (pbar->d_value * 100.0));

  /* the frame of the chart */
  gtk_paint_box (widget->style, widget->window, GTK_WIDGET_STATE (widget),
		 GTK_SHADOW_ETCHED_IN,
		 &pbar->rect,
		 widget,
		 "gapc_hbar_frame",
		 0, 0, widget->allocation.width - 1, widget->allocation.height - 1);

  /* the scaled value */
  gtk_paint_box (widget->style, widget->window, GTK_STATE_SELECTED,
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
			"gapc_hbar_text", 
			(pbar->b_center_text) ? x : 6, y, 
			playout);

      g_object_unref (playout);
    }

  return TRUE;
}

/* 
 * callback for APPLY push button on config page
 */
static void gapc_cb_button_config_apply (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  gchar *pch = NULL;
  GtkEntry *ef = NULL;
  gdouble d_old_time = 0.0;
  GtkWidget *w = NULL;
  gchar ch_old_host[GAPC_MAX_TEXT];
  guint i_old_port = 0, x = 0;


  g_return_if_fail (pcfg);	/* error exit */

  x = g_snprintf ( ch_old_host, GAPC_MAX_TEXT, "%s", pcfg->pch_host );
  i_old_port = pcfg->i_port;

  /* get current value of entry-fields */
  ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerAddress");
  if (pcfg->pch_host != NULL)
      g_free (pcfg->pch_host);

  pch = (gchar *) gtk_entry_get_text (ef);
  pcfg->pch_host = g_strdup (pch);

  ef = g_hash_table_lookup (pcfg->pht_Widgets, "ServerPort");
  pch = (gchar *) gtk_entry_get_text (ef);
  pcfg->i_port = (gint) g_strtod (pch, NULL);

  ef = g_hash_table_lookup (pcfg->pht_Widgets, "RefreshInterval");
  d_old_time = pcfg->d_refresh;
  pcfg->d_refresh = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ef));

  if (pcfg->d_refresh != d_old_time)	/* changed ?? */
      pcfg->b_timer_control = TRUE;

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_push (GTK_STATUSBAR (w),
		      pcfg->i_info_context, "Configuration Applied...");  
 
  return;
}

/* 
 * callback for SAVE push button on config page
 */
static void gapc_cb_button_config_save (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  g_return_if_fail (pcfg);	/* error exit */

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);

  gapc_cb_button_config_apply (button, gp);	/* use existing code */

  if (gapc_save_preferences (pcfg))
    gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context, "Configuration Saved...");
  else
    gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context, "Configuration NOT SAVED...");    

  return;
}

/* 
 * callback for REFRESH push button on main page
 */
extern void gapc_cb_refresh_button (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  g_return_if_fail (gp != NULL);

  if ( !pcfg->b_run )
     return;

  pcfg->b_refresh_button = TRUE;
  pcfg->b_window_visible = TRUE;
  
  g_source_remove ( pcfg->i_timer_ids[GAPC_TIMER_DEDICATED] );
  pcfg->i_timer_ids[GAPC_TIMER_DEDICATED] = 
       g_timeout_add (200, gapc_cb_dedicated_one_time_refresh, pcfg);	/* one shot timer */

  return;
}

/* 
 * timer service routine for IPL refresh and refresh_button. 
 * used to overcome the multi-threaded startup delay.  very short
 */
extern gboolean gapc_cb_dedicated_one_time_refresh (gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  g_return_val_if_fail (gp != NULL, FALSE);

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);


  if (gapc_update_tooltip_msg (pcfg))
    return TRUE;		/* network offline */

  if ( !pcfg->b_run )
     return FALSE;

  if (!gapc_monitor_update (pcfg))
    {
      gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);

      gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			  "Refresh Not Completed... Network Busy!");

      return TRUE;		/* try again */
    }

  gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context, "Refresh Completed...");


  return FALSE;			/* this will terminate the timer */
}

/* 
 * used to restart timers when interval changes.
 */
extern gboolean gapc_cb_timer_control (gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  guint i_new_time = 0;
  GtkWidget *w = NULL;

  g_return_val_if_fail (gp != NULL, FALSE);

  if (!pcfg->b_run)
    return FALSE;

  pcfg->b_timer_control = FALSE;	/* reset flag */
  i_new_time = (guint) (gdouble) (pcfg->d_refresh * 1.04) * 1000.0;

  g_source_remove ( pcfg->i_timer_ids[GAPC_TIMER_AUTO] );

  pcfg->i_timer_ids[GAPC_TIMER_AUTO] = 
                  g_timeout_add (i_new_time, gapc_cb_auto_refresh, pcfg);

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
		      "Refresh Timer Update Completed...");

  return FALSE;
}

/* 
 * timer service routine for normal data display refreshs
 */
extern gboolean gapc_cb_auto_refresh (gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  GtkWidget *w = NULL;

  g_return_val_if_fail (gp != NULL, FALSE);

  if (!pcfg->b_run)
    return FALSE;

  if (pcfg->b_timer_control)	/* change timers */
    {
      pcfg->i_timer_ids[GAPC_TIMER_CONTROL] = 
                       g_timeout_add (200, gapc_cb_timer_control, pcfg);
      return FALSE;
    }


  if (gapc_update_tooltip_msg (pcfg))
    return TRUE;		/* network offline */

  w = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
  gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);


  if (!gapc_monitor_update (pcfg))
    {
      gtk_statusbar_pop (GTK_STATUSBAR (w), pcfg->i_info_context);

      gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			  "Automatic Update Skipped...  Thread is busy");

      return TRUE;
    }

  if (pcfg->b_window_visible)
    gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			"Automatic Update Completed... !");
  else
    gtk_statusbar_push (GTK_STATUSBAR (w), pcfg->i_info_context,
			"Automatic Update Completed... ?");

  return TRUE;
}


/* 
 *  capture the current application related error values 
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this may be a background thread)
 */
static void gapc_log_app_error (gchar * pch_func, gchar * pch_topic, gchar * pch_emsg)
{
  gchar *pch = NULL;

  g_return_if_fail (pch_func != NULL);

  pch = g_strdup_printf ("%s(%s) emsg=%s", pch_func, pch_topic, pch_emsg);

  g_timeout_add (100, gapc_cb_application_message, pch);

  return;
}

/* 
 *  capture the current network related error values 
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this is a background thread)
 */
static void gapc_log_net_error (gchar * pch_func, gchar * pch_topic, GnomeVFSResult result)
{
  gchar *pch = NULL;

  g_return_if_fail (pch_func != NULL);

  pch = g_strdup_printf ("%s(%s) emsg=%s",
			 pch_func, pch_topic, gnome_vfs_result_to_string (result));

  g_timeout_add (100, gapc_cb_application_message, pch);

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
      result = gnome_vfs_socket_read (psocket, ptr, nleft, &nread GNOMEVFS_CANCELLATION);

      if (result != GNOME_VFS_OK)
	  {
	  	gapc_log_net_error ("read_nbytes", "read from network failed", result);
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
      result = gnome_vfs_socket_write (psocket, ptr, nleft, &nwritten GNOMEVFS_CANCELLATION);

      if (result != GNOME_VFS_OK)
	  {
	  	gapc_log_net_error ("write_nbytes", "write to network failed", result);
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
 * Returns connection id value  -- version 2.8 maybe required for reusing addresses ---
 *   if this is an issue pass routine the address of a int set to FALSE
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
      gapc_log_net_error ("net_open", "create inet connection failed", result);
      return NULL;
    }
 
  return connection;
}

/* Close the network connection */
static void
gapc_net_close (GnomeVFSInetConnection * connection, GnomeVFSSocket * v_socket)
{
  gshort pktsiz = 0;

  if ( connection == NULL )
  	   return;

  /* send EOF sentinel */
  gapc_net_write_nbytes (v_socket, (gchar *) & pktsiz, sizeof (gshort));
  gnome_vfs_inet_connection_destroy (connection, NULL);

  return;
}


/*
 * performs a complete NIS transaction by sending cmd and 
 * loading each result line into the pch array.
 * also, refreshes status key/value pairs in hastable.
 * return error = 0,  or number of lines read from network
 */
extern gint gapc_net_transaction_service (PGAPC_CONFIG pcfg, gchar * cp_cmd, gchar ** pch)
{
  gint n = 0, iflag = 0, i_port = 0;
  gchar recvline[GAPC_MAX_TEXT];
  GnomeVFSInetConnection *v_connection = NULL;
  GnomeVFSSocket *v_socket = NULL;


  g_return_val_if_fail (pcfg, -1);
  g_return_val_if_fail (pcfg->pch_host, -1);

  i_port = pcfg->i_port;
  
  v_connection = gapc_net_open (pcfg->pch_host, i_port );
  if (v_connection == NULL)
    {
      return 0;
    } 
  
  v_socket = gnome_vfs_inet_connection_to_socket (v_connection);
  if (v_socket == NULL)
    {
      gapc_log_net_error ("transaction_service", "connect to socket for io failed",
			  GNOME_VFS_OK);
      gnome_vfs_inet_connection_destroy (v_connection, NULL);
      return 0;
    }

  n = gapc_net_send (v_socket, cp_cmd, g_utf8_strlen (cp_cmd, -1));
  if (n < 0)
    {
      gapc_net_close (v_connection, v_socket);
      return 0;
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

  g_return_val_if_fail (pcfg != NULL, FALSE);
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

/*
 * Manage the state icon in the panel and the associated tooltip
 * Composes the expanded tooltip message
 */
static gboolean gapc_update_tooltip_msg (PGAPC_CONFIG pcfg)
{
  gchar *pchx = NULL, *pmsg = NULL, *ptitle = NULL;
  gchar *pch1 = NULL, *pch2 = NULL, *pch3 = NULL, *pch4 = NULL, *pch5 = NULL;  
  gchar *pch6 = NULL, *pch7 = NULL, *pch8 = NULL, *pch9 = NULL, *pcha = NULL;  
  gchar *pchb = NULL, *pchc = NULL, *pchd = NULL, *pche = NULL;
  GtkWidget *w = NULL;
  gdouble d_value = 0.0;
  gboolean b_flag = FALSE;
  
  g_return_val_if_fail (pcfg != NULL, TRUE);

  pcfg->i_icon_index = GAPC_ICON_ONLINE;

  w = g_hash_table_lookup (pcfg->pht_Widgets, "TitleStatus");
  
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "UPSNAME");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "HOSTNAME");  
  if ( pch2 == NULL )
  {
       pch2 = pcfg->pch_host;
  }
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "STATUS");
  if ( pch3 == NULL )
  {
       pch3 = "NISERROR";
  } 
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "NUMXFERS");
  pch5 = g_hash_table_lookup (pcfg->pht_Status, "XONBATT");  
  pch6 = g_hash_table_lookup (pcfg->pht_Status, "LINEV");
  pch7 = g_hash_table_lookup (pcfg->pht_Status, "BCHARGE");
  if ( pch7 == NULL )
  {
       pch7 = "n/a";
  } 
  pch8 = g_hash_table_lookup (pcfg->pht_Status, "LOADPCT");    
  pch9 = g_hash_table_lookup (pcfg->pht_Status, "TIMELEFT");
  pcha = g_hash_table_lookup (pcfg->pht_Status, "VERSION");
  pchb = g_hash_table_lookup (pcfg->pht_Status, "STARTTIME");
  pchc = g_hash_table_lookup (pcfg->pht_Status, "MODEL");
  pchd = g_hash_table_lookup (pcfg->pht_Status, "UPSMODE");  
  pche = g_hash_table_lookup (pcfg->pht_Status, "CABLE");    

  if (pcfg->b_data_available)
  {
    d_value = g_strtod (pch7, NULL);
    pchx = NULL;
    if (g_strrstr (pch3, "COMMLOST") != NULL)
    {
        pchx = " cable un-plugged...";
        pcfg->i_icon_index = GAPC_ICON_UNPLUGGED;
        b_flag = TRUE;            
    } else if ((d_value < 99.0) && (g_strrstr (pch3, "LINE") != NULL))
           {
                pchx = " and charging...";
                pcfg->i_icon_index = GAPC_ICON_CHARGING;
           } else if (g_strrstr (pch3, "BATT") != NULL)
                  {
                      pchx = " on battery...";
                      pcfg->i_icon_index = GAPC_ICON_ONBATT;
                  }
  }
  else
  {
    b_flag = TRUE;      
    if (g_strrstr (pch3, "COMMLOST") != NULL)
    {
      pchx = " cable un-plugged...";
      pcfg->i_icon_index = GAPC_ICON_UNPLUGGED;
        b_flag = TRUE;                  
    }
    else
    {
        pchx = "NIS network error...";
        pch3 = " ";
        pcfg->i_icon_index = GAPC_ICON_DEFAULT;
        b_flag = TRUE;                    
    }
  }
    
  if ( b_flag )
  {
  ptitle = g_strdup_printf ( 
  		"<span foreground=\"red\" size=\"large\">"
		"Monitored UPS %s on %s is %s%s"
		"</span>",
	      (pch1 != NULL) ? pch1 : "unknown",
	      (pch2 != NULL) ? pch2 : "unknown", 
	      (pch3 != NULL) ? pch3 : "n/a", 	      
	      (pchx != NULL) ? pchx : " " );
  }      
  else
  {
  ptitle = g_strdup_printf ( 
  		"<span foreground=\"blue\" size=\"large\">"
		"Monitored UPS %s on %s is %s%s"
		"</span>",
	      (pch1 != NULL) ? pch1 : "unknown",
	      (pch2 != NULL) ? pch2 : "unknown", 
	      (pch3 != NULL) ? pch3 : "n/a", 	      
	      (pchx != NULL) ? pchx : " " );
  }
            
  pmsg = g_strdup_printf (  "APCUPSD Monitor(%d)\n"
  			   "UPS %s at %s\n"
  			   "Status: %s%s\n"
    		 "----------------------------------------------------------\n"
  			   "%s Outage[s]\n"
  			   "Last one on %s\n"
  			   "%s Utility VAC\n"			   
  			   "%s Battery Charge\n"  			   
  			   "%s UPS Load\n"  			     			   
  			   "%s Remaining\n"  			     			 
    		 "----------------------------------------------------------\n"  			     
  			   "Build: %s\n"  			     			   
  			   "Start: %s\n"  			     		
    		 "----------------------------------------------------------\n"  			   	   
  			   "Model: %s\n"  			     			   
  			   " Mode: %s\n"  			     			   
  			   "Cable: %s ",
  			   pcfg->cb_id,
			  (pch1 != NULL) ? pch1 : "unknown",
			  (pch2 != NULL) ? pch2 : "unknown",
			  (pch3 != NULL) ? pch3 : "n/a",
			  (pchx != NULL) ? pchx : " ",
			  (pch4 != NULL) ? pch4 : "n/a",
			  (pch5 != NULL) ? pch5 : "n/a",
			  (pch6 != NULL) ? pch6 : "n/a",
			  (pch7 != NULL) ? pch7 : "n/a",
			  (pch8 != NULL) ? pch8 : "n/a",
			  (pch9 != NULL) ? pch9 : "n/a",
			  (pcha != NULL) ? pcha : "n/a",
			  (pchb != NULL) ? pchb : "n/a",
			  (pchc != NULL) ? pchc : "n/a",		  
			  (pchd != NULL) ? pchd : "n/a",		  
			  (pche != NULL) ? pche : "n/a");


  if ( pcfg->tooltips != NULL )
	   gtk_tooltips_set_tip (pcfg->tooltips, GTK_WIDGET (pcfg->applet), pmsg, NULL);	   
  else gtk_window_set_title ( pcfg->window, pmsg); /* this must be gapcmon and not gpanel_apcmon */
	   
  gapc_change_status_icon (pcfg);
  gtk_label_set_markup (GTK_LABEL (w), ptitle);
  
  g_free ( pmsg );
  g_free ( ptitle );  
    
  return b_flag;
}


static gint
gapc_text_view_clear_buffer (GtkWidget * view)
{
  GtkTextIter start, end;
  GtkTextBuffer *buffer = NULL;

  g_return_val_if_fail (view != NULL, FALSE);

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

  g_return_if_fail (view != NULL);
  
  if ( s == NULL )
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_start_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, s, -1);
}

static void
gapc_text_view_append (GtkWidget * view, gchar * s)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;

  g_return_if_fail (view != NULL);

  if ( s == NULL )
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, s, -1);
}


static GtkWidget *
gapc_create_scrolled_text_view (GtkWidget * box)
{
  PangoFontDescription *font_desc;
  GtkWidget *scrolled, *view;

  g_return_val_if_fail (box != NULL, NULL);

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

  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 5);

  return view;
}

/*
 * Worker thread for network communications.
 */
extern gpointer *gapc_network_thread (PGAPC_CONFIG pcfg)
{
  gint s = 0;
  gulong gul_qtr = 0;

  g_return_val_if_fail (pcfg != NULL, NULL);

  while (pcfg->b_run)
    {
      gul_qtr = pcfg->d_refresh * 250000;

      g_mutex_lock (pcfg->gm_update);

      if (pcfg->b_run)
	  if ((s = gapc_net_transaction_service (pcfg, "status", pcfg->pach_status)))
	  	   gapc_net_transaction_service (pcfg, "events", pcfg->pach_events);

      g_mutex_unlock (pcfg->gm_update);

      if (s > 0)
	 	  pcfg->b_data_available = TRUE;
      else
      	  pcfg->b_data_available = FALSE;

      pcfg->b_refresh_button = FALSE;
      
      if (pcfg->b_data_available)	/* delay controls */
	  {
	  	if (pcfg->b_run)
	    	g_usleep (gul_qtr);

	    if (pcfg->b_refresh_button)
	    	continue;

	  	if (pcfg->b_run)
	    	g_usleep (gul_qtr);

	  	if (pcfg->b_refresh_button)
	    	continue;

	  	if (pcfg->b_run)
	    	g_usleep (gul_qtr);

	  	if (pcfg->b_refresh_button)
	    	continue;

	  	if (pcfg->b_run)
	    	g_usleep (gul_qtr);
	  }
      else g_usleep (gul_qtr * 2);	/* do the full wait */

    }

  g_thread_exit (pcfg);

  return NULL;
}

/*
 * main data updating routine.
 * -- collects and pushes data to all ui
 */
static gint gapc_monitor_update (PGAPC_CONFIG pcfg)
{
  gint i_x = 0;
  GtkWidget *win = NULL, *w = NULL;
  gchar *pch = NULL, *pch1 = NULL, *pch2 = NULL, 
    *pch3 = NULL, *pch4 = NULL, *pch5 = NULL, *pch6 = NULL;
  gdouble dValue = 0.00, dScale = 0.0, dtmp = 0.0, dCharge = 0.0;
  gchar ch_buffer[GAPC_MAX_TEXT];
  PGAPC_BAR_H pbar = NULL;


  g_return_val_if_fail (pcfg != NULL, FALSE);

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
  if (dScale == 0.0)
      dScale = ((gint)(dValue / 100) * 100) + 100.0;
  dValue /= dScale;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar1");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s from Utility", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar1-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryVoltage");
  pch = g_hash_table_lookup (pcfg->pht_Status, "BATTV");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "NOMBATTV");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dValue = g_strtod (pch, NULL);
  dScale = g_strtod (pch1, NULL);
  dScale *= 1.2;
  if (dScale == 0.0)
      dScale = ((gint)(dValue / 10.0) * 10) + 10.0;
  dValue /= dScale;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar2");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s DC on Battery", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar2-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "BatteryCharge");
  pch = g_hash_table_lookup (pcfg->pht_Status, "BCHARGE");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (win), pch);
  dCharge = dValue = g_strtod (pch, NULL);
  dValue /= 100.0;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (win), dValue);
  pbar = g_hash_table_lookup (pcfg->pht_Status, "HBar3");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), "%s Battery Charge", pch);
  w = g_hash_table_lookup (pcfg->pht_Widgets, "HBar3-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
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
  if (GTK_WIDGET_DRAWABLE (w))
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
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);


  win = g_hash_table_lookup (pcfg->pht_Widgets, "SoftwareInformation");
  pch = g_hash_table_lookup (pcfg->pht_Status, "VERSION");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "CABLE");
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "UPSMODE");
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "STARTTIME");
  pch5 = g_hash_table_lookup (pcfg->pht_Status, "STATUS");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\">"
	      "%s\n%s\n%s\n%s\n%s\n%s"
	      "</span>",
	      (pch != NULL) ? pch : "N/A",
	      (pch1 != NULL) ? pch1 : "N/A",
	      (pch2 != NULL) ? pch2 : "N/A",
	      (pch3 != NULL) ? pch3 : "N/A",
	      (pch4 != NULL) ? pch4 : "N/A", (pch5 != NULL) ? pch5 : "N/A");
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
	      (pch != NULL) ? pch : "N/A",
	      (pch1 != NULL) ? pch1 : "N/A",
	      (pch2 != NULL) ? pch2 : "N/A",
	      (pch3 != NULL) ? pch3 : "N/A",
	      (pch4 != NULL) ? pch4 : "N/A",
	      (pch5 != NULL) ? pch5 : "N/A", (pch6 != NULL) ? pch6 : "N/A");
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (pcfg->pht_Widgets, "ProductInformation");
  pch = g_hash_table_lookup (pcfg->pht_Status, "MODEL");
  pch1 = g_hash_table_lookup (pcfg->pht_Status, "SERIALNO");
  pch2 = g_hash_table_lookup (pcfg->pht_Status, "MANDATE");
  pch3 = g_hash_table_lookup (pcfg->pht_Status, "FIRMWARE");
  pch4 = g_hash_table_lookup (pcfg->pht_Status, "BATTDATE");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
	      "<span foreground=\"blue\">"
	      "%s\n%s\n%s\n%s\n%s"
	      "</span>",
	      (pch != NULL) ? pch : "N/A",
	      (pch1 != NULL) ? pch1 : "N/A",
	      (pch2 != NULL) ? pch2 : "N/A",
	      (pch3 != NULL) ? pch3 : "N/A", (pch4 != NULL) ? pch4 : "N/A");
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

  g_return_val_if_fail (pcfg != NULL, NULL);

  pbar = g_new0 (GAPC_BAR_H, 1);
  pbar->d_value = d_percent;
  pbar->b_center_text = FALSE;
  g_strlcpy (pbar->c_text, pch_text, sizeof (pbar->c_text));

  drawing_area = gtk_drawing_area_new ();	/* manual bargraph */
  gtk_widget_set_size_request (drawing_area, 100, 20);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (gapc_cb_h_bar_chart_exposed), (gpointer) pbar);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
  g_hash_table_insert (pcfg->pht_Status, g_strdup (pch_hbar_name), pbar);
  pch = g_strdup_printf ("%s-Widget", pch_hbar_name);
  g_hash_table_insert (pcfg->pht_Widgets, pch, drawing_area);

  return drawing_area;
}


extern gint gapc_create_notebook_page_overview (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  GtkWidget *frame, *label, *pbox, *lbox, *rbox, *pbar;
  GtkSizeGroup *gSize;
  gint i_page = 0;

  /* Create the Overview Notebook Page */
  frame = gtk_frame_new ("UPS Status Overview");
	gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  	gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  label  = gtk_label_new ("Overview");
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

extern gint
gapc_create_notebook_page_information (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  GtkWidget *frame, *label, *pbox, *lbox, *rbox, *table3, *gbox;
  gint i_page = 0;

  /* Create a Notebook Page */
  table3 = gtk_table_new (4, 4, FALSE);
  label  = gtk_label_new ("Information");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table3, label);

  frame = gtk_frame_new ("<b><i>Software Information</i></b>");
  	gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  	gtk_table_attach (GTK_TABLE (table3), frame,
		    0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  	gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  	gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
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

  gapc_create_h_barchart (pcfg, gbox, "HBar1", 10.8, "Waiting for refresh");
  gapc_create_h_barchart (pcfg, gbox, "HBar2", 40.8, "Waiting for refresh");
  gapc_create_h_barchart (pcfg, gbox, "HBar3", 80.8, "Waiting for refresh");
  gapc_create_h_barchart (pcfg, gbox, "HBar4", 40.8, "Waiting for refresh");
  gapc_create_h_barchart (pcfg, gbox, "HBar5", 10.8, "Waiting for refresh");

  frame = gtk_frame_new ("<b><i>Performance Summary</i></b>");
  	gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  	gtk_table_attach (GTK_TABLE (table3), frame,
		    0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  	gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  	gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
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
		    2, 3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 2);

  pbox = gtk_hbox_new (FALSE, 4);
  	gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (TRUE, 0);
  	gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
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

extern gint
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

extern gint
gapc_create_notebook_page_configuration (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  gint i_page = 0;
  GtkWidget *frame, *label, *table2, *entry, *button;
  GtkAdjustment *adjustment = NULL;
  gchar ch_buffer[16], *pch = NULL;

  /* Create Configuration Notebook Page */
  frame = gtk_frame_new ("Configuration Options");
  	gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  	gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
  label = gtk_label_new ("Config");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

  table2 = gtk_table_new (5, 5, TRUE);
  	gtk_container_add (GTK_CONTAINER (frame), table2);

  label = gtk_label_new ("Server hostname or ip address:");
  entry = gtk_entry_new_with_max_length (255);
  	gtk_table_attach (GTK_TABLE (table2), label, 0, 2, 0, 1, GTK_FILL, 0, 0, 5);
  	gtk_table_attach (GTK_TABLE (table2), entry, 2, 4, 0, 1, GTK_EXPAND | GTK_FILL, 0, 2, 5);
	gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  	gtk_entry_set_text (GTK_ENTRY (entry), pcfg->pch_host);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("ServerAddress"), entry);

  label = gtk_label_new ("Server port number:");
  	gtk_table_attach (GTK_TABLE (table2), label, 0, 2, 1, 2, GTK_FILL, 0, 0, 5);
  entry = gtk_entry_new_with_max_length (10);
  	gtk_table_attach (GTK_TABLE (table2), entry, 2, 3, 1, 2, GTK_FILL, 0, 2, 5);
  	gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  	gtk_entry_set_text (GTK_ENTRY (entry),
		      g_ascii_dtostr (ch_buffer,
				      sizeof (ch_buffer), (gdouble) pcfg->i_port));
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("ServerPort"), entry);

  label = gtk_label_new ("Refresh Interval in seconds:");
  	gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 1.0);
  	gtk_table_attach (GTK_TABLE (table2), label, 0, 2, 2, 3, GTK_FILL, 0, 0, 5);
  	adjustment = (GtkAdjustment *) gtk_adjustment_new (pcfg->d_refresh,
						     GAPC_MIN_INCREMENT, 300, 1, 5, 5);
  entry = gtk_spin_button_new (adjustment, 10, 0);
  	gtk_table_attach (GTK_TABLE (table2), entry, 2, 3, 2, 3, GTK_FILL, 0, 2, 5);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("RefreshInterval"), entry);


  button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  	gtk_table_attach (GTK_TABLE (table2), button, 1, 2, 3, 4, GTK_FILL, 0, 5, 5);
  	g_signal_connect (button, "clicked", G_CALLBACK (gapc_cb_button_config_apply), pcfg);
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  	gtk_table_attach (GTK_TABLE (table2), button, 3, 4, 3, 4, GTK_FILL, 0, 5, 5);
  	g_signal_connect (button, "clicked", G_CALLBACK (gapc_cb_button_config_save), pcfg);

  pch = g_strdup_printf ("Configuration filename: <b>%s</b>", pcfg->pch_key_filename);
  label = gtk_label_new (pch);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  	gtk_table_attach (GTK_TABLE (table2), label, 0, 4, 5, 6, GTK_FILL, 0, 0, 5);

  return i_page;
}


