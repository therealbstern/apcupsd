/* gapcmon.c       serial-0054-7 ***********************************************
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

#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>

#define _GAPC_CORE_H_
#include "gapcmon_common.h"


/*
 * gp_apcmon_core.c routines
 */
extern gboolean   gapc_load_icons (PGAPC_CONFIG pcfg); 
extern gint   	  gapc_save_preferences (PGAPC_CONFIG pcfg);
extern gint   	  gapc_load_preferences (PGAPC_CONFIG pcfg);
extern gpointer  *gapc_network_thread (PGAPC_CONFIG pcfg);

extern gboolean   gapc_cb_dedicated_one_time_refresh (gpointer gp);
extern void 	  gapc_cb_refresh_button (GtkButton * button, gpointer gp);
extern gboolean   gapc_cb_auto_refresh (gpointer gp);

extern gint   gapc_create_notebook_page_overview (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_information (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_configuration (GtkWidget * notebook, PGAPC_CONFIG pcfg);
extern gint   gapc_create_notebook_page_text_report (GtkWidget * notebook, PGAPC_CONFIG pcfg, gchar * pchTitle, gchar * pchTab, gchar * pchKey);


/*
 * Internal routines
 */
extern int		  main (int argc, char *argv[]);
static gint 	  gapc_parse_args (gint argc, gchar ** argv, PGAPC_CONFIG pcfg);
static gint 	  gapc_get_preferences (PGAPC_CONFIG pcfg);
extern gboolean   gapc_change_status_icon(PGAPC_CONFIG pcfg);
static GtkWindow *gapc_create_user_interface (PGAPC_CONFIG pcfg);
static gint 	  gapc_create_notebook_page_about (GtkWidget * notebook, PGAPC_CONFIG pcfg);
static gboolean   gapc_cb_window_delete_event (GtkWidget * w, GdkEvent * event, gpointer gp);
static void 	  gapc_cb_button_quit (GtkButton * button, gpointer gp);

/*
 * Present here because of text subsitutions
 */
static GtkWindow *gapc_create_user_interface (PGAPC_CONFIG pcfg)
{
  GtkWidget *wStatus_bar;
  GtkWidget *notebook, *label, *button, *dbutton, *table, *evbox;
  GtkWindow *window = NULL;
  gint i_page = 0;

  g_return_val_if_fail (pcfg != NULL, NULL);

  /* *********************************************************
   * Create main window and display
   */
  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  	gtk_container_set_border_width (GTK_CONTAINER (window), 0);
  	gtk_window_set_title (GTK_WINDOW (window), GAPC_WINDOW_TITLE );
  	gtk_window_set_type_hint ( window, GDK_WINDOW_TYPE_HINT_NORMAL );
  	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
  	g_signal_connect (window, "delete_event", G_CALLBACK (gapc_cb_window_delete_event), pcfg);
  	gtk_window_set_icon (GTK_WINDOW(window), pcfg->my_icons[GAPC_ICON_ONLINE]);
 
  table = gtk_table_new (6, 4, FALSE);
  	gtk_container_add (GTK_CONTAINER (window), table);

  label = gtk_label_new (GAPC_GROUP_TITLE);
  g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("TitleStatus"), label);
  	gtk_table_attach (GTK_TABLE (table), label, 0, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 5);

  evbox = gtk_event_box_new ();
  	gtk_table_attach (GTK_TABLE (table), evbox, 0, 3, 1, 3,
		    GTK_EXPAND | GTK_FILL | GTK_SHRINK,
		    GTK_EXPAND | GTK_FILL | GTK_SHRINK, 5, 0);

  /* Create a new notebook, place the position of the tabs */
  notebook = gtk_notebook_new ();
  	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  	gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  	gtk_container_add (GTK_CONTAINER (evbox), notebook);

  /* Create the Notebook Pages */
  gapc_create_notebook_page_overview (notebook, pcfg);
 i_page = gapc_create_notebook_page_information (notebook, pcfg);
  gapc_create_notebook_page_text_report (notebook, pcfg, "Power Events", "Events", "EventsPage");
  gapc_create_notebook_page_text_report (notebook, pcfg, "Full UPS Status", "Status", "StatusPage");
  gapc_create_notebook_page_configuration (notebook, pcfg);
  gapc_create_notebook_page_about (notebook, pcfg);

  /* Control buttons */
  dbutton = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  	g_signal_connect (dbutton, "clicked", G_CALLBACK (gapc_cb_refresh_button), pcfg);
  	gtk_table_attach (GTK_TABLE (table), dbutton, 0, 1, 3, 4,
		    GTK_FILL | GTK_SHRINK, 0, 8, 4);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
  	g_signal_connect (button, "clicked", G_CALLBACK (gapc_cb_button_quit), pcfg);
  	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 3, 4,
		    GTK_FILL | GTK_SHRINK, 0, 8, 4);

  wStatus_bar = gtk_statusbar_new ();
  	g_hash_table_insert (pcfg->pht_Widgets, g_strdup ("StatusBar"), wStatus_bar);
  	gtk_table_attach (GTK_TABLE (table), wStatus_bar, 0, 3, 4, 5,
		    GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0, 0);

  pcfg->i_info_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (wStatus_bar),
						       "Informational");

  gtk_widget_realize (GTK_WIDGET (button));

  gtk_widget_set_size_request ( GTK_WIDGET(window), 580, 400 );

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i_page);
  GTK_WIDGET_SET_FLAGS (dbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (dbutton);

  return GTK_WINDOW (window);
}


