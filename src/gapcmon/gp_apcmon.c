/* gp_apcmon.c       serial-0054-7 *********************************************
  Gtk applet for monitoring the apcupsd.sourceforge.net package.
  
  Created Feb 1,2006
  Copyright (C) 2006 James Scott, Jr. <skoona@users.sourceforge.net>
  
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

#include <glib.h>
#include <gtk/gtk.h>
#include <panel-applet.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>

#define _GPANEL_APCMON_H_
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
 * Internal Routines
 */
static GtkWindow *gapc_main_win (PGAPC_CONFIG pcfg);
static GtkWindow *gapc_create_user_interface (PGAPC_CONFIG pcfg);
static gboolean   gapc_applet_factory (PanelApplet * applet, const gchar * iid, gpointer data);
static gboolean   gapc_applet_populate (PanelApplet * applet, PGAPC_CONFIG pcfg);
static gint 	  gapc_get_preferences (PGAPC_CONFIG pcfg);
extern gboolean   gapc_change_status_icon (PGAPC_CONFIG pcfg);

static gboolean gapc_cb_toggle_main_window (GtkWidget * widget, GdkEventButton * event, gpointer gp);
static gboolean gapc_cb_window_delete_event (GtkWidget * w, GdkEvent * event, gpointer gp);
static void 	gapc_cb_applet_change_size (PanelApplet * applet, gint size, PGAPC_CONFIG pcfg);
static void 	gapc_cb_menu_about (BonoboUIComponent * uic, gpointer gp, const gchar * verbname);
static void 	gapc_cb_menu_main_window (BonoboUIComponent * uic, gpointer gp, const gchar * verbname);
static void 	gapc_cb_applet_destroy (GtkObject * object, gpointer gp);
static void 	gapc_cb_about_dialog_response  (GtkDialog *dialog,  gint arg1,  gpointer gp);
static void 	gapc_cb_button_quit (GtkButton * button, gpointer gp);


/*
 * Create user interface
 * not in common file because of text subsitutions
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
  	gtk_window_set_icon (GTK_WINDOW(window), pcfg->my_icons[GAPC_ICON_DEFAULT]);
 
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

  pcfg->i_info_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (wStatus_bar), "Informational");

  gtk_widget_realize (GTK_WIDGET (button));

  gtk_widget_set_size_request ( GTK_WIDGET(window), 580, 400 );

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i_page);
  GTK_WIDGET_SET_FLAGS (dbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (dbutton);

  return GTK_WINDOW (window);
}


/*
 * Update the panel icon
 */
extern gboolean gapc_change_status_icon (PGAPC_CONFIG pcfg)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled = NULL;
  guint size = 0;

  g_return_val_if_fail (pcfg, FALSE);	/* error exit */


  if( pcfg->i_old_icon_index == pcfg->i_icon_index )  
      return FALSE;
      
  pcfg->i_old_icon_index = pcfg->i_icon_index;     /* save it for next time */

  if ( pcfg->i_icon_index >= GAPC_N_ICONS) 
       pcfg->i_icon_index = GAPC_ICON_DEFAULT;

  pixbuf = pcfg->my_icons[pcfg->i_icon_index];
  if (pixbuf)
    {
      size = panel_applet_get_size (PANEL_APPLET (pcfg->applet));
      pcfg->size = size;

      scaled = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);

      gtk_image_set_from_pixbuf (GTK_IMAGE (pcfg->image), scaled);
      gtk_widget_show (pcfg->image);
      
      gdk_pixbuf_unref ( scaled );
    }

  return TRUE;
}


/* 
 * callback for QUIT push button on main window
 */
static void gapc_cb_button_quit (GtkButton * button, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  g_return_if_fail (pcfg);	/* error exit */

  pcfg->b_window_visible = FALSE;

  gtk_widget_hide_all (GTK_WIDGET (pcfg->window));
}

/*
 * Prevent the normal window exit, just hide the window 
 */