/*
 * returns TRUE if config file was create and defaults were used.
 *         FALSE is parms were read from file.
 */
static gint gapc_get_preferences (PGAPC_CONFIG pcfg)
{
  gint i_create_file = 0;


  g_return_val_if_fail (pcfg, FALSE);	/* error exit */

  if (pcfg->pch_key_filename != NULL)
      g_free (pcfg->pch_key_filename);

  pcfg->pch_key_filename = g_strdup_printf (GAPC_CONFIG_FILE, g_get_home_dir ());

  i_create_file = gapc_load_preferences (pcfg);

  if (i_create_file != TRUE)
      gapc_save_preferences (pcfg);

  return i_create_file;
}

/*
 * Update the Window Icon
 */
extern gboolean gapc_change_status_icon (PGAPC_CONFIG pcfg)
{
  g_return_val_if_fail (pcfg, FALSE);	/* error exit */

  if ( pcfg->i_old_icon_index == pcfg->i_icon_index )
  	   return FALSE;
  	 
  pcfg->i_old_icon_index = pcfg->i_icon_index;     /* save it for next time */

  if ( pcfg->i_icon_index >= GAPC_N_ICONS) 
       pcfg->i_icon_index = GAPC_ICON_DEFAULT;

  if (pcfg->my_icons[pcfg->i_icon_index])
    {
	  gtk_window_set_icon (GTK_WINDOW(pcfg->window), pcfg->my_icons[pcfg->i_icon_index]); 
	  return TRUE;	  
    }

  return FALSE;
}

/*
 * Prevent the normal window exit, just hide the window 
 */
static gboolean gapc_cb_window_delete_event (GtkWidget * w, GdkEvent * event, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  guint i_x = 0;

  pcfg->b_run = FALSE;
  pcfg->b_refresh_button = TRUE;

  for ( i_x = 0; i_x < GAPC_N_TIMERS ; i_x++ )
  {
        if( pcfg->i_timer_ids[i_x] != 0 )
            g_source_remove ( pcfg->i_timer_ids[i_x] );
  }	
  gtk_main_quit ();
  return TRUE;
}


/* 
 * callback for QUIT push button on main window
 */
static void gapc_cb_button_quit (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  /* Stop the thread  and Refresh Timer */
  pcfg->b_run = FALSE;

  gdk_flush ();

  gtk_main_quit ();
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
  gapc_get_preferences (pcfg);

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
  if ( pcfg->d_refresh < GAPC_MIN_INCREMENT )
       pcfg->d_refresh = 15;
    
  g_string_free (gs_parm1, TRUE);
  g_string_free (gs_parm2, TRUE);

  return FALSE;
}


/* 
 * The about page in the information window
 */