static gboolean gapc_cb_window_delete_event (GtkWidget * w, GdkEvent * event, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  g_return_val_if_fail (pcfg, FALSE);	/* error exit */

  pcfg->b_window_visible = FALSE;

  return gtk_widget_hide_on_delete (w);
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

  pcfg->pch_key_filename = g_strdup_printf (GAPC_CONFIG_FILE, g_get_home_dir (), pcfg->cb_id);

  i_create_file = gapc_load_preferences (pcfg);

  if (i_create_file != TRUE)
      gapc_save_preferences (pcfg);

  return i_create_file;
}

/*
 * Intialize system and create main user interface
 */
static GtkWindow * gapc_main_win (PGAPC_CONFIG pcfg)
{
  guint i_x = 0;
  GtkWindow *window;

  g_return_val_if_fail (pcfg != NULL, NULL);

  gapc_get_preferences (pcfg);

  pcfg->pht_Status  = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  pcfg->pht_Widgets = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Create an update thread to handle network io */

  pcfg->b_run = TRUE;
  pcfg->tid_refresh =
    g_thread_create ((GThreadFunc) gapc_network_thread, pcfg, TRUE, NULL);

  window = gapc_create_user_interface (pcfg);

  /* Main refresh timer calcs */
  if ( pcfg->d_refresh < GAPC_MIN_INCREMENT )
       pcfg->d_refresh = GAPC_REFRESH_INCREMENT;
      
  i_x = (guint)(gdouble)(pcfg->d_refresh * 1.04) * 1000.0;

  pcfg->i_timer_ids[GAPC_TIMER_AUTO] = 
                    g_timeout_add (i_x, gapc_cb_auto_refresh, pcfg);

  pcfg->i_timer_ids[GAPC_TIMER_DEDICATED] = 
                    g_timeout_add (200, gapc_cb_dedicated_one_time_refresh, pcfg);

  return (window);
}

/*
 * Manage about dialog destruction 
 */
static void gapc_cb_about_dialog_response  (GtkDialog *dialog,  gint arg1,  gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  if ( arg1 != GTK_RESPONSE_NONE )   /* not a regular destroy event for a dialog */
	 gtk_widget_destroy ( GTK_WIDGET(pcfg->about) );  

  pcfg->about = NULL;
  
 return;	
}


/*
 * Cleanup routine called when the applet widget is destroyed
 *  - if multiple instances are active, who knows what lives on
 *    because they share the same process and memory space.
 *    And can exist even without their controllers.
 *    So we stop the timers and threads and try to destroy
 *    any visual clues that this instance was present.
 */
static void
gapc_cb_applet_destroy (GtkObject *object, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;
  guint i_x = 0;


  pcfg->b_run = FALSE;
  pcfg->b_refresh_button = TRUE;

  for ( i_x = 0; i_x < GAPC_N_TIMERS ; i_x++ )
     	g_source_remove ( pcfg->i_timer_ids[i_x] );

  for ( i_x = 0; i_x < GAPC_N_ICONS ; i_x++ )
     	g_object_unref ( pcfg->my_icons[i_x] );

  gtk_widget_destroy ( GTK_WIDGET(pcfg->window) );

  if ( pcfg->about != NULL )
       gtk_widget_destroy ( GTK_WIDGET(pcfg->about) );  

  g_thread_join (pcfg->tid_refresh);

  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_status[i_x])
    {
      g_free (pcfg->pach_status[i_x]);
      pcfg->pach_status[i_x] = NULL;      
    }
  for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    if (pcfg->pach_events[i_x])
    {
      g_free (pcfg->pach_events[i_x]);
      pcfg->pach_events[i_x] = NULL;
    }      

 return;
}

/* 
 * Menu Callback : Display main info window 
 */
static void
gapc_cb_menu_main_window (BonoboUIComponent * uic, gpointer gp, const gchar * verbname)
{
  PGAPC_CONFIG pcfg = gp;


  g_return_if_fail (pcfg != NULL);
  g_return_if_fail (GTK_WINDOW (pcfg->window));

  pcfg->b_window_visible = GTK_WIDGET_VISIBLE (pcfg->window);
  if (pcfg->b_window_visible)
    return;

  pcfg->b_window_visible = TRUE;
  gtk_widget_show_all (GTK_WIDGET (pcfg->window));

  return;
}

/* 
 * callback for panel icon itself  
 */
static gboolean
gapc_cb_toggle_main_window (GtkWidget * widget, GdkEventButton * event, gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  if (event->button == 1)
    {
      pcfg->b_window_visible = GTK_WIDGET_VISIBLE (pcfg->window);
      pcfg->b_window_visible = !pcfg->b_window_visible;

      if (pcfg->b_window_visible)
		  gtk_widget_show_all (GTK_WIDGET (pcfg->window));
      else
		  gtk_widget_hide_all (GTK_WIDGET (pcfg->window));

      return TRUE;
    }

  return FALSE;
}

/* 
 * Menu Callback : Display About window
 */
static void
gapc_cb_menu_about (BonoboUIComponent * uic, gpointer gp, const gchar * verbname)
{
  PGAPC_CONFIG pcfg = gp;
  GtkDialog *window = NULL;
  GtkWidget *label  = NULL, *button = NULL, *frame = NULL, *mbox  = NULL;
  GtkWidget *hbox   = NULL, *vbox   = NULL, *image = NULL;
  gchar *about_text = NULL;
  gchar *about_msg = NULL;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled = NULL;

  about_text = g_strdup_printf ("<b><big>%s Version %s</big></b>", 
    		   		GAPC_GROUP_TITLE, GAPC_VERSION);
  about_msg = g_strdup_printf (
    "<b>Applet which monitors UPSs managed by the APCUPSD.sourceforge.net package</b>\n"
    "<i>http://gapcmon.sourceforge.net/</i>\n\n" 
    "Copyright (C) 2006 James Scott, Jr.\n"
    "skoona@users.sourceforge.net\n\n" 
    "Released under the GNU Public License\n"
    "%s comes with ABSOLUTELY NO WARRANTY", 
    GAPC_GROUP_TITLE );

  pcfg->about = window = GTK_DIALOG (gtk_dialog_new ());
  	gtk_window_set_title (GTK_WINDOW (window), _("About gpanel_apcmon"));
  	gtk_window_set_type_hint ( GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG );
  	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
  	gtk_container_set_border_width (GTK_CONTAINER (window), 6);
  	gtk_window_set_icon (GTK_WINDOW(window), pcfg->my_icons[GAPC_ICON_DEFAULT]);
  
  frame = gtk_frame_new (NULL);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  	gtk_box_pack_start (GTK_BOX (window->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  	gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  frame = gtk_frame_new (NULL);
  	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
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
  	gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
  	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  mbox = gtk_vbox_new (FALSE, 0);
  	gtk_box_pack_start (GTK_BOX (vbox), mbox, TRUE, TRUE, 0);
  label = gtk_label_new (about_msg);
  	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  	gtk_misc_set_alignment ((GtkMisc *) label, 0.5, 0.5);
  	gtk_box_pack_start (GTK_BOX (mbox), label, TRUE, TRUE, 0);

  button = gtk_dialog_add_button (window, GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);
  	g_signal_connect (window, "response", G_CALLBACK (gapc_cb_about_dialog_response), pcfg);

  gtk_widget_show_all (GTK_WIDGET (window));

  g_free (about_text);
  g_free (about_msg);

  return;
}

/*
 * One of two callbacks the panel object support
 */
static void
gapc_cb_applet_change_size (PanelApplet * applet, gint size, PGAPC_CONFIG pcfg)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *scaled;


  if (pcfg->i_icon_index >= GAPC_N_ICONS)
    pcfg->i_icon_index = GAPC_ICON_DEFAULT;

  pixbuf = pcfg->my_icons[pcfg->i_icon_index];
  if (pixbuf)
    {
      scaled = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);

      gtk_image_set_from_pixbuf (GTK_IMAGE (pcfg->image), scaled);
      gtk_widget_show (pcfg->image);
      
      gdk_pixbuf_unref ( scaled );      
    }

  return;
}

/*
 * Main entry point - pointed ot by PANEL_APPLET_BONOBO_FACTORY statement
 * Return TRUE is you want to initialize the panel program, FALSE otherwise
 */