static gint gapc_create_notebook_page_about (GtkWidget * notebook, PGAPC_CONFIG pcfg)
{
  GtkWidget  *label = NULL, *frame = NULL, *mbox  = NULL;
  GtkWidget  *hbox  = NULL, *vbox  = NULL, *image = NULL;
  gchar *about_text = NULL;
  gchar *about_msg  = NULL;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled = NULL;
  gint 		i_page  = 0;
  
  about_text = g_strdup_printf ("<b><big>%s Version %s</big></b>\n",
				  GAPC_GROUP_TITLE, GAPC_VERSION);
  about_msg  = g_strdup_printf ("<b>gui monitor for UPSs under the management"
  				" of the APCUPSD.sourceforge.net package</b>\n"
				"<i>http://gapcmon.sourceforge.net/</i>\n\n"
				"Copyright \xC2\xA9 2006 James Scott, Jr.\n"
				"skoona@users.sourceforge.net\n\n"
				"Released under the GNU Public License\n"
				"%s comes with ABSOLUTELY NO WARRANTY\n",
				GAPC_GROUP_TITLE);

  /* Create About page */
  frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  label  = gtk_label_new ("About");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

  vbox = gtk_vbox_new (FALSE, 0);
  	gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (TRUE, 0);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  frame = gtk_frame_new (NULL);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (hbox), frame);
  image = gtk_image_new ();
  	gtk_misc_set_alignment ((GtkMisc *) image, 1.0, 0.5);  	
  	gtk_container_add (GTK_CONTAINER (frame), image);
  	pixbuf = pcfg->my_icons[GAPC_ICON_DEFAULT];
  	scaled = gdk_pixbuf_scale_simple (pixbuf, 75, 100, GDK_INTERP_BILINEAR);
  	gtk_image_set_from_pixbuf (GTK_IMAGE (image), scaled);
  	gtk_widget_show (image);
  	gdk_pixbuf_unref ( scaled );  

  label = gtk_label_new (about_text);
  	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  	gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.7); 
    gtk_container_add (GTK_CONTAINER (hbox), label);

  mbox = gtk_vbox_new (FALSE, 0);
  	gtk_box_pack_start (GTK_BOX (vbox), mbox, TRUE, TRUE, 0);
  label = gtk_label_new (about_msg);
  	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  	gtk_misc_set_alignment ((GtkMisc *) label, 0.5, 0.5);
  	gtk_box_pack_start (GTK_BOX (mbox), label, TRUE, TRUE, 0);
				
  g_free (about_text);
  g_free (about_msg);  

  return i_page;
}


extern int  main (int argc, char *argv[])
{
  PGAPC_CONFIG pcfg = NULL;
  guint i_x = 0;
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
  pcfg->cb_id = getpid();
  pcfg->gm_update = g_mutex_new ();

  if (gapc_parse_args (argc, argv, pcfg))
  {
    gnome_vfs_shutdown ();
    gdk_threads_leave ();
    return 1;			/* exit if user only wanted help */
  }
  
  gapc_load_icons ( pcfg );
  
  pcfg->b_window_visible = TRUE;

  /* 
   * Create hash table for easy access to status info and widgets
   */
  pcfg->pht_Status = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  pcfg->pht_Widgets = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* 
   * Create an update thread to handle network io 
   */
  pcfg->b_run = TRUE;
  pcfg->tid_refresh =
    g_thread_create ((GThreadFunc) gapc_network_thread, pcfg, FALSE, NULL);

  pcfg->window =  gapc_create_user_interface (pcfg) ;
  window = pcfg->applet = GTK_WIDGET (pcfg->window);
  
  pcfg->tooltips = NULL;

  gtk_widget_show_all ( window );

  /* 
   * Add a timer callback to refresh ups data 1 sec = 1/1000 
   */
  if ( pcfg->d_refresh < GAPC_MIN_INCREMENT )
       pcfg->d_refresh = GAPC_REFRESH_INCREMENT;

  i_x = (guint)(gdouble)(pcfg->d_refresh * 1.04) * 1000.0;

  pcfg->i_timer_ids[GAPC_TIMER_AUTO] = 
                    g_timeout_add (i_x, gapc_cb_auto_refresh, pcfg);

  pcfg->i_timer_ids[GAPC_TIMER_DEDICATED] = 
                    g_timeout_add (200, gapc_cb_dedicated_one_time_refresh, pcfg);

  /* 
   * enter the GTK main loop 
   */
  gtk_main ();
  gdk_threads_leave ();

  g_hash_table_destroy (pcfg->pht_Status);
  g_hash_table_destroy (pcfg->pht_Widgets);

  gnome_vfs_shutdown ();

  /* 
   * cleanup big memory arrays 
   */
  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_status[i_x])
      g_free (pcfg->pach_status[i_x]);
  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_events[i_x])
      g_free (pcfg->pach_events[i_x]);


  return (0);
}