static gboolean
gapc_applet_populate (PanelApplet * applet, PGAPC_CONFIG pcfg)
{
/*  PanelAppletOrient orientation = NULL; */	
  const BonoboUIVerb gapc_applet_menu_verbs[] = {
    BONOBO_UI_UNSAFE_VERB ("gp_apcmon_main_window", gapc_cb_menu_main_window),
    BONOBO_UI_UNSAFE_VERB ("gp_apcmon_about", gapc_cb_menu_about),
    BONOBO_UI_VERB_END		/* Popup menu on the applet */
  };

  const char gp_apcmon_context_menu_xml[] =
    "<popup name=\"button3\">\n"
    "   <menuitem name=\"Info Panel Item\" verb=\"gp_apcmon_main_window\" _label=\"_Open ...\"\n"
    "             pixtype=\"stock\" pixname=\"gnome-stock-open\"/>\n"
    "   <menuitem name=\"About Item\" verb=\"gp_apcmon_about\" _label=\"_About ...\"\n"
    "             pixtype=\"stock\" pixname=\"gnome-stock-about\"/>\n" "</popup>\n";

  if (!g_thread_supported ())
    g_thread_init (NULL);

  if (!gnome_vfs_initialized ())
    gnome_vfs_init ();

  pcfg->gm_update = g_mutex_new ();

  gapc_load_icons ( pcfg );

  pcfg->b_window_visible = FALSE;
  pcfg->applet = GTK_WIDGET (applet);
  pcfg->b_network_changed = TRUE;
  
  pcfg->frame = gtk_frame_new (NULL);
  	gtk_frame_set_shadow_type (GTK_FRAME (pcfg->frame), GTK_SHADOW_NONE);
  	gtk_container_add (GTK_CONTAINER (applet), pcfg->frame);
  	g_signal_connect (applet, "button_press_event", G_CALLBACK (gapc_cb_toggle_main_window), pcfg);

  pcfg->image = gtk_image_new ();
  	gtk_container_add (GTK_CONTAINER (pcfg->frame), pcfg->image);

  pcfg->tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (pcfg->tooltips, GTK_WIDGET (applet), _("UPS Initializing..."), NULL);

  panel_applet_setup_menu (PANEL_APPLET (applet),
			   			   gp_apcmon_context_menu_xml, 
			   			   gapc_applet_menu_verbs, 
			   			   pcfg);

  g_signal_connect (applet, "change_size", G_CALLBACK (gapc_cb_applet_change_size), pcfg);

  g_signal_connect (applet, "destroy", G_CALLBACK (gapc_cb_applet_destroy), pcfg); 

  gtk_widget_show_all (GTK_WIDGET (pcfg->frame));
  gtk_widget_show (GTK_WIDGET (applet));  
  gapc_cb_applet_change_size (PANEL_APPLET (applet), panel_applet_get_size (applet), pcfg);

  pcfg->window = NULL;
  pcfg->window = gapc_main_win (pcfg);
  pcfg->b_network_changed = TRUE;  

  return TRUE;
}

/*
 * Main entry point - pointed ot by PANEL_APPLET_BONOBO_FACTORY statement
 */
static gboolean
gapc_applet_factory (PanelApplet * applet, const gchar * iid, gpointer data)
{
  PGAPC_CONFIG pcfg = NULL;

  if ( g_str_equal (iid, "OAFIID:gpanel_apcmonApplet") )
    {
      guint i_instance_count = 0;
      
      i_instance_count = bonobo_control_life_get_count ();
      pcfg = g_new0 (GAPC_CONFIG, 1);
      pcfg->cb_id = i_instance_count;

      return ( gapc_applet_populate (applet, pcfg) );
    }
     
  return FALSE;

}

/*
 * GNOME connection to GNOME_gpanel_apcmon.server file
 * and pointer to panels main entry point
 */
PANEL_APPLET_BONOBO_FACTORY ("OAFIID:gpanel_apcmonApplet_Factory",
				 PANEL_TYPE_APPLET,
			     "gpanel_apcmonApplet", 
			     GAPC_VERSION, 
			     gapc_applet_factory, 
			     NULL)


