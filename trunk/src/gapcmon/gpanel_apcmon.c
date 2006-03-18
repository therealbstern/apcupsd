/* gpanel_apcmon.c       serial-0057-3 *****************************************
  Gnome applet for monitoring the apcupsd.sourceforge.net package.
  Copyright (C) 2006 James Scott, Jr. <skoona@users.sourceforge.net>

 * Network utility routines, adapted from package: apcupsd, file 
 * ./src/lib/apclibnis.c
 *

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

 * Program Structure
 * ------------------------ -------------------------------------------------
 * PanelApplet				GtkMain Thread ----
 * - GtkFrame				Toplevel
 * 	 - Gtk[v|h]Box			Monitor Container depends on Orientation
 * 	   - GtkEventBox		Monitor frame for tooltip\icon\mouse-clicks
 * 		 - GtkImage	Monitor State Icon Display
 *
 * 		 - g_timeout_add	Individual gTimers to perform work and manage
 * 							communication with background thread.
 *
 * - GtkWindow				Monitor Information window featuring a tabbed
 *                          notebook.
 * 	 - GtkNotebook			with four pages; summary chart, detailed, eventlog,
 *                          and status log pages.  Pops up in response to
 *                          button-click on	monitor icon eventbox.
 *
 * 	  - nbPage HISTORY		A GtkGLGraph histogram line chart of
 * 							LINEV, LOADPCT, BCHARGE, CUMONBATT, TIMELEFT
 * 	  - nbPage DETAILS		Show grouped results from the status output.
 * 	  - nbPage EVENTS		Show the current events log
 * 	  - nbPage STATUS		Show the current status log, same as APCACCESS
 *
 * - GtkDialog				Toplevel - About dialog  -- From panel menu
 * - GtkDialog				Toplevel - Preferences dialog -- From panel menu
 * 	 - GtkScrolledWindow	Container for Preference selection
 * 	   - GtkTreeView		Multi-colum display and interaction of configuration
 * 							values that control the overall applet.
 *
 * - GConfClient			Tied to Preferences dialog GROUP key
 * 							Produces direct change on application state and
 * 							Operation via controlled changes to config values
 *
 * - GThread				Network Communication via socket io using GnomeVFS.
 * 							Communication to interface gtkthread through
 *  						a GAsyncQueue
 *
 *  CURRENT KEY IS:
 *  key \schemas\apps\panel\applets\gpanel_apcmon\prefs\instance_x
 *              \apps\panel\applets\gpanel_apcmon\prefs\instance_x\monitor_x\key
 *  Where x is the instance number 0-5 for instances and 0-7 for monitors:
 *  max monitors=8
 *  Where key is the actual keyname like enabled, host_name, port_name, or
 *  refresh_interval
 *
 *  Should consider this for desktops \schemas\apps\gp_apcmon\prefs\key
 * 				                              \apps\gp_apcmon\prefs\key
 *  Filename: gpanel_apcmon.schemas
 *  Install Directory: \etc\gconf\schemas\   or $(sysconfdir)\gconf\schemas\
 *  gconf-devel may be needed
 *  gconftool-2 --install-schema-file=gpanel_apcmon.schemas
 *
*/


#include <gtk/gtk.h>
#include <panel-applet.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>
#include "gapcmon_gtkglgraph.h"

#define GAPC_PROG_NAME    "gpanel_apcmon"
#define GAPC_WINDOW_TITLE "gpanel_apcmon: UPS Information"
#define GAPC_GROUP_TITLE  "<i>gpanel_apcmon: UPS Information Panels</i>"
#define GAPC_PROG_IID "OAFIID:gpanel_apcmonApplet"
#define GAPC_PROG_IID_FACTORY  "OAFIID:gpanel_apcmonApplet_Factory"

#ifndef VERSION
#define GAPC_VERSION "0.5.7-0"
#else
#define GAPC_VERSION VERSION
#endif

#define GAPC_GROUP_KEY  "/apps/panel/applets/gpanel_apcmon/prefs/instance_%d"
#define GAPC_ENABLE_KEY "/apps/panel/applets/gpanel_apcmon/prefs/instance_%d/monitor_%d/enabled"
#define GAPC_HOST_KEY   "/apps/panel/applets/gpanel_apcmon/prefs/instance_%d/monitor_%d/host_name"
#define GAPC_PORT_KEY   "/apps/panel/applets/gpanel_apcmon/prefs/instance_%d/monitor_%d/port_number"
#define GAPC_REFRESH_KEY "/apps/panel/applets/gpanel_apcmon/prefs/instance_%d/monitor_%d/refresh_interval"

#define GAPC_MONITOR_MAX 8      /* maximum num of monitor supported  */
#define GAPC_MAX_ARRAY 256      /* for string and temps */
#define GAPC_MAX_TEXT 256
#define GAPC_MIN_INCREMENT     1.0 /* Minimum refresh cycle seconds */
#define GAPC_THREAD_CYCLE      15000000 /* 15 sec Network update timer 1M=1 */
#define GAPC_REFRESH_INCREMENT 30000 /* 30 sec Display update timer 1K=1 */
#define GAPC_REFRESH_FACTOR_1M 250000 /* 1-4th of usecs for network threads */
#define GAPC_REFRESH_FACTOR_1K 1000 /* secs for visual refresh           */
#define GAPC_HISTORY_CHART_PTS 40 /* Num of data points in each series */
#define GAPC_HISTORY_CHART_SERIES 5 /* Num of data series       */
#define GAPC_HISTORY_CHART_FACTOR_XINC 30 /* Num refreshes per collection  */
#define GAPC_HOST_VALUE_DEFAULT "localhost"
#define GAPC_PORT_VALUE_DEFAULT 3551

#define GAPC_LINEGRAPH_XMAX GAPC_HISTORY_CHART_PTS
#define GAPC_LINEGRAPH_YMAX 110
#define GAPC_MAX_SERIES GAPC_HISTORY_CHART_SERIES

typedef enum _State_Icons_IDs
{
  GAPC_ICON_ONLINE,
  GAPC_ICON_ONBATT,
  GAPC_ICON_CHARGING,
  GAPC_ICON_DEFAULT,
  GAPC_ICON_UNPLUGGED,  
  GAPC_N_ICONS
} GAPC_IconType;

typedef enum _Timer_IDs
{
  GAPC_TIMER_AUTO,
  GAPC_TIMER_DEDICATED,
  GAPC_TIMER_CONTROL,
  GAPC_N_TIMERS
} GAPC_TimerType;

typedef enum _Prefs_Store_IDs
{
  COLUMN_MONITOR,
  COLUMN_HOST,
  COLUMN_PORT,
  COLUMN_REFRESH,
  COLUMN_ENABLED,
  GAPC_N_COLUMNS
} GAPC_PrefType;

/* Control structure for TreeView columns and callbacks */
typedef struct _Prefs_Column_Data
{
  guint           cb_instance_num; /* This is REQUIRED TO BE 1ST in struct */
  guint           cb_monitor_num; /* monitor number 0-based */
  guint           i_col_num;
  GConfClient    *client;
  GtkTreeModel   *prefs_model;  /* GtkListStore */

} GAPC_COLUMN  , *PGAPC_COLUMN;

typedef struct _GAPC_H_CHART
{
  gdouble         d_value;
  gboolean        b_center_text;
  gchar           c_text[GAPC_MAX_TEXT];
  GdkRectangle    rect;
} GAPC_BAR_H   , *PGAPC_BAR_H;

typedef struct _GAPC_SUM_SQUARES
{
  gint            point_count;

  gdouble         this_point;
  gdouble         this_answer;

  gdouble         last_point;
  gdouble         last_answer;

  gdouble         answer_summ;
  gdouble         point_min;
  gdouble         point_max;

  GMutex         *gm_graph;     /* Control mutex  for graphics filter */
} GAPC_SUMS    , *PGAPC_SUMS;

/* * Control structure for GtkExtra Charts in Information Window */
typedef struct _History_Page_Data
{
  guint           cb_instance_num; /* This is REQUIRED TO BE 1ST in struct   */
  guint           cb_monitor_num; /* monitor number 0-based */
  gpointer        *gp;              /* ptr back to the instance */
  GHashTable    **pht_Status;   /* hashtable holding status key=values COPY */
  GHashTable    **pht_Widgets;  /* hashtable holding wdiget ptrs  COPY */
  GtkGLGraph     *glg;          /* GtkGLGraph widget */

  gchar          *xlabel;
  gchar          *ylabel;
  gchar          *zlabel;

  gdouble         xmin;
  gdouble         xmax;
  gint32          xmajor_steps;
  gint32          xminor_steps;
  gint8           xprecision;
  gdouble         ymin;
  gdouble         ymax;
  gint32          ymajor_steps;
  gint32          yminor_steps;
  gint8           yprecision;

  gchar           ch_label_color[GAPC_HISTORY_CHART_SERIES+4][GAPC_MAX_ARRAY];
  gchar           ch_label_legend[GAPC_HISTORY_CHART_SERIES+4][GAPC_MAX_ARRAY];

  gdouble         d_xinc;       /* base refresh increment for scaling x legend */
  gint32          timer_id;     /* timer id for graph update */

  GAPC_SUMS       sq[GAPC_HISTORY_CHART_SERIES + 4]; /* regression work area */

} GAPC_HISTORY , *PGAPC_HISTORY;

/* * Control structure per active monitor icon in panel  */
typedef struct _Monitor_Instance_Data
{
  guint           cb_instance_num; /* This is REQUIRED TO BE 1ST in struct   */
  guint           cb_monitor_num; /* monitor number 0-based */

  PanelApplet    *applet;
  GtkTreeModel   *prefs_model;  /* GtkListStore */
  GtkWidget      *evbox;
  GtkWidget      *image;
  GtkTooltips    *tooltips;
  GConfClient    *client;

  GtkWidget      *window;       /* information window or dialog */
  gboolean        b_window_visible; /* TRUE flags a window as visible */
  guint           i_info_context; /* StatusBar Context */

  gchar          *pach_status[GAPC_MAX_ARRAY]; /* Holds line of status text */
  gchar          *pach_events[GAPC_MAX_ARRAY]; /* Holds line of event text */
  GHashTable     *pht_Status;   /* hashtable holding status key=values */
  GHashTable     *pht_Widgets;  /* hashtable holding wdiget ptrs  */

  guint           tid_automatic_refresh;
  guint           i_icon_index;
  gint            i_old_icon_index;

  GMutex         *gm_update;    /* Control mutex  for hashtables and pcfg */
  GnomeVFSAddress *gapc_last_address;

  gboolean        b_run;
  gboolean        b_network_changed; /* TRUE signals a change in host name */
  gboolean        b_timer_control; /* TRUE signals change in refresh interval */
  gboolean        b_refresh_button; /* Flag to thread to immediately update */
  gboolean        b_data_available; /* Flag indicating that netowrk is online */

  gboolean        cb_enabled;   /* id state of each monitor */
  gchar          *pch_host;
  gint            i_port;
  gdouble         d_refresh;
  GAPC_HISTORY    phs;          /* history chart structure for history notebook page */
  gpointer       *gp;           /* assumed to point to pcfg */

} GAPC_INSTANCE, *PGAPC_INSTANCE;

/* * Control structure for root panel object -- this is the anchor */
typedef struct _System_Control_Data
{
  guint           cb_instance_num; /* This is REQUIRED TO BE 1ST in struct  */
  guint           cb_monitors;  /* number of enabled monitors 0-based */
  gboolean        b_run;

  GThread        *tid_thread_qwork; /* Background Thread */
  GAsyncQueue    *q_network;

  PanelApplet    *applet;
  GtkWidget      *ppframe;
  GtkWidget      *ppbox;
  GtkWidget      *about;  
  PanelAppletOrient orientation;

  GtkWidget      *prefs_dlg;
  GtkTreeModel   *prefs_model;  /* GtkListStore */
  GtkTreeView    *prefs_treeview;

  GdkPixbuf      *my_icons[GAPC_N_ICONS];

  GConfClient    *client;
  guint           i_group_id;   /* GCONF notify ids */
  GAPC_INSTANCE   ppi[GAPC_MONITOR_MAX + 1]; /* Instance Data Structs */

} GAPC_CONFIG  , *PGAPC_CONFIG;

static gboolean gapc_applet_factory (PanelApplet * applet, const gchar * iid,
                                     gpointer data);
static gboolean gapc_applet_create (PanelApplet * applet, PGAPC_CONFIG pcfg);
static GtkWidget *gapc_applet_interface_create (PGAPC_CONFIG pcfg);

static gint     gapc_util_change_icons (PGAPC_INSTANCE ppi, gint size);
static gboolean gapc_util_load_icons (PGAPC_CONFIG pcfg);
static void     gapc_util_log_app_error (gchar * pch_func, gchar * pch_topic,
                                         gchar * pch_emsg);
static void     gapc_util_text_view_append (GtkWidget * view, gchar * pch);
static void     gapc_util_text_view_prepend (GtkWidget * view, gchar * pch);
static void     gapc_util_text_view_clear_buffer (GtkWidget * view);
static GtkWidget *gapc_util_barchart_create (PGAPC_INSTANCE ppi,
                                             GtkWidget * vbox,
                                             gchar * pch_hbar_name,
                                             gdouble d_percent,
                                             gchar * pch_text);

static gboolean gapc_preferences_init (PGAPC_CONFIG pcfg);
static GtkWidget *gapc_preferences_dialog_create (PGAPC_CONFIG pcfg);
static GtkTreeView *gapc_preferences_dialog_view (PGAPC_CONFIG pcfg,
                                                  GtkWidget * vbox);
static PGAPC_COLUMN gapc_preferences_column_data_init (PGAPC_CONFIG pcfg,
                                                       GAPC_PrefType col_num);
static gboolean gapc_preferences_save (PGAPC_INSTANCE ppi);
static GtkTreeModel *gapc_preferences_model_data_init (PGAPC_CONFIG pcfg);
static gboolean gapc_preferences_model_data_load (PGAPC_INSTANCE ppi);
static gint     gapc_preferences_model_enable_one (PGAPC_CONFIG pcfg);

static gboolean cb_monitor_dedicated_one_time_refresh (PGAPC_INSTANCE ppi);
static gint     gapc_monitor_interface_count_enabled (GtkTreeModel * model);
static gboolean gapc_monitor_interface_create (PGAPC_INSTANCE ppi);
static gboolean gapc_monitor_interface_remove (PGAPC_INSTANCE ppi);
static gint     gapc_monitor_update (PGAPC_INSTANCE ppi);
static gboolean cb_util_barchart_handle_exposed (GtkWidget * widget,
                                                 GdkEventExpose * event,
                                                 gpointer data);
static gboolean cb_monitor_handle_icon_clicked (GtkWidget * widget,
                                                GdkEventButton * event,
                                                PGAPC_INSTANCE ppi);
static gboolean cb_monitor_automatic_refresh (PGAPC_INSTANCE ppi);
static gboolean cb_monitor_refresh_control (PGAPC_INSTANCE ppi);
static void     cb_monitor_preferences_changed (GConfClient * client,
                                                guint cnxn_id,
                                                GConfEntry * entry,
                                                PGAPC_CONFIG pcfg);

static void     cb_applet_destroy (GtkObject * object, PGAPC_CONFIG pcfg);
static void     cb_applet_change_orientation (PanelApplet * applet, guint arg1,
                                              PGAPC_CONFIG pcfg);
static void     cb_applet_change_size (PanelApplet * applet, gint size,
                                       PGAPC_CONFIG pcfg);
static void     cb_applet_menu_verbs (BonoboUIComponent * uic,
                                      PGAPC_CONFIG pcfg,
                                      const gchar * verbname);

static void     cb_preferences_dialog_response (GtkDialog * dialog, gint arg1,
                                                PGAPC_CONFIG pcfg);
static void     cb_preferences_dialog_destroy (GtkWidget * widget, gpointer gp);
static void     cb_preferences_handle_float_format (GtkTreeViewColumn * col,
                                                    GtkCellRenderer * renderer,
                                                    GtkTreeModel * model,
                                                    GtkTreeIter * iter,
                                                    gpointer gp);
static void     cb_preferences_handle_cell_edited (GtkCellRendererText * cell,
                                                   gchar * path_string,
                                                   gchar * pch_new,
                                                   PGAPC_COLUMN pcolumn);
static void     cb_preferences_handle_cell_toggled (GtkCellRendererToggle *
                                                    cell, gchar * path_str,
                                                    gpointer gp);

/* * Main information window routines */
static void     cb_information_window_button_refresh (GtkButton * button,
                                                      PGAPC_INSTANCE ppi);
static void     cb_information_window_button_quit (GtkButton * button,
                                                   PGAPC_INSTANCE ppi);
static gboolean cb_information_window_delete_event (GtkWidget * window,
                                                    GdkEvent * event,
                                                    PGAPC_INSTANCE ppi);
static void     cb_information_window_destroy (GtkWidget * window,
                                               PGAPC_INSTANCE ppi);
static GtkWidget *gapc_information_window_create (PGAPC_INSTANCE ppi);

static gint     gapc_information_information_page (PGAPC_INSTANCE ppi,
                                                   GtkWidget * notebook);
static gint     gapc_information_text_report_page (PGAPC_INSTANCE ppi,
                                                   GtkWidget * notebook,
                                                   gchar * pchTitle,
                                                   gchar * pchKey);

/*
 * Network Routines
*/
static gpointer *gapc_net_thread_qwork (PGAPC_CONFIG pcfg);
static void     gapc_net_log_error (gchar * pch_func, gchar * pch_topic,
                                    GnomeVFSResult result);
static gint     gapc_net_read_nbytes (GnomeVFSSocket * psocket, gchar * ptr,
                                      gint nbytes);
static gint     gapc_net_write_nbytes (GnomeVFSSocket * psocket, gchar * ptr,
                                       gint nbytes);
static gint     gapc_net_recv (GnomeVFSSocket * psocket, gchar * buff,
                               gint maxlen);
static gint     gapc_net_send (GnomeVFSSocket * v_socket, gchar * buff,
                               gint len);
static GnomeVFSInetConnection *gapc_net_open (gchar * pch_host, gint i_port,
                                              gboolean * b_changed,
                                              GnomeVFSAddress ** address);
static void     gapc_net_close (GnomeVFSInetConnection * connection,
                                GnomeVFSSocket * v_socket);
static gint     gapc_net_transaction_service (PGAPC_INSTANCE ppi,
                                              gchar * cp_cmd, gchar ** pch);
static gint     gapc_util_update_hashtable (PGAPC_INSTANCE ppi,
                                            gchar * pch_unparsed);
static gboolean gapc_monitor_update_tooltip_msg (PGAPC_INSTANCE ppi);
static gdouble  gapc_util_point_filter_reset (PGAPC_SUMS sq);
static gdouble  gapc_util_point_filter_set (PGAPC_SUMS sq, gdouble this_point);

/* **************************************************************
 *  GtkGLGraph Graphing Routines
 * 
 * **************************************************************
*/

static gboolean gapc_util_line_chart_create (PGAPC_HISTORY pg, GtkWidget * box);
static gboolean gapc_util_line_chart_ds_init (PGAPC_HISTORY pg, gint i_series);
static gboolean gapc_util_line_chart_toggle_legend (PGAPC_HISTORY pg);
static gboolean cb_util_line_chart_refresh (PGAPC_HISTORY pg);
static gboolean cb_util_line_chart_toggle_legend (GtkWidget * widget,
                                                  GdkEventButton * event,
                                                  PGAPC_HISTORY pg);

static gint     gapc_information_history_page (PGAPC_INSTANCE ppi,
                                               GtkWidget * notebook);
static void gapc_applet_interface_about_dlg (PGAPC_CONFIG pcfg );
static void cb_applet_interface_about_dialog_response (GtkDialog *dialog,  
                                                       gint arg1,  gpointer gp);
/*
 * Manage about dialog destruction 
 */
static void cb_applet_interface_about_dialog_response (GtkDialog *dialog,  
                                                       gint arg1,  gpointer gp)
{
  PGAPC_CONFIG pcfg = gp;

  if ( arg1 != GTK_RESPONSE_NONE )   /* not a regular destroy event for a dialog */
     gtk_widget_destroy ( GTK_WIDGET(pcfg->about) );  

  pcfg->about = NULL;
  
 return;    
}

/* 
 * Display About window
 */
static void gapc_applet_interface_about_dlg (PGAPC_CONFIG pcfg )
{
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
    "<b>Applet which monitors UPSs managed by the "
    "APCUPSD.sourceforge.net package</b>\n"
    "<i>http://gapcmon.sourceforge.net/</i>\n\n" 
    "Copyright (C) 2006 James Scott, Jr.\n"
    "skoona@users.sourceforge.net\n\n" 
    "Released under the GNU Public License\n"
    "%s comes with ABSOLUTELY NO WARRANTY", 
    GAPC_GROUP_TITLE );

    window = GTK_DIALOG (gtk_dialog_new ());
    pcfg->about = GTK_WIDGET (window);
    gtk_window_set_title (GTK_WINDOW (window), _("About gpanel_apcmon"));
    gtk_window_set_type_hint ( GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
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
    g_signal_connect (window, "response", 
                      G_CALLBACK (cb_applet_interface_about_dialog_response), 
                      pcfg);

  gtk_widget_show_all (GTK_WIDGET (window));

  g_free (about_text);
  g_free (about_msg);

  return;
}


/*
 * return the answer and reset the internal controls to zero
*/
static gdouble gapc_util_point_filter_reset (PGAPC_SUMS sq)
{
  gdouble         d_the_final_answer = 0.0;

  g_mutex_lock (sq->gm_graph);

  d_the_final_answer = sq->last_answer;

  sq->point_count = 0;

  sq->this_point = 0.0;
  sq->last_point = 0.0;

  sq->this_answer = 0.0;
  sq->last_answer = 0.0;

  sq->answer_summ = 0.0;
  sq->point_min = 0.0;
  sq->point_max = 0.0;

  g_mutex_unlock (sq->gm_graph);

  return (d_the_final_answer);
}

/*
 * Compute the average of the given data point
*/
static gdouble gapc_util_point_filter_set (PGAPC_SUMS sq, gdouble this_point)
{
  g_mutex_lock (sq->gm_graph);

  sq->this_point = this_point;
  sq->point_count++;

  /* some calc here */
  sq->answer_summ += (sq->this_point * 100);
  sq->this_answer = sq->answer_summ / sq->point_count;

  sq->last_point = sq->this_point;
  sq->last_answer = sq->this_answer;

  if (sq->point_min > sq->this_point)
    sq->point_min = sq->this_point;
  if (sq->point_max < sq->this_point)
    sq->point_max = sq->this_point;

  g_mutex_unlock (sq->gm_graph);

  return (sq->this_answer);
}

/*
 * Manage the state icon in the panel and the associated tooltip
 * Composes the expanded tooltip message
 */
static gboolean gapc_monitor_update_tooltip_msg (PGAPC_INSTANCE ppi)
{
  gchar          *pchx = NULL, *pmsg = NULL, *ptitle = NULL, *pch5 = NULL;
  gchar          *pch1 = NULL, *pch2 = NULL, *pch3 = NULL, *pch4 = NULL;
  gchar          *pch6 = NULL, *pch7 = NULL, *pch8 = NULL, *pch9 = NULL;
  gchar          *pchb = NULL, *pchc = NULL, *pchd = NULL, *pche = NULL;
  gchar          *pcha = NULL;
  GtkWidget      *w = NULL;
  gdouble         d_value = 0.0;
  gboolean        b_flag = FALSE;
  gint            i_series = 0;

  g_return_val_if_fail (ppi != NULL, TRUE);

  if (!g_mutex_trylock (ppi->gm_update))
    return FALSE;               /* thread must be busy */

  ppi->i_icon_index = GAPC_ICON_ONLINE;

  pch1 = g_hash_table_lookup (ppi->pht_Status, "UPSNAME");
  pch2 = g_hash_table_lookup (ppi->pht_Status, "HOSTNAME");
  if (pch2 == NULL)
  {
    pch2 = ppi->pch_host;
  }
  pch3 = g_hash_table_lookup (ppi->pht_Status, "STATUS");
  if ( pch3 == NULL )
  {
       pch3 = "COMMLOST";
  }
  pch4 = g_hash_table_lookup (ppi->pht_Status, "NUMXFERS");
  pch5 = g_hash_table_lookup (ppi->pht_Status, "XONBATT");
  pch6 = g_hash_table_lookup (ppi->pht_Status, "LINEV");
  pch7 = g_hash_table_lookup (ppi->pht_Status, "BCHARGE");
  if ( pch7 == NULL )
  {
       pch7 = "0.0";
  }
  pch8 = g_hash_table_lookup (ppi->pht_Status, "LOADPCT");
  pch9 = g_hash_table_lookup (ppi->pht_Status, "TIMELEFT");
  pcha = g_hash_table_lookup (ppi->pht_Status, "VERSION");
  pchb = g_hash_table_lookup (ppi->pht_Status, "STARTTIME");
  pchc = g_hash_table_lookup (ppi->pht_Status, "MODEL");
  pchd = g_hash_table_lookup (ppi->pht_Status, "UPSMODE");
  pche = g_hash_table_lookup (ppi->pht_Status, "CABLE");

  if (ppi->b_data_available)
  {
    d_value = g_strtod (pch7, NULL);
    pchx = NULL;
    if (g_strrstr (pch3, "COMMLOST") != NULL)
    {
        pchx = " cable un-plugged...";
        ppi->i_icon_index = GAPC_ICON_UNPLUGGED;
        b_flag = TRUE;            
    } else if ((d_value < 99.0) && (g_strrstr (pch3, "LINE") != NULL))
           {
                pchx = " and charging...";
                ppi->i_icon_index = GAPC_ICON_CHARGING;
           } else if (g_strrstr (pch3, "BATT") != NULL)
                  {
                      pchx = " on battery...";
                      ppi->i_icon_index = GAPC_ICON_ONBATT;
                  }
  }
  else
  {
    b_flag = TRUE;      
    if (g_strrstr (pch3, "COMMLOST") != NULL)
    {
      pchx = " cable un-plugged...";
      ppi->i_icon_index = GAPC_ICON_UNPLUGGED;
    }
    else
    {
        pchx = "NIS network error...";
        pch3 = " ";
        ppi->i_icon_index = GAPC_ICON_DEFAULT;
    }
    for (i_series = 0; i_series < GAPC_HISTORY_CHART_SERIES; i_series++)
    {
         gapc_util_point_filter_set (&(ppi->phs.sq[i_series]), 0.0);         
    }
  }

  if (b_flag)
  {
    ptitle = g_strdup_printf ("<span foreground=\"red\" size=\"large\">"
                              "Monitored UPS %s on %s is %s%s" "</span>",
                              (pch1 != NULL) ? pch1 : "unknown",
                              (pch2 != NULL) ? pch2 : "unknown",
                              (pch3 != NULL) ? pch3 : "n/a",
                              (pchx != NULL) ? pchx : " ");
  }
  else
  {
    ptitle = g_strdup_printf ("<span foreground=\"blue\" size=\"large\">"
                              "Monitored UPS %s on %s is %s%s" "</span>",
                              (pch1 != NULL) ? pch1 : "unknown",
                              (pch2 != NULL) ? pch2 : "unknown",
                              (pch3 != NULL) ? pch3 : "n/a",
                              (pchx != NULL) ? pchx : " ");
  }

  pmsg = g_strdup_printf ("APCUPSD Monitor(%d:%d)\n" "UPS %s at %s\nis %s%s\n"
                          "Refresh occurs every %3.1f seconds\n"
                          "----------------------------------------------------------\n"
                          "%s Outage[s], Last one on %s\n" "%s Utility VAC\n"
                          "%s Battery Charge\n" "%s UPS Load\n" "%s Remaining\n"
                          "----------------------------------------------------------\n"
                          "Build: %s\n" "Started: %s\n"
                          "----------------------------------------------------------\n"
                          "%s UPS Model\n" "%s Mode \n" "%s Cable",
                          ppi->cb_instance_num, ppi->cb_monitor_num + 1,
                          (pch1 != NULL) ? pch1 : "unknown",
                          (pch2 != NULL) ? pch2 : "unknown",
                          (pch3 != NULL) ? pch3 : "n/a",
                          (pchx != NULL) ? pchx : " ", ppi->d_refresh,
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

  if (ppi->tooltips != NULL)
  {
    gtk_tooltips_set_tip (ppi->tooltips, GTK_WIDGET (ppi->evbox), pmsg, NULL);
  }
  else if (ppi->window != NULL) /* this must be gapcmon and not gpanel */
  {
    gtk_window_set_title (GTK_WINDOW (ppi->window), pmsg);
  }

  gapc_util_change_icons (ppi, panel_applet_get_size (ppi->applet));
  
  if ( (w = g_hash_table_lookup (ppi->pht_Widgets, "TitleStatus")) ) 
  { /* may not be created yet */
    gtk_label_set_markup (GTK_LABEL (w), ptitle);
  }

  g_free (pmsg);
  g_free (ptitle);

  g_mutex_unlock (ppi->gm_update);

  return b_flag;
}

/*
 *  capture the current network related error values
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this is a background thread)
 */
static void gapc_net_log_error (gchar * pch_func, gchar * pch_topic,
                                GnomeVFSResult result)
{
  gchar          *pch = NULL;

  g_return_if_fail (pch_func != NULL);

  pch = g_strdup_printf ("%s(%s) emsg=%s", pch_func, pch_topic,
                         gnome_vfs_result_to_string (result));

  g_message (pch);

  g_free (pch);

  return;
}

/*
 * Read nbytes from the network. It is possible that the
 * total bytes requires several read requests.
 * returns -1 on error, or number of bytes read
 */
static gint gapc_net_read_nbytes (GnomeVFSSocket * psocket, gchar * ptr,
                                  gint nbytes)
{
  GnomeVFSResult  result = GNOME_VFS_OK;
  GnomeVFSFileSize nread = 0, nleft = 0, nread_total = 0;

  g_return_val_if_fail (ptr, -1);

  nleft = nbytes;

  while (nleft > 0)
  {
    result = gnome_vfs_socket_read (psocket, ptr, nleft, &nread, NULL);

    if (result != GNOME_VFS_OK)
    {
      gapc_net_log_error ("read_nbytes", "read from network failed", result);
      return (-1);              /* error, or EOF */
    }

    nread_total += nread;
    nleft -= nread;
    ptr += nread;
  }

  return (nread_total);         /* return >= 0 */
}

/*
 * Write nbytes to the network.  It may require several writes.
 * returns -1 on error, or number of bytes written
 */
static gint gapc_net_write_nbytes (GnomeVFSSocket * psocket, gchar * ptr,
                                   gint nbytes)
{
  GnomeVFSFileSize nwritten = 0, nwritten_total = 0;
  GnomeVFSResult  result = GNOME_VFS_OK;
  gint            nleft = 0;

  g_return_val_if_fail (ptr, -1);

  nleft = nbytes;
  while (nleft > 0)
  {
    result = gnome_vfs_socket_write (psocket, ptr, nleft, &nwritten, NULL);

    if (result != GNOME_VFS_OK)
    {
      gapc_net_log_error ("write_nbytes", "write to network failed", result);
      return (-1);              /* error */
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
static gint gapc_net_recv (GnomeVFSSocket * psocket, gchar * buff, gint maxlen)
{
  gint            nbytes = 0;
  gshort          pktsiz = 0;

  g_return_val_if_fail (buff, -1);

  /* get data size -- in short */
  nbytes = gapc_net_read_nbytes (psocket, (gchar *) & pktsiz, sizeof (gshort));
  if (nbytes < 0)
    return -1;                  /* assume hard EOF received */

  if (nbytes != sizeof (gshort))
    return -2;

  pktsiz = g_ntohs (pktsiz);    /* decode no. of bytes that follow */
  if (pktsiz > maxlen)
    return -2;

  if (pktsiz == 0)
    return 0;                   /* soft EOF */

  /* now read the actual data */
  nbytes = gapc_net_read_nbytes (psocket, buff, pktsiz);
  if (nbytes < 0)
    return -2;

  if (nbytes != pktsiz)
    return -2;

  return (nbytes);              /* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
static gint gapc_net_send (GnomeVFSSocket * v_socket, gchar * buff, gint len)
{
  gint            rc = 0;
  gshort          pktsiz = 0;

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
static GnomeVFSInetConnection *gapc_net_open (gchar * pch_host, gint i_port,
                                              gboolean * b_changed,
                                              GnomeVFSAddress ** address)
{
  GnomeVFSResult  result = GNOME_VFS_OK;
  GnomeVFSInetConnection *connection = NULL;

  g_return_val_if_fail (pch_host, NULL);

  if (*b_changed)
  {
    result = gnome_vfs_inet_connection_create (&connection, pch_host, i_port,
                                               NULL);
    if (result != GNOME_VFS_OK)
    {
      gapc_net_log_error ("net_open", "create inet connection failed", result);
      return NULL;
    }

    *address = gnome_vfs_inet_connection_get_address (connection);
    if (*address == NULL)
    {
      gapc_net_log_error ("net_open", "get inet connection address failed",
                          result);
      return NULL;
    }

    *b_changed = FALSE;
  }
  else
  {
    result = gnome_vfs_inet_connection_create_from_address (&connection,
                                                            *address, i_port,
                                                            NULL);
    if (result != GNOME_VFS_OK)
    {
      gapc_net_log_error ("net_open", "reuse inet connection failed", result);
      *b_changed = TRUE;
      return NULL;
    }
  }

  return connection;
}

/* Close the network connection */
static void gapc_net_close (GnomeVFSInetConnection * connection,
                            GnomeVFSSocket * v_socket)
{
  gshort          pktsiz = 0;

  if (connection == NULL)
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
static gint gapc_net_transaction_service (PGAPC_INSTANCE ppi, gchar * cp_cmd,
                                          gchar ** pch)
{
  gint            n = 0, iflag = 0, i_port = 0;
  gchar           recvline[GAPC_MAX_TEXT];
  GnomeVFSInetConnection *v_connection = NULL;
  GnomeVFSSocket *v_socket = NULL;

  g_return_val_if_fail (ppi, -1);
  g_return_val_if_fail (ppi->pch_host, -1);

  i_port = ppi->i_port;

  v_connection =
          gapc_net_open (ppi->pch_host, i_port, &ppi->b_network_changed,
                         &ppi->gapc_last_address);
  if (v_connection == NULL)
  {
    ppi->b_network_changed = TRUE;
    return 0;
  }

  v_socket = gnome_vfs_inet_connection_to_socket (v_connection);
  if (v_socket == NULL)
  {
    gapc_net_log_error ("transaction_service",
                        "connect to socket for io failed", GNOME_VFS_OK);
    gnome_vfs_inet_connection_destroy (v_connection, NULL);
    ppi->b_network_changed = TRUE;
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
      gapc_util_update_hashtable (ppi, recvline);

    if (iflag > (GAPC_MAX_ARRAY - 2))
      break;
  }

  gapc_net_close (v_connection, v_socket);

  return iflag;                 /* count of records received */
}

/*
 * Worker thread for network communications.
 */
static gpointer *gapc_net_thread_qwork (PGAPC_CONFIG pcfg)
{
  PGAPC_INSTANCE  ppi = NULL;

  gint            rc = 0;

  g_return_val_if_fail (pcfg != NULL, NULL);
  g_return_val_if_fail (pcfg->q_network != NULL, NULL);

  g_async_queue_ref (pcfg->q_network);

  ppi = (PGAPC_INSTANCE) g_async_queue_pop (pcfg->q_network);

  while (ppi)
  {
    if (!pcfg->b_run)
      break;

    if (ppi->b_run)
    {
      g_mutex_lock (ppi->gm_update);
      if ((rc = gapc_net_transaction_service (ppi, "status", ppi->pach_status)))
      {
        gapc_net_transaction_service (ppi, "events", ppi->pach_events);
      }
      g_mutex_unlock (ppi->gm_update);

      if (rc > 0)
      {
        ppi->b_data_available = TRUE;
      }
      else
      {
        ppi->b_data_available = FALSE;
      }
    }
    else
    {
      ppi->b_data_available = FALSE;
    }

    ppi = (PGAPC_INSTANCE) g_async_queue_pop (pcfg->q_network);
  }

  g_async_queue_unref (pcfg->q_network);

  g_thread_exit (GINT_TO_POINTER (1));

  return NULL;
}

/*
 * parses received line of text into key/value pairs to be inserted
 * into the status hashtable.
 */
static gint gapc_util_update_hashtable (PGAPC_INSTANCE ppi,
                                        gchar * pch_unparsed)
{
  gchar          *pch_in = NULL;
  gchar          *pch = NULL;
  gchar          *pch_end = NULL;
  gint            ilen = 0;

  g_return_val_if_fail (ppi != NULL, FALSE);
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

  g_hash_table_replace (ppi->pht_Status, g_strdup (pch_in), g_strdup (pch));

  g_free (pch_in);

  return ilen;
}

/*
 * creates horizontal bar chart and allocates control data
 * requires cb_h_bar_chart_exposed() routine
 * return drawing area widget
 */
static GtkWidget *gapc_util_barchart_create (PGAPC_INSTANCE ppi,
                                             GtkWidget * vbox,
                                             gchar * pch_hbar_name,
                                             gdouble d_percent,
                                             gchar * pch_text)
{
  PGAPC_BAR_H     pbar = NULL;
  GtkWidget      *drawing_area = NULL;
  gchar          *pch = NULL;

  g_return_val_if_fail (ppi != NULL, NULL);

  pbar = g_new0 (GAPC_BAR_H, 1);
  pbar->d_value = d_percent;
  pbar->b_center_text = FALSE;
  g_strlcpy (pbar->c_text, pch_text, sizeof (pbar->c_text));

  drawing_area = gtk_drawing_area_new (); /* manual bargraph */
  gtk_widget_set_size_request (drawing_area, 100, 20);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                    G_CALLBACK (cb_util_barchart_handle_exposed),
                    (gpointer) pbar);

  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
  g_hash_table_insert (ppi->pht_Status, g_strdup (pch_hbar_name), pbar);
  pch = g_strdup_printf ("%s-Widget", pch_hbar_name);
  g_hash_table_insert (ppi->pht_Widgets, pch, drawing_area);

  return drawing_area;
}

/*
 * Utility Routines for text views
 */
static void gapc_util_text_view_clear_buffer (GtkWidget * view)
{
  GtkTextIter     start, end;
  GtkTextBuffer  *buffer = NULL;

  g_return_if_fail (view != NULL);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_delete (buffer, &start, &end);

  return;
}

/*
 * Utility Routines for text views
 */
static void gapc_util_text_view_prepend (GtkWidget * view, gchar * pch)
{
  GtkTextIter     iter;
  GtkTextBuffer  *buffer;

  g_return_if_fail (view != NULL);

  if (pch == NULL)
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_start_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, pch, -1);
}

/*
 * Utility Routines for text views
 */
static void gapc_util_text_view_append (GtkWidget * view, gchar * pch)
{
  GtkTextIter     iter;
  GtkTextBuffer  *buffer;

  g_return_if_fail (view != NULL);

  if (pch == NULL)
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, pch, -1);
}

/*
 * Events and Status Report Pages
*/
static gint gapc_information_text_report_page (PGAPC_INSTANCE ppi,
                                               GtkWidget * notebook,
                                               gchar * pchTitle, gchar * pchKey)
{
  PangoFontDescription *font_desc;
  GtkWidget      *scrolled, *view, *label;
  gint            i_page = 0;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  label = gtk_label_new (pchTitle);
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled, label);

  view = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled), view);

  /* Change default font throughout the widget */
  font_desc = pango_font_description_from_string ("Monospace 9");
  gtk_widget_modify_font (view, font_desc);
  pango_font_description_free (font_desc);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 5);

  g_hash_table_insert (ppi->pht_Widgets, g_strdup (pchKey), view);

  return i_page;
}

/*
 * Detailed Information Notebook Page
*/
static gint gapc_information_information_page (PGAPC_INSTANCE ppi,
                                               GtkWidget * notebook)
{
  GtkWidget      *frame, *label, *pbox, *lbox, *rbox, *gbox;
  GtkWidget      *tbox, *tlbox, *trbox;
  gint            i_page = 0;

  /* Create a Notebook Page */
  gbox = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER (gbox), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (gbox), GTK_SHADOW_NONE);
  label = gtk_label_new ("Detailed Information");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gbox, label);

  tbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (gbox), tbox);

  /*
   * create basic frame */
  tlbox = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (tbox), tlbox, TRUE, TRUE, 0);
  trbox = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (tbox), trbox, TRUE, TRUE, 0);

  frame = gtk_frame_new ("<b><i>Software Information</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (tlbox), frame, TRUE, TRUE, 0);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
  rbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("APCUPSD version\n" "Monitored UPS name\n"
                         "Cable Driver type\n" "Configuration mode\n"
                         "Last started\n" "UPS State");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Waiting for refresh\n");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("SoftwareInformation"),
                       label);

  frame = gtk_frame_new ("<b><i>Performance Summary</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_end (GTK_BOX (tlbox), frame, TRUE, TRUE, 0);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
  rbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("Selftest running\n" "Number of transfers\n"
                         "Reason last transfer\n" "Last transfer to battery\n"
                         "Last transfer off battery\n" "Time on battery\n"
                         "Cummulative on battery");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Waiting for refresh\n");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("PerformanceSummary"),
                       label);

  frame = gtk_frame_new ("<b><i>UPS Metrics</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (trbox), frame, TRUE, TRUE, 0);

  gbox = gtk_vbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (frame), gbox);

  gapc_util_barchart_create (ppi, gbox, "HBar1", 10.8, "Waiting for refresh");
  gapc_util_barchart_create (ppi, gbox, "HBar2", 40.8, "Waiting for refresh");
  gapc_util_barchart_create (ppi, gbox, "HBar3", 0.8, "Waiting for refresh");
  gapc_util_barchart_create (ppi, gbox, "HBar4", 40.8, "Waiting for refresh");
  gapc_util_barchart_create (ppi, gbox, "HBar5", 10.8, "Waiting for refresh");

  frame = gtk_frame_new ("<b><i>Product Information</i></b>");
  gtk_frame_set_label_align (GTK_FRAME (frame), 0.1, 0.8);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_end (GTK_BOX (trbox), frame, TRUE, TRUE, 0);

  pbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pbox);
  lbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), lbox, FALSE, FALSE, 0);
  rbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (pbox), rbox, TRUE, TRUE, 0);

  label = gtk_label_new ("Device\n" "Serial\n" "Manf date\n" "Firmware\n"
                         "Batt date");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (lbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("Waiting for refresh\n");
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (rbox), label, TRUE, TRUE, 0);
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("ProductInformation"),
                       label);

  return i_page;
}

/*
 *  Implements a Horizontal Bar Chart...
 *  - data value has a range of 0.0 to 1.0 for 0-100% display
 *  - in chart text is limited to about 30 chars
 */
static gboolean cb_util_barchart_handle_exposed (GtkWidget * widget,
                                                 GdkEventExpose * event,
                                                 gpointer data)
{
  PGAPC_BAR_H     pbar = data;
  gint            i_percent = 0;
  PangoLayout    *playout = NULL;

  g_return_val_if_fail (data, FALSE); /* error exit */

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
                 GTK_SHADOW_ETCHED_IN, &pbar->rect, widget, "gapc_hbar_frame",
                 0, 0, widget->allocation.width - 1,
                 widget->allocation.height - 1);

  /* the scaled value */
  gtk_paint_box (widget->style, widget->window, GTK_STATE_SELECTED,
                 GTK_SHADOW_OUT, &pbar->rect, widget, "gapc_hbar_value", 1, 1,
                 i_percent, widget->allocation.height - 4);

  if (pbar->c_text[0])
  {
    gint            x = 0, y = 0;

    playout = gtk_widget_create_pango_layout (widget, pbar->c_text);
    pango_layout_set_markup (playout, pbar->c_text, -1);

    pango_layout_get_pixel_size (playout, &x, &y);
    x = (widget->allocation.width - x) / 2;
    y = (widget->allocation.height - y) / 2;

    gtk_paint_layout (widget->style, widget->window, GTK_WIDGET_STATE (widget),
                      TRUE, &pbar->rect, widget, "gapc_hbar_text",
                      (pbar->b_center_text) ? x : 6, y, playout);

    g_object_unref (playout);
  }

  return TRUE;
}

/* *******************************************************************
 * GtkGLGraph Graphing Routines
 * 
 * *******************************************************************
*/
/*
 * Creates a GtkGLGraph histogram linechart page
*/
static gint gapc_information_history_page (PGAPC_INSTANCE ppi,
                                           GtkWidget * notebook)
{
  PGAPC_HISTORY   pphs = (PGAPC_HISTORY) & ppi->phs;
  gint            i_page = 0, i_series = 0;
  GtkWidget      *label = NULL, *box = NULL;
  gchar          *pch_colors[] = { "green", "blue", "red", "yellow", "black" };
  gchar          *pch_legend[] =
          { "LINEV", "LOADPCT", "TIMELEFT", "BCHARGE", "BATTV" };

  g_return_val_if_fail (ppi != NULL, -1);

  /*
   * Prepare the environment */
  pphs->d_xinc = ppi->d_refresh;
  pphs->pht_Status = &ppi->pht_Status; /* in case they do change */
  pphs->pht_Widgets = &ppi->pht_Widgets;

  if (pphs->ch_label_color[0][0] == '\0') /* dont re-init values */
    for (i_series = 0; i_series < GAPC_HISTORY_CHART_SERIES; i_series++)
    {
      g_snprintf (pphs->ch_label_color[i_series], GAPC_MAX_ARRAY, "%s",
                  pch_colors[i_series]);
      g_snprintf (pphs->ch_label_legend[i_series], GAPC_MAX_ARRAY, "%s",
                  pch_legend[i_series]);
    }

  /*
   * Create notebook page page */
  box = gtk_vbox_new (FALSE, 0);
  label = gtk_label_new ("Historical Summary");
  i_page = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

  /*
   * Create Chart surface */
  if (gapc_util_line_chart_create (pphs, box))
  {
    g_signal_connect (pphs->glg, "button-press-event",
                      G_CALLBACK (cb_util_line_chart_toggle_legend), pphs);

    gapc_util_line_chart_ds_init (pphs, 5);
  }

  pphs->timer_id =
          gtk_timeout_add (((ppi->d_refresh * GAPC_REFRESH_FACTOR_1K *
                            GAPC_HISTORY_CHART_FACTOR_XINC) + 75),
                           (GSourceFunc) cb_util_line_chart_refresh, pphs);

  return i_page;
}

/*
 * Add a new data point until xrange is reached
 * then rotate back the y points and add new point to end
 * return TRUE is successful, FALSE other wise
*/
static gboolean cb_util_line_chart_refresh (PGAPC_HISTORY pg)
{
  gint            h_index = 0, i_pos = 0, i_series = 0;
  GList          *ds;
  GtkGLGraph     *glg;
  GtkGLGDataSet  *glds;
  gboolean        b_flag = FALSE;
  PGAPC_INSTANCE  ppi = (PGAPC_INSTANCE)pg->gp;

  g_return_val_if_fail (pg != NULL, FALSE);


  if (ppi->b_run != TRUE)    /* stop this timer */
      return FALSE;
 
  glg = pg->glg;
  ds = g_list_first (glg->datasets);

  g_return_val_if_fail (ds != NULL, FALSE);

  while (ds != NULL)
  {
    glds = ds->data;
    if (glds == NULL)
      break;

    if (glds->x_length > GAPC_LINEGRAPH_XMAX)
    {
      for (h_index = 0; h_index < GAPC_LINEGRAPH_XMAX; h_index++)
      {
        glds->y[h_index] = glds->y[h_index + 1];
      }

      b_flag = TRUE;
    }

    if (b_flag)
    {
      i_pos = glds->x_length - 1;
      glds->x[i_pos] = i_pos;
      glds->y[i_pos] = gapc_util_point_filter_reset (&(pg->sq[i_series]));
      b_flag = FALSE;
    }
    else
    {
      i_pos = glds->x_length++;
      glds->y_length++;
      glds->x[i_pos] = i_pos;
      glds->y[i_pos] = gapc_util_point_filter_reset (&(pg->sq[i_series]));
    }

    ds = g_list_next (ds);
    i_series++;
  }

  gdk_threads_enter ();
  gtk_glgraph_redraw (pg->glg);
  gdk_threads_leave ();
  
  return TRUE;
}

/*
 * Initialize the requested number of data series
*/
static gboolean gapc_util_line_chart_ds_init (PGAPC_HISTORY pg, gint i_series)
{
  GtkGLGDataSet  *glds;
  gint            v_index = 0;
  GdkColor        color;

  g_return_val_if_fail (pg != NULL, FALSE);
  g_return_val_if_fail (i_series <= GAPC_MAX_SERIES, FALSE);

  /* Allocate the gtkgldataset */
  for (v_index = 0; v_index < i_series; v_index++)
  {
    glds = gtk_glgraph_dataset_create ();

    g_return_val_if_fail (glds != NULL, FALSE);

    glds->graph_type = GTKGLG_TYPE_XY;
    glds->y_units = g_strdup (pg->ch_label_legend[v_index]);
    glds->x_units = g_strdup ("intervals");

    glds->x = g_new0 (gdouble, GAPC_LINEGRAPH_XMAX + 4);
    glds->y = g_new0 (gdouble, GAPC_LINEGRAPH_XMAX + 4);

    glds->z = g_new0 (gdouble, GAPC_LINEGRAPH_XMAX + 4);
    glds->x_length = 0;
    glds->y_length = 0;
    glds->z_length = 0;

    gdk_color_parse (pg->ch_label_color[v_index], &color);
    glds->line_color[0] = color.red;
    glds->line_color[1] = color.green;
    glds->line_color[2] = color.blue;
    glds->line_color[3] = 1;

    glds->point_color[0] = color.red;
    glds->point_color[1] = color.green;
    glds->point_color[2] = color.blue;
    glds->point_color[3] = 1;

    glds->glgcmap = GTKGLG_COLORMAP_JET;
    glds->glgcmap_alpha = 0.0;

    glds->draw_points = TRUE;
    glds->draw_lines = TRUE;
    glds->stipple_line = FALSE;

    glds->line_width = 2.0;
    glds->line_stipple = 0;
    glds->point_size = 4.0;

    gtk_glgraph_dataset_add (pg->glg, glds);
    gtk_glgraph_dataset_set_title (pg->glg, glds, pg->ch_label_legend[v_index]);
  }

  if (v_index == i_series)
    return TRUE;
  else
    return FALSE;
}

/*
 * Create a GtkGLGraph and set its attributes 
*/
static gboolean gapc_util_line_chart_create (PGAPC_HISTORY pg, GtkWidget * box)
{
  GtkGLGraph     *glg;
  GtkGLGDraw      draw;
  gchar          *pch;

  g_return_val_if_fail (pg != NULL, FALSE);

  /* 
   * Create the GtkGLGraph Object */
  glg = pg->glg = GTK_GLGRAPH (gtk_glgraph_new ());
  gtk_glgraph_axis_set_mode (glg, GTKGLG_AXIS_MODE_FILL);
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (glg));
  
  gtk_widget_show(GTK_WIDGET(glg));
 
  draw = gtk_glgraph_get_drawn (glg);
  draw |= GTKGLG_D_TITLE;
  gtk_glgraph_set_drawn (glg, draw);
  glg->tooltip_visible = FALSE;
  glg->legend_in_out = TRUE;    /* put legend inside the box */
  
  /* 
   * Set the graph attributes */
  gtk_glgraph_set_title (glg,
                         "<span foreground=\"red\"><big>"
                         "click to toggle legend" "</big></span> ");

  pch = g_strdup_printf ("<span foreground=\"blue\">"
                         "<i>sampled every %3.2f seconds</i>" "</span> ",
                         pg->d_xinc * GAPC_HISTORY_CHART_FACTOR_XINC);
  gtk_glgraph_axis_set_label (glg, GTKGLG_AXIS_X, pch);
  g_free (pch);

  gtk_glgraph_axis_set_label (glg, GTKGLG_AXIS_Y,
                              "<span foreground=\"blue\">"
                              "Percentage of 100 % normal" "</span> ");

  gtk_glgraph_axis_set_drawn (glg, GTKGLG_AXIS_X,
                              GTKGLG_DA_AXIS | GTKGLG_DA_MAJOR_GRID |
                              GTKGLG_DA_MINOR_GRID | GTKGLG_DA_TITLE |
                              GTKGLG_DA_MAJOR_TICK_TEXT);
  gtk_glgraph_axis_set_drawn (glg, GTKGLG_AXIS_Y,
                              GTKGLG_DA_AXIS | GTKGLG_DA_MAJOR_GRID |
                              GTKGLG_DA_MINOR_GRID | GTKGLG_DA_TITLE |
                              GTKGLG_DA_MAJOR_TICK_TEXT);
  gtk_glgraph_axis_set_drawn (glg, GTKGLG_AXIS_Z, 0);

  pg->xmin = 0;
  pg->xmax = GAPC_LINEGRAPH_XMAX;
  pg->xmajor_steps = pg->xmax / 2;
  pg->xminor_steps = 2;         /* pg->xmax; */
  pg->xprecision = 0;
  gtk_glgraph_axis_set_range (glg, GTKGLG_AXIS_X, &(pg->xmin), &(pg->xmax),
                              &(pg->xmajor_steps), &(pg->xminor_steps),
                              &(pg->xprecision));
  pg->ymin = 0.0;
  pg->ymax = GAPC_LINEGRAPH_YMAX;
  pg->ymajor_steps = pg->ymax / 10;
  pg->yminor_steps = 5;         /* pg->ymajor_steps * 2; */
  pg->yprecision = 0;
  gtk_glgraph_axis_set_range (glg, GTKGLG_AXIS_Y, &(pg->ymin), &(pg->ymax),
                              &(pg->ymajor_steps), &(pg->yminor_steps),
                              &(pg->yprecision));
  if (glg != NULL)
    return TRUE;
  else
    return FALSE;
}

/*
 * Attempt to make visible then invisible the chart legend
*/
static gboolean gapc_util_line_chart_toggle_legend (PGAPC_HISTORY pg)
{
  GtkGLGDraw      draw;

  g_return_val_if_fail (pg != NULL, FALSE);

  draw = gtk_glgraph_get_drawn (pg->glg);
  if (draw & GTKGLG_D_LEGEND)
  {
    draw -= GTKGLG_D_LEGEND;
  }
  else
  {
    draw += GTKGLG_D_LEGEND;
  }

  gtk_glgraph_set_drawn (pg->glg, draw);
  gtk_glgraph_redraw (pg->glg);

  return TRUE;
}

/*
 * Handles clicked signal from eventbox underneath 
 * the glgraph window.  Used to toggle the legend on/off
*/
static gboolean cb_util_line_chart_toggle_legend (GtkWidget * widget,
                                                  GdkEventButton * event,
                                                  PGAPC_HISTORY pg)
{
  g_return_val_if_fail (pg != NULL, FALSE);

  if (!((event->type == GDK_BUTTON_PRESS) && (event->button == 1)))
    return FALSE;               /* clicked button 1 only */

  gapc_util_line_chart_toggle_legend (pg);

  return TRUE;
}

/*
 * Handle the refresh button action from the information window
*/
static void cb_information_window_button_refresh (GtkButton * button,
                                                  PGAPC_INSTANCE ppi)
{
  gapc_monitor_update_tooltip_msg (ppi);
  if (gapc_monitor_update (ppi))
  {
    GtkWidget      *w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");
    if ( w != NULL )
    {
        gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                        "Refresh Completed!...");
    }
  }
  else
  {
    GtkWidget      *w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");

    if ( w != NULL )
    {
        gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                        "Refresh Failed(retry enabled): Thread is Busy...");
        g_timeout_add (200, (GSourceFunc)cb_monitor_dedicated_one_time_refresh, ppi);
    }
  }

  return;
}

/*
 * Handle the quit button action from the information window
*/
static void cb_information_window_button_quit (GtkButton * button,
                                               PGAPC_INSTANCE ppi)
{
  ppi->b_window_visible = FALSE;

  gtk_widget_hide (ppi->window);
  return;
}

/*
 * Prevent the normal window exit, just hide the window
*/
static gboolean cb_information_window_delete_event (GtkWidget * window,
                                                    GdkEvent * event,
                                                    PGAPC_INSTANCE ppi)
{

  ppi->b_window_visible = FALSE;

  gtk_widget_hide (window);
  return TRUE;
}

/*
 * Cleanup ptr if window is destroyed (and it will be)
*/
static void cb_information_window_destroy (GtkWidget * window,
                                           PGAPC_INSTANCE ppi)
{

  ppi->b_window_visible = FALSE;

  ppi->window = NULL;
  g_hash_table_remove (ppi->pht_Widgets, "StatusBar");
  g_hash_table_remove (ppi->pht_Widgets, "TitleStatus");   
  g_hash_table_remove (ppi->pht_Widgets, "InformationWindow");     
  /*
   * stop History Page refresh*/
  if (ppi->phs.timer_id)
  {
    g_source_remove (ppi->phs.timer_id);   
    ppi->phs.timer_id = 0;
  }
  if ( ppi->phs.glg != NULL )
  {
      if (ppi->phs.glg->tooltip_id)
      {
        g_source_remove (ppi->phs.glg->tooltip_id);    
        ppi->phs.glg->tooltip_id = 0;
      }
      if (ppi->phs.glg->expose_id)
      {
        g_source_remove (ppi->phs.glg->expose_id);        
        ppi->phs.glg->expose_id = 0;
      }
  }
  return;
}

/*
 * Handle nulling the dlg window ptr when its destroyed
*/
static void cb_preferences_dialog_destroy (GtkWidget * widget, gpointer gp)
{
  PGAPC_CONFIG    pcfg = gp;

  pcfg->prefs_dlg = NULL;

  return;
}

/*
 * Handle the preferences dlg buttons response
 *  hide the window isntead of deleting it, when possible
*/
static void cb_preferences_dialog_response (GtkDialog * dialog, gint arg1,
                                            PGAPC_CONFIG pcfg)
{
  PGAPC_INSTANCE  ppi = NULL;
  GtkWidget      *w = NULL;
  gchar          *pch = NULL;
  gint            x = 0;

  switch (arg1)
  {
    case GTK_RESPONSE_NONE:    /* delete event for dlg */
      gtk_widget_hide_on_delete (GTK_WIDGET (dialog));
      break;
    case GTK_RESPONSE_CLOSE:   /* close button         */
    default:
      gtk_widget_hide (GTK_WIDGET (dialog));
  }

  pcfg->cb_monitors = gapc_monitor_interface_count_enabled (pcfg->prefs_model);
/* *************************************************** */
/* COME BACK HERE AT SOME POINT WITH A NO ZERO MESSAGE */
/* *************************************************** */

  for (x = 0; (x < GAPC_MONITOR_MAX) && pcfg->ppi[x].cb_enabled; x++)
  {
    ppi = (PGAPC_INSTANCE) & (pcfg->ppi[x]);
    if (ppi->window != NULL)
    {
      w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");
      if ( w != NULL )
      {
        continue;
      }
      pch = g_strdup_printf ("Preferences Complete: %d Monitors online..",
                             pcfg->cb_monitors);
      gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context, pch);
      g_free (pch);
    }
  }

  return;
}

/*
 * Handles closing down the app.
*/
static void cb_applet_destroy (GtkObject * object, PGAPC_CONFIG pcfg)
{
  gchar          *pgroup =
          g_strdup_printf (GAPC_GROUP_KEY, pcfg->cb_instance_num);
  gint            x = 0, i_x = 0;

  pcfg->b_run = FALSE;          /* stop the thread */

  for (x = 0; x < GAPC_MONITOR_MAX; x++)
  {
    if (pcfg->ppi[x].b_run)
    {
      pcfg->ppi[x].b_run = FALSE; /*stop timers */
      g_source_remove (pcfg->ppi[x].tid_automatic_refresh);
      pcfg->ppi[x].tid_automatic_refresh = 0;      
    }
  }
  
  gconf_client_notify_remove (pcfg->client, pcfg->i_group_id);
  gconf_client_remove_dir (pcfg->client, pgroup, NULL);
  g_object_unref (pcfg->client);

  g_async_queue_push (pcfg->q_network,
                      (PGAPC_INSTANCE) & (pcfg->ppi[0].cb_instance_num));
  g_thread_join (pcfg->tid_thread_qwork);
  g_async_queue_unref (pcfg->q_network);
  g_free (pgroup);

  if (pcfg->prefs_model != NULL)
  {
    gtk_list_store_clear (GTK_LIST_STORE (pcfg->prefs_model));
    g_object_unref (G_OBJECT (pcfg->prefs_model));
  }

  for (x = 0; x < GAPC_MONITOR_MAX; x++)
  {
    if (pcfg->ppi[x].gm_update != NULL)
      g_mutex_free (pcfg->ppi[x].gm_update);

    if (pcfg->ppi[x].pht_Widgets != NULL)
    {
      g_hash_table_destroy (pcfg->ppi[x].pht_Widgets);
      g_hash_table_destroy (pcfg->ppi[x].pht_Status);
    }
    for (i_x = 0; i_x < GAPC_MAX_ARRAY; i_x++)
    {
      if (pcfg->ppi[x].pach_status[i_x])
      {
        g_free (pcfg->ppi[x].pach_status[i_x]);
        pcfg->ppi[x].pach_status[i_x] = NULL;
      }
      if (pcfg->ppi[x].pach_events[i_x])
      {
        g_free (pcfg->ppi[x].pach_events[i_x]);
        pcfg->ppi[x].pach_events[i_x] = NULL;
      }
    }
  }

  return;
}

/*
 * Handles changing the vertical/horizontal orientation of the
 * panel when it changes
*/
static void cb_applet_change_orientation (PanelApplet * applet, guint arg1,
                                          PGAPC_CONFIG pcfg)
{
  GtkWidget      *ppbox = NULL;
  GList          *panels = NULL, *item = NULL;

  g_return_if_fail (pcfg != NULL);

  switch (arg1)
  {
    case PANEL_APPLET_ORIENT_LEFT:
    case PANEL_APPLET_ORIENT_RIGHT:
      ppbox = gtk_vbox_new (TRUE, 1);
      break;
    case PANEL_APPLET_ORIENT_DOWN:
    case PANEL_APPLET_ORIENT_UP:
    default:
      ppbox = gtk_hbox_new (TRUE, 3);
      break;
  }

  pcfg->orientation = arg1;     /* save it for later */

  /*
   * Swap v/h boxes */
  panels = gtk_container_get_children (GTK_CONTAINER (pcfg->ppbox));
  item = g_list_first (panels);
  while (item)
  {
    gtk_container_remove (GTK_CONTAINER (pcfg->ppbox), GTK_WIDGET (item->data));
    gtk_box_pack_start (GTK_BOX(ppbox), GTK_WIDGET(item->data), TRUE, TRUE, 0);
    item = g_list_next (item);
  }
  g_list_free (panels);

  gtk_widget_destroy (pcfg->ppbox);
  gtk_container_add (GTK_CONTAINER (pcfg->ppframe), ppbox);
  pcfg->ppbox = ppbox;
  gtk_widget_show_all (pcfg->ppframe);

  return;
}

/*
 * Handles clicked signal from panel icon buttons
 * Assume frame has user data of its monitor number.
*/
static gboolean cb_monitor_handle_icon_clicked (GtkWidget * widget,
                                                GdkEventButton * event,
                                                PGAPC_INSTANCE ppi)
{

  g_return_val_if_fail (ppi != NULL, FALSE);

  if (!((event->type == GDK_BUTTON_PRESS) && (event->button == 1)))
    return FALSE;               /* clicked button 1 only */

  if (ppi->window == NULL)
  {
    ppi->window = gapc_information_window_create (ppi);
    gtk_widget_show_all (ppi->window);
    ppi->b_window_visible = TRUE;
    return FALSE;
  }
  if ((ppi->b_window_visible) && (ppi->window != NULL))
  {
    gtk_widget_hide (ppi->window);
    ppi->b_window_visible = FALSE;
    return FALSE;
  }
  if (!(ppi->b_window_visible) && (ppi->window != NULL))
  {
    gtk_widget_show (ppi->window);
    ppi->b_window_visible = TRUE;
    return FALSE;
  }

  return FALSE;
}

/*
 * callback to handle preference values changing (maybe not different,
 * but changed anyway).   entry is the new value,
 *  cnxn_id is the id given when gconf added this monitor for this value..
*/
static void cb_monitor_preferences_changed (GConfClient * client, guint cnxn_id,
                                            GConfEntry * entry,
                                            PGAPC_CONFIG pcfg)
{
  gchar          *p_key = NULL;
  gint            i_monitor = 0, i_count = 0;
  PGAPC_INSTANCE  ppi = NULL;
  gboolean        b_old_enable = FALSE;

  g_return_if_fail (pcfg != NULL);

  p_key = g_strrstr (entry->key, "/");
  *p_key = '\0';
  i_monitor = (gint) g_strtod (p_key - 1, NULL); /* get monitor number */
  p_key++;                      /* point to actual key */

  ppi = (PGAPC_INSTANCE) & (pcfg->ppi[i_monitor]);

  if (entry->value != NULL)
  {
    switch (entry->value->type)
    {
      case GCONF_VALUE_STRING:
        g_free (ppi->pch_host);
        ppi->pch_host = g_strdup (gconf_value_get_string (entry->value));
        ppi->b_network_changed = TRUE;
        break;
      case GCONF_VALUE_INT:
        ppi->i_port = gconf_value_get_int (entry->value);
        break;
      case GCONF_VALUE_BOOL:
        b_old_enable = ppi->cb_enabled;
        ppi->cb_enabled = gconf_value_get_bool (entry->value);
                               
        if (ppi->cb_enabled)        
        {
          gapc_monitor_interface_create (ppi);
        }
        else
        {
          gapc_monitor_interface_remove (ppi);
        }
                 
        i_count = gapc_monitor_interface_count_enabled (ppi->prefs_model);
        if (i_count == 0)
        {
            gapc_preferences_model_enable_one ( (PGAPC_CONFIG)ppi->gp );
        }
        break;
      case GCONF_VALUE_FLOAT:
        ppi->d_refresh = (gdouble) gconf_value_get_float (entry->value);
        ppi->b_timer_control = TRUE;
        break;
      default:
        g_message ("cb_monitor_preferences_changed(UnKnown Data Type->%d)",
                   entry->value->type);
    }
  }

  return;
}

/*
 * Called when the panel changes its size in response to user action
 * Icons need to be rescaled to fit - need a better get_size routine
*/
static void cb_applet_change_size (PanelApplet * applet, gint size,
                                   PGAPC_CONFIG pcfg)
{
  GdkPixbuf      *pixbuf;
  GdkPixbuf      *scaled;
  guint           i_x = 0;

  g_return_if_fail (applet != NULL);
  g_return_if_fail (pcfg != NULL);

  for (i_x = 0; i_x < GAPC_MONITOR_MAX; i_x++)
  {
    if (!pcfg->ppi[i_x].b_run)
      continue;

    /*
     * save it for next time */
    if (pcfg->ppi[i_x].i_icon_index >= GAPC_N_ICONS)
    {
      pcfg->ppi[i_x].i_icon_index = GAPC_ICON_ONLINE;
    }
    else pcfg->ppi[i_x].i_old_icon_index = pcfg->ppi[i_x].i_icon_index;
    
    pixbuf = pcfg->my_icons[pcfg->ppi[i_x].i_icon_index];
    if (pixbuf)
    {
      scaled = gdk_pixbuf_scale_simple (pixbuf, size, size,
                                        GDK_INTERP_BILINEAR);

      gtk_image_set_from_pixbuf (GTK_IMAGE (pcfg->ppi[i_x].image), scaled);
      gtk_widget_show (pcfg->ppi[i_x].image);

      gdk_pixbuf_unref (scaled);
    }
  }

  return;
}

/*
 *  Menu Callback : Display About window
*/
static void cb_applet_menu_verbs (BonoboUIComponent * uic, PGAPC_CONFIG pcfg,
                                  const gchar * verbname)
{

  if (g_str_equal (verbname, "gapc_about"))
  {
    gapc_applet_interface_about_dlg ( pcfg );
    return;
  }
  if (g_str_equal (verbname, "gapc_preferences"))
  {
    if (pcfg->prefs_dlg != NULL)
    {
      gtk_widget_show_all (pcfg->prefs_dlg);
      return;
    }

    pcfg->prefs_dlg = gapc_preferences_dialog_create (pcfg);
    return;
  }

  return;
}

/*
 * timer service routine for IPL refresh and refresh_button.
 * used to overcome the multi-threaded startup delay.  very short
 */
static gboolean cb_monitor_dedicated_one_time_refresh (PGAPC_INSTANCE ppi)
{
  GtkWidget      *w = NULL;

  g_return_val_if_fail (ppi != NULL, FALSE);

  if ( (!ppi->b_run) || !(ppi->cb_enabled) )
  {
    return FALSE;
  }

  gdk_threads_enter ();

  w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");
  if (w != NULL)
  {
    gtk_statusbar_pop (GTK_STATUSBAR (w), ppi->i_info_context);
  }

  gapc_monitor_update_tooltip_msg (ppi);

  if (!gapc_monitor_update (ppi))
  {
    if (w != NULL)
    {
      gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                          "Refresh Not Completed(retry enabled)... Network Busy!");
    }
    gdk_flush ();
    gdk_threads_leave ();
    return TRUE;                /* try again */
  }

  if (w != NULL)
  {
    gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                        "One-Time Refresh Completed...");
  }
  gdk_flush ();
  gdk_threads_leave ();

  return FALSE;                 /* this will terminate the timer */
}

/*
 * used to switch timers when d_refresh changes
 * b_timer_control True trigger the target timer to stop and have this one
 * restart it; so it can pickup the new interval;
*/
static gboolean cb_monitor_refresh_control (PGAPC_INSTANCE ppi)
{

  g_return_val_if_fail (ppi != NULL, FALSE);

  if ((!ppi->b_run) || !(ppi->cb_enabled))
    return FALSE;               /* stop timers */

  ppi->b_timer_control = FALSE;

  gdk_threads_enter ();

  ppi->tid_automatic_refresh =
          g_timeout_add ((guint) (ppi->d_refresh * GAPC_REFRESH_FACTOR_1K),
                         (GSourceFunc) cb_monitor_automatic_refresh, ppi);

  if ((ppi->tid_automatic_refresh != 0) && (ppi->window != NULL))
  {
    GtkWidget  *w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");
    if ( w != NULL )
    {
        gtk_statusbar_pop (GTK_STATUSBAR (w), ppi->i_info_context);
        gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                        "Refresh Cycle Change Completed!...");
    }
  }

  gdk_flush ();
  gdk_threads_leave ();

  return FALSE;
}

/*
 * used to make a work request to network queue
*/
static gboolean cb_monitor_automatic_refresh (PGAPC_INSTANCE ppi)
{

  g_return_val_if_fail (ppi != NULL, FALSE);

  if ((!ppi->b_run) || !(ppi->cb_enabled))
    return FALSE;               /* stop timers */

  if (ppi->b_timer_control)
  {
    g_timeout_add (100, (GSourceFunc) cb_monitor_refresh_control, ppi);
    return FALSE;
  }

  gdk_threads_enter ();

  gapc_monitor_update_tooltip_msg (ppi);

  if (ppi->window)
  {
    if (gapc_monitor_update (ppi))
    {
      GtkWidget      *w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");

      if (w != NULL)
      {
        gtk_statusbar_pop (GTK_STATUSBAR (w), ppi->i_info_context);
        gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                            "Automatic refresh complete...");
      }
    }
    else
    {
      GtkWidget      *w = g_hash_table_lookup (ppi->pht_Widgets, "StatusBar");

      if (w != NULL)
      {
        gtk_statusbar_pop (GTK_STATUSBAR (w), ppi->i_info_context);
        gtk_statusbar_push (GTK_STATUSBAR (w), ppi->i_info_context,
                            "Automatic refresh failed! Thread is busy...");
      }
    }
  }
  /*
   * This is the work request to network queue */
  {
    PGAPC_CONFIG    pcfg = (PGAPC_CONFIG) ppi->gp;

    g_async_queue_push (pcfg->q_network, (gpointer) ppi);
  }

  gdk_flush ();
  gdk_threads_leave ();

  return TRUE;
}

/*
 * Handled the toggle of a checkbox in the preferences dialog
*/
static void cb_preferences_handle_cell_toggled (GtkCellRendererToggle * cell,
                                                gchar * path_str, gpointer gp)
{
  PGAPC_COLUMN    pcolumn = (PGAPC_COLUMN) gp;
  GtkTreeIter     iter;
  GtkTreePath    *path = gtk_tree_path_new_from_string (path_str);
  gboolean        fixed;
  gint            i_monitor = 0;
  gchar          *penabled = NULL;

  /* get toggled iter */
  gtk_tree_model_get_iter (pcolumn->prefs_model, &iter, path);

  gtk_tree_model_get (pcolumn->prefs_model, &iter, COLUMN_ENABLED, &fixed,
                      COLUMN_MONITOR, &i_monitor, -1);

  i_monitor -= 1;               /* move value to zero base */

  /* do something with the value */
  fixed ^= 1;

  penabled =
          g_strdup_printf (GAPC_ENABLE_KEY, pcolumn->cb_instance_num,
                           i_monitor);

  gconf_client_set_bool (pcolumn->client, penabled, fixed, NULL);

  g_free (penabled);

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (pcolumn->prefs_model), &iter,
                      COLUMN_ENABLED, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);

  return;
}

/*
 * A callback routine that collect changes to the path row and save it directly
 * to the desired location, also passes it back to the treeview for display.
*/
static void cb_preferences_handle_cell_edited (GtkCellRendererText * cell,
                                               gchar * path_string,
                                               gchar * pch_new,
                                               PGAPC_COLUMN pcolumn)
{
  GtkTreeModel   *model;
  GtkTreeIter     iter;
  GtkTreePath    *path;
  gint            col_number = 0, i_port = 0, i_monitor = 0, cb_id = 0;
  gfloat          f_refresh = 0.0;
  gchar          *s_host = NULL;

  g_return_if_fail (pcolumn != NULL);

  model = pcolumn->prefs_model;
  col_number = pcolumn->i_col_num;
  cb_id = pcolumn->cb_instance_num;

  /*
   * get edited row number
   */
  path = gtk_tree_path_new_from_string (path_string);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  /*
   * get data from that row
   */
  gtk_tree_model_get (model, &iter, COLUMN_MONITOR, &i_monitor, COLUMN_HOST,
                      &s_host, COLUMN_PORT, &i_port, COLUMN_REFRESH, &f_refresh,
                      -1);

  i_monitor -= 1;               /* move value to zero base */

  switch (col_number)
  {
    case COLUMN_HOST:
    {
      gchar          *phost = g_strdup_printf (GAPC_HOST_KEY, cb_id, i_monitor);

      if (pch_new == NULL)
        pch_new = g_strdup (GAPC_HOST_VALUE_DEFAULT);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_HOST, pch_new,
                          -1);
      gconf_client_set_string (pcolumn->client, phost, pch_new, NULL);
      g_free (phost);
    }
      break;
    case COLUMN_PORT:
    {
      gchar          *pport = g_strdup_printf (GAPC_PORT_KEY, cb_id, i_monitor);

      i_port = (gint) g_strtod (pch_new, NULL);

      if (i_port == 0)
        i_port = GAPC_PORT_VALUE_DEFAULT;

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_PORT, i_port,
                          -1);
      gconf_client_set_int (pcolumn->client, pport, i_port, NULL);
      g_free (pport);
    }
      break;
    case COLUMN_REFRESH:
    {
      gchar          *prefresh =
              g_strdup_printf (GAPC_REFRESH_KEY, cb_id, i_monitor);

      f_refresh = (gfloat) g_strtod (pch_new, NULL);

      if (f_refresh < GAPC_MIN_INCREMENT)
        f_refresh = GAPC_MIN_INCREMENT;

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_REFRESH,
                          f_refresh, -1);
      gconf_client_set_float (pcolumn->client, prefresh, f_refresh, NULL);
      g_free (prefresh);
    }
      break;
    default:
      break;
  }

  g_free (s_host);              /* free any string returned from model */

  return;
}

/*
 * Cell data function used to format floating point numbers
*/
static void cb_preferences_handle_float_format (GtkTreeViewColumn * col,
                                                GtkCellRenderer * renderer,
                                                GtkTreeModel * model,
                                                GtkTreeIter * iter, gpointer gp)
{
  gfloat          d_value;
  gchar           buf[32];
  guint           colnum = 0;
  gchar          *pch_format = NULL;

  colnum = GPOINTER_TO_UINT (gp);

  pch_format = (gchar *) g_object_get_data (G_OBJECT (col), "float_format");

  gtk_tree_model_get (model, iter, colnum, &d_value, -1);

  if (pch_format)
    g_snprintf (buf, sizeof (buf), pch_format, d_value);
  else
    g_snprintf (buf, sizeof (buf), "%3.0f", d_value);

  g_object_set (renderer, "text", buf, NULL);

  return;
}

/*
 * Counts the number of monitors enabled in the preferences view
*/
static gint gapc_monitor_interface_count_enabled (GtkTreeModel * model)
{
  GtkTreeIter     iter;
  gboolean        b_valid = FALSE, b_enabled = FALSE;
  gint            row_count = 0, i_monitor = 0, i_num_enabled = 0;

  /* Get the first iter in the list
   */
  b_valid = gtk_tree_model_get_iter_first (model, &iter);

  /* Walk through the list, reading each row
   */
  while (b_valid)
  {
    gtk_tree_model_get (model, &iter, COLUMN_ENABLED, &b_enabled,
                        COLUMN_MONITOR, &i_monitor, -1);

    if (b_enabled)
      i_num_enabled++;

    row_count++;
    b_valid = gtk_tree_model_iter_next (model, &iter);
  }

  return i_num_enabled;
}

/* This routine initializes a user-data structure for use by the renderers of
 * the preference treeview
*/
static PGAPC_COLUMN gapc_preferences_column_data_init (PGAPC_CONFIG pcfg,
                                                       GAPC_PrefType col_num)
{
  PGAPC_COLUMN    pcol = NULL;

  pcol = g_new0 (GAPC_COLUMN, 1);

  pcol->prefs_model = pcfg->prefs_model;
  pcol->i_col_num = col_num;
  pcol->cb_instance_num = pcfg->cb_instance_num;
  pcol->client = pcfg->client;

  return pcol;
}

/*
 * This routine populates the preferences dialog vbox with a TreeView of
 * preferences and returns the treeview id.
*/
static GtkTreeView *gapc_preferences_dialog_view (PGAPC_CONFIG pcfg,
                                                  GtkWidget * vbox)
{
  GtkWidget      *wid = NULL, *sw = NULL;
  GtkTreeModel   *model = NULL;
  GtkTreeView    *treeview = NULL; /* GtkTreeView    */
  GtkTreeViewColumn *column = NULL;
  GtkCellRenderer *renderer, *renderer_int, *renderer_text, *renderer_float,
          *renderer_bool;
  PGAPC_COLUMN    col_bool, col_two, col_three, col_four;

  /* Create the window dressing ****************************
   */
  wid = gtk_label_new ("<i>double-click a column value to change it.</i>");
  gtk_label_set_use_markup (GTK_LABEL (wid), TRUE);
  gtk_label_set_justify (GTK_LABEL (wid), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), wid, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (GTK_WIDGET (sw), 440, 187);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

  /*
   * Access the existing model */
  model = pcfg->prefs_model;

  col_bool = gapc_preferences_column_data_init (pcfg, COLUMN_ENABLED);
  col_two = gapc_preferences_column_data_init (pcfg, COLUMN_HOST);
  col_three = gapc_preferences_column_data_init (pcfg, COLUMN_PORT);
  col_four = gapc_preferences_column_data_init (pcfg, COLUMN_REFRESH);

  /* Create the TreeView **************
   */
  treeview =
          GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);
  gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (treeview));

  /*
   * Create the cell render */
  renderer_bool = gtk_cell_renderer_toggle_new ();
  renderer = gtk_cell_renderer_text_new ();
  renderer_int = gtk_cell_renderer_text_new ();
  renderer_text = gtk_cell_renderer_text_new ();
  renderer_float = gtk_cell_renderer_text_new ();

  g_object_set (G_OBJECT (renderer), "foreground", "blue", NULL);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  g_object_set (G_OBJECT (renderer_int), "xalign", 1.0, NULL);
  g_object_set (G_OBJECT (renderer_text), "xalign", 0.0, NULL);
  g_object_set (G_OBJECT (renderer_float), "xalign", 1.0, NULL);

  g_object_set (G_OBJECT (renderer_int), "editable", TRUE, NULL);
  g_object_set (G_OBJECT (renderer_text), "editable", TRUE, NULL);
  g_object_set (G_OBJECT (renderer_float), "editable", TRUE, NULL);

  /* careful to use these only once per column --
   * using an auto g_free to release allocated storage
   */
  gtk_signal_connect_full (GTK_OBJECT (renderer_bool), "toggled",
                           G_CALLBACK (cb_preferences_handle_cell_toggled),
                           NULL, col_bool, g_free, FALSE, TRUE);
  gtk_signal_connect_full (GTK_OBJECT (renderer_text), "edited",
                           G_CALLBACK (cb_preferences_handle_cell_edited), NULL,
                           col_two, g_free, FALSE, TRUE);
  gtk_signal_connect_full (GTK_OBJECT (renderer_int), "edited",
                           G_CALLBACK (cb_preferences_handle_cell_edited), NULL,
                           col_three, g_free, FALSE, TRUE);
  gtk_signal_connect_full (GTK_OBJECT (renderer_float), "edited",
                           G_CALLBACK (cb_preferences_handle_cell_edited), NULL,
                           col_four, g_free, FALSE, TRUE);

  /* Define the column order and attributes
   */
  column = gtk_tree_view_column_new_with_attributes ("Enabled", renderer_bool,
                                                     "active", COLUMN_ENABLED,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_ENABLED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Monitor", renderer,
                                                     "text", COLUMN_MONITOR,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_MONITOR);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Host Name or IP Address",
                                                     renderer_text, "text",
                                                     COLUMN_HOST, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_HOST);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Port Number",
                                                     renderer_int, "text",
                                                     COLUMN_PORT, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_PORT);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Refresh", renderer_float,
                                                     "text", COLUMN_REFRESH,
                                                     NULL);
  g_object_set_data (G_OBJECT (column), "float_format", "%3.3f");
  gtk_tree_view_column_set_cell_data_func (column, renderer_float,
                                           cb_preferences_handle_float_format,
                                           GUINT_TO_POINTER (COLUMN_REFRESH),
                                           NULL);

  gtk_tree_view_column_set_sort_column_id (column, COLUMN_REFRESH);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  gtk_widget_show_all (sw);

  return GTK_TREE_VIEW (treeview);
}

/*
 * This routine creates the dialog with the preference viewer in it.
*/
static GtkWidget *gapc_preferences_dialog_create (PGAPC_CONFIG pcfg)
{
  GtkWidget      *dialog = NULL;
  GtkWidget      *vbox = NULL;
  GtkTreeView    *treeview = NULL;
  GtkWidget      *button = NULL;

  /* Create a new dialog window for the scrolled window to be
   * packed into.  */
  dialog = gtk_dialog_new ();
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (cb_preferences_dialog_destroy), pcfg);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (cb_preferences_dialog_response), pcfg);
  gtk_window_set_title (GTK_WINDOW (dialog), "gpanel_apcmon: Preferences");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);

  vbox = GTK_DIALOG (dialog)->vbox;

  treeview = gapc_preferences_dialog_view (pcfg, vbox);

  /* Add a "close" button to the bottom of the dialog
   */
  button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE,
                                  GTK_RESPONSE_CLOSE);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);

  gtk_widget_show_all (dialog);

  return dialog;
}

/*
 * This routine creates the information window for each monitor
*/
static GtkWidget *gapc_information_window_create (PGAPC_INSTANCE ppi)
{
  GtkWindow      *window = NULL;
  GtkWidget      *wStatus_bar = NULL;
  GtkWidget      *notebook = NULL, *label = NULL, *button = NULL, *dbutton =
          NULL, *vbox = NULL, *pfbox = NULL, *hbox = NULL;
  gint            i_page = 0;
  PGAPC_CONFIG    pcfg = (PGAPC_CONFIG) ppi->gp;

  g_return_val_if_fail (ppi != NULL, NULL);

  /*
   * Create a new window for the notebook to be packed into.*/
  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("InformationWindow"),
                       window);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);
  gtk_window_set_title (GTK_WINDOW (window), GAPC_WINDOW_TITLE);
  gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
  gtk_window_set_resizable (window, TRUE);
  g_signal_connect (window, "delete-event",
                    G_CALLBACK (cb_information_window_delete_event), ppi);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (cb_information_window_destroy), ppi);
  gtk_window_set_icon (GTK_WINDOW (window), pcfg->my_icons[GAPC_ICON_ONLINE]);

  pfbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), pfbox);

  label = gtk_label_new (GAPC_GROUP_TITLE);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label), FALSE);
  gtk_misc_set_alignment ((GtkMisc *) label, 0.5, 0.5);
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("TitleStatus"), label);
  gtk_box_pack_start (GTK_BOX (pfbox), label, FALSE, TRUE, 4);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (pfbox), vbox, TRUE, TRUE, 0);

  /* Create a new notebook, place the position of the tabs */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 4);

  /*
   * Add the pages */
  gapc_information_history_page (ppi, notebook);
  gapc_information_information_page (ppi, notebook);
  gapc_information_text_report_page (ppi, notebook, "Power Events",
                                     "EventsPage");
  gapc_information_text_report_page (ppi, notebook, "Full UPS Status",
                                     "StatusPage");

  /* Control buttons */
  hbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pfbox), hbox, FALSE, TRUE, 0);

  dbutton = gtk_button_new_from_stock (GTK_STOCK_REFRESH);
  g_signal_connect (dbutton, "clicked",
                    G_CALLBACK (cb_information_window_button_refresh), ppi);
  gtk_box_pack_start (GTK_BOX (hbox), dbutton, TRUE, TRUE, 6);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (cb_information_window_button_quit), ppi);
  gtk_box_pack_end (GTK_BOX (hbox), button, TRUE, TRUE, 6);

  vbox = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (pfbox), vbox, FALSE, TRUE, 0);

  wStatus_bar = gtk_statusbar_new ();
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (wStatus_bar), FALSE);
  g_hash_table_insert (ppi->pht_Widgets, g_strdup ("StatusBar"), wStatus_bar);
  gtk_box_pack_end (GTK_BOX (pfbox), wStatus_bar, FALSE, TRUE, 0);

  ppi->i_info_context =
          gtk_statusbar_get_context_id (GTK_STATUSBAR (wStatus_bar),
                                        "Informational");

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i_page);
  GTK_WIDGET_SET_FLAGS (dbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (dbutton);

  g_async_queue_push (pcfg->q_network, (gpointer) ppi);
  g_timeout_add (250, (GSourceFunc) cb_monitor_dedicated_one_time_refresh, ppi);

  return GTK_WIDGET (window);
}

/*
 * main data updating routine.
 * -- collects and pushes data to all ui
 */
static gint gapc_monitor_update (PGAPC_INSTANCE ppi)
{
  gint            i_x = 0;
  GtkWidget      *win = NULL, *w = NULL;
  gchar          *pch = NULL, *pch1 = NULL, *pch2 = NULL, *pch3 = NULL, *pch4 =
          NULL, *pch5 = NULL, *pch6 = NULL;
  gdouble         dValue = 0.00, dScale = 0.0, dtmp = 0.0, dCharge = 0.0;
  gchar           ch_buffer[GAPC_MAX_TEXT];
  PGAPC_BAR_H     pbar = NULL;

  g_return_val_if_fail (ppi != NULL, FALSE);

  if (ppi->window == NULL)      /* not created yet */
    return TRUE;

  if (ppi->b_data_available == FALSE)
    return FALSE;

  if (!g_mutex_trylock (ppi->gm_update))
    return FALSE;               /* thread must be busy */

  w = g_hash_table_lookup (ppi->pht_Widgets, "StatusPage");
  gapc_util_text_view_clear_buffer (GTK_WIDGET (w));
  for (i_x = 1; ppi->pach_status[i_x] != NULL; i_x++)
    gapc_util_text_view_append (GTK_WIDGET (w), ppi->pach_status[i_x]);

  w = g_hash_table_lookup (ppi->pht_Widgets, "EventsPage");
  gapc_util_text_view_clear_buffer (GTK_WIDGET (w));
  for (i_x = 0; ppi->pach_events[i_x] != NULL; i_x++)
    gapc_util_text_view_prepend (GTK_WIDGET (w), ppi->pach_events[i_x]);

  /*
   *  compute graphic points */
  pch = g_hash_table_lookup (ppi->pht_Status, "LINEV");
  pch1 = g_hash_table_lookup (ppi->pht_Status, "HITRANS");
  dValue = g_strtod (pch, NULL);
  dScale = g_strtod (pch1, NULL);
  if (dScale == 0.0)
    dScale = ((gint) (dValue / 100) * 100) + 100.0;
  dValue /= dScale;
  gapc_util_point_filter_set (&(ppi->phs.sq[0]), dValue);
  pbar = g_hash_table_lookup (ppi->pht_Status, "HBar1");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text), 
              "<span foreground=\"blue\" style=\"italic\">"
              "%s from Utility"
              "</span>", pch);
  w = g_hash_table_lookup (ppi->pht_Widgets, "HBar1-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  pch = g_hash_table_lookup (ppi->pht_Status, "BATTV");
  pch1 = g_hash_table_lookup (ppi->pht_Status, "NOMBATTV");
  dValue = g_strtod (pch, NULL);
  dScale = g_strtod (pch1, NULL);
  dScale *= 1.2;
  if (dScale == 0.0)
    dScale = ((gint) (dValue / 10.0) * 10) + 10.0;
  dValue /= dScale;
  gapc_util_point_filter_set (&(ppi->phs.sq[4]), dValue);
  pbar = g_hash_table_lookup (ppi->pht_Status, "HBar2");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text),
              "<span foreground=\"blue\" style=\"italic\">"
              "%s DC on Battery"
              "</span>", pch);

  w = g_hash_table_lookup (ppi->pht_Widgets, "HBar2-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  pch = g_hash_table_lookup (ppi->pht_Status, "BCHARGE");
  dCharge = dValue = g_strtod (pch, NULL);
  dValue /= 100.0;
  gapc_util_point_filter_set (&(ppi->phs.sq[3]), dValue);
  pbar = g_hash_table_lookup (ppi->pht_Status, "HBar3");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text),
              "<span foreground=\"blue\" style=\"italic\">"
              "%s Battery Charge"
              "</span>", pch);
  w = g_hash_table_lookup (ppi->pht_Widgets, "HBar3-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  pch = g_hash_table_lookup (ppi->pht_Status, "LOADPCT");
  dValue = g_strtod (pch, NULL);
  dtmp = dValue /= 100.0;
  gapc_util_point_filter_set (&(ppi->phs.sq[1]), dValue);
  pbar = g_hash_table_lookup (ppi->pht_Status, "HBar4");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text),
              "<span foreground=\"blue\" style=\"italic\">"
              "%s"
              "</span>", pch);

  w = g_hash_table_lookup (ppi->pht_Widgets, "HBar4-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  pch = g_hash_table_lookup (ppi->pht_Status, "TIMELEFT");
  dValue = g_strtod (pch, NULL);
  dScale = dValue / (1 - dtmp);
  dValue /= dScale;
  gapc_util_point_filter_set (&(ppi->phs.sq[2]), dValue);
  pbar = g_hash_table_lookup (ppi->pht_Status, "HBar5");
  pbar->d_value = dValue;
  g_snprintf (pbar->c_text, sizeof (pbar->c_text),
              "<span foreground=\"blue\" style=\"italic\">"
              "%s Remaining"
              "</span>", pch);
  
  w = g_hash_table_lookup (ppi->pht_Widgets, "HBar5-Widget");
  if (GTK_WIDGET_DRAWABLE (w))
    gdk_window_invalidate_rect (w->window, &pbar->rect, FALSE);

  /*
   * information window update */
  win = g_hash_table_lookup (ppi->pht_Widgets, "SoftwareInformation");
  pch = g_hash_table_lookup (ppi->pht_Status, "VERSION");
  pch1 = g_hash_table_lookup (ppi->pht_Status, "UPSNAME");
  pch2 = g_hash_table_lookup (ppi->pht_Status, "CABLE");
  pch3 = g_hash_table_lookup (ppi->pht_Status, "UPSMODE");
  pch4 = g_hash_table_lookup (ppi->pht_Status, "STARTTIME");
  pch5 = g_hash_table_lookup (ppi->pht_Status, "STATUS");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
              "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s\n%s" "</span>",
              (pch != NULL) ? pch : "N/A", (pch1 != NULL) ? pch1 : "N/A",
              (pch2 != NULL) ? pch2 : "N/A", (pch3 != NULL) ? pch3 : "N/A",
              (pch4 != NULL) ? pch4 : "N/A", (pch5 != NULL) ? pch5 : "N/A");
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (ppi->pht_Widgets, "PerformanceSummary");
  pch = g_hash_table_lookup (ppi->pht_Status, "SELFTEST");
  pch1 = g_hash_table_lookup (ppi->pht_Status, "NUMXFERS");
  pch2 = g_hash_table_lookup (ppi->pht_Status, "LASTXFER");
  pch3 = g_hash_table_lookup (ppi->pht_Status, "XONBATT");
  pch4 = g_hash_table_lookup (ppi->pht_Status, "XOFFBATT");
  pch5 = g_hash_table_lookup (ppi->pht_Status, "TONBATT");
  pch6 = g_hash_table_lookup (ppi->pht_Status, "CUMONBATT");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
              "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s\n%s\n%s"
              "</span>", (pch != NULL) ? pch : "N/A",
              (pch1 != NULL) ? pch1 : "N/A", (pch2 != NULL) ? pch2 : "N/A",
              (pch3 != NULL) ? pch3 : "N/A", (pch4 != NULL) ? pch4 : "N/A",
              (pch5 != NULL) ? pch5 : "N/A", (pch6 != NULL) ? pch6 : "N/A");
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  win = g_hash_table_lookup (ppi->pht_Widgets, "ProductInformation");
  pch = g_hash_table_lookup (ppi->pht_Status, "MODEL");
  pch1 = g_hash_table_lookup (ppi->pht_Status, "SERIALNO");
  pch2 = g_hash_table_lookup (ppi->pht_Status, "MANDATE");
  pch3 = g_hash_table_lookup (ppi->pht_Status, "FIRMWARE");
  pch4 = g_hash_table_lookup (ppi->pht_Status, "BATTDATE");
  g_snprintf (ch_buffer, sizeof (ch_buffer),
              "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s" "</span>",
              (pch != NULL) ? pch : "N/A", (pch1 != NULL) ? pch1 : "N/A",
              (pch2 != NULL) ? pch2 : "N/A", (pch3 != NULL) ? pch3 : "N/A",
              (pch4 != NULL) ? pch4 : "N/A");
  gtk_label_set_markup (GTK_LABEL (win), ch_buffer);

  g_mutex_unlock (ppi->gm_update);

  return TRUE;
}

/*
 * Saves the gconf instance preferences for this program
 * returns FALSE if loaded sucessful
 * returns TRUE  if error occurs
*/
static gboolean gapc_preferences_save (PGAPC_INSTANCE ppi)
{
  GError         *gerror = NULL;
  gboolean        b_rc = FALSE;
  gchar          *phost    = NULL;
  gchar          *pport    = NULL;
  gchar          *prefresh = NULL;
  gchar          *penabled = NULL;
  gint           i_instance = 0, i_monitor = 0;


  g_return_val_if_fail (ppi != NULL, TRUE);
  
  i_instance = ppi->cb_instance_num;
  i_monitor  = ppi->cb_monitor_num;
  
  phost    = g_strdup_printf (GAPC_HOST_KEY, i_instance, i_monitor);
  pport    = g_strdup_printf (GAPC_PORT_KEY, i_instance, i_monitor);
  prefresh = g_strdup_printf (GAPC_REFRESH_KEY, i_instance, i_monitor);
  penabled = g_strdup_printf (GAPC_ENABLE_KEY, i_instance, i_monitor);

  gconf_client_set_string (ppi->client, phost, ppi->pch_host, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_save",
                             "Save Host Preference Failed", gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
  }
  gconf_client_set_int (ppi->client, pport, ppi->i_port, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_save",
                             "Save Port Preference Failed", gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
  }
  gconf_client_set_float (ppi->client, prefresh, ppi->d_refresh, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_save",
                             "Save Refresh Interval Preference Failed",
                             gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
  }
  gconf_client_set_bool (ppi->client, penabled, ppi->cb_enabled, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_save",
                             "Save enabled state of Monitor Preference Failed",
                             gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
  }

  if (ppi->cb_monitor_num == GAPC_MONITOR_MAX - 1)
  {
      gconf_client_suggest_sync (ppi->client, &gerror);
      if (gerror != NULL)
      {
        gapc_util_log_app_error ("gapc_preferences_save",
                                 "gconf_client_suggest_sync() Failed",
                                 gerror->message);
        g_error_free (gerror);
        gerror = NULL;
        b_rc = TRUE;
      }
  }
  
  g_free (phost);
  g_free (pport);
  g_free (prefresh);
  g_free (penabled);

  return b_rc;
}

/*
 * Load the gconf instance preferences for this program
 * returns FALSE if loaded sucessful
 * returns TRUE if defaults where applied or error occurs
*/
static gboolean gapc_preferences_model_data_load (PGAPC_INSTANCE ppi)
{
  GError         *gerror   = NULL;
  gboolean        b_rc     = FALSE;
  gchar          *phost    = NULL;
  gchar          *pport    = NULL;
  gchar          *prefresh = NULL;
  gchar          *penabled = NULL;
  gint           i_instance = 0, i_monitor = 0;


  g_return_val_if_fail (ppi != NULL, TRUE);
  
  i_instance = ppi->cb_instance_num;
  i_monitor  = ppi->cb_monitor_num;
  
  phost    = g_strdup_printf (GAPC_HOST_KEY, i_instance, i_monitor);
  pport    = g_strdup_printf (GAPC_PORT_KEY, i_instance, i_monitor);
  prefresh = g_strdup_printf (GAPC_REFRESH_KEY, i_instance, i_monitor);
  penabled = g_strdup_printf (GAPC_ENABLE_KEY, i_instance, i_monitor);

  if (ppi->pch_host != NULL)
  {
      g_free (ppi->pch_host);
  }
  
  ppi->pch_host = gconf_client_get_string (ppi->client, phost, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_model_data_load",
                             "Load Host Preference Failed", gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
    if (ppi->pch_host != NULL)
      g_free (ppi->pch_host);

    ppi->pch_host = g_strdup (GAPC_HOST_VALUE_DEFAULT);
  }
  if (ppi->pch_host == NULL)
  {
    ppi->pch_host = g_strdup (GAPC_HOST_VALUE_DEFAULT);
    b_rc = TRUE;
  }

  ppi->i_port = gconf_client_get_int (ppi->client, pport, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_model_data_load",
                             "Load Port Preference Failed", gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
    ppi->i_port = GAPC_PORT_VALUE_DEFAULT;
  }
  if (ppi->i_port == 0)
  {
    ppi->i_port = GAPC_PORT_VALUE_DEFAULT;
    b_rc = TRUE;
  }

  ppi->d_refresh =
          (gdouble) gconf_client_get_float (ppi->client, prefresh, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_model_data_load",
                             "Load Refresh Interval Preference Failed",
                             gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
    ppi->d_refresh = 15.0;
  }
  if (ppi->d_refresh == 0.0)
  {
    ppi->d_refresh = 10.0;
    b_rc = TRUE;
  }

  ppi->cb_enabled = gconf_client_get_bool (ppi->client, penabled, &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_model_data_load",
                             "Load enabled state of monitor Preference Failed",
                             gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    b_rc = TRUE;
    ppi->cb_enabled = FALSE;
  }

  g_free (phost);
  g_free (pport);
  g_free (prefresh);
  g_free (penabled);

  return b_rc;
}

/*
 * Create a model.  We are using the list store model for now, though we
 * could use any other GtkTreeModel -- we keep the model once created
 * also setup the  charting structure with baseline values
*/
static GtkTreeModel *gapc_preferences_model_data_init (PGAPC_CONFIG pcfg)
{
  GtkTreeModel   *model = NULL;
  GtkTreeIter     iter;
  gint            v_index = 0, h_index = 0;
  PGAPC_INSTANCE  ppi = NULL;

  /*
   * Don't create it twice */
  if (pcfg->prefs_model != NULL)
    return pcfg->prefs_model;

  /*
   * Create the model  */
  pcfg->prefs_model = 
        model = GTK_TREE_MODEL (gtk_list_store_new (5, G_TYPE_INT, /* Monitor */
                                                    G_TYPE_STRING, /* Host  */
                                                    G_TYPE_INT,    /* Port  */
                                                    G_TYPE_FLOAT,  /* Refresh */
                                                    G_TYPE_BOOLEAN /* enabled */
                                                    ));

  /*
   * Load the model with data */
  for (v_index = 0; v_index < GAPC_MONITOR_MAX; v_index++)
  {
    ppi = (PGAPC_INSTANCE) & (pcfg->ppi[v_index]); /* initialize panel data */

    ppi->prefs_model = model;
    ppi->client = pcfg->client;
    ppi->cb_instance_num = pcfg->cb_instance_num;
    ppi->cb_monitor_num = v_index;
    ppi->applet = pcfg->applet;
    ppi->gp = (gpointer) pcfg;
    ppi->phs.cb_instance_num = pcfg->cb_instance_num;
    ppi->phs.cb_monitor_num = v_index;
    ppi->gm_update = g_mutex_new ();
    ppi->b_data_available = FALSE;   
    ppi->b_window_visible = FALSE;
    ppi->b_run = FALSE;
    ppi->b_network_changed = TRUE;
    ppi->phs.timer_id = 0;
    
    for ( h_index = 0; h_index < GAPC_HISTORY_CHART_SERIES; h_index++)
    {
          if (ppi->phs.sq[h_index].gm_graph != NULL )
              g_mutex_free (ppi->phs.sq[h_index].gm_graph);

          ppi->phs.sq[h_index].gm_graph = g_mutex_new ();
    }
    

    gapc_preferences_model_data_load (ppi);

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
                        COLUMN_ENABLED, ppi->cb_enabled, 
                        COLUMN_MONITOR, ppi->cb_monitor_num + 1, 
                        COLUMN_HOST, ppi->pch_host,
                        COLUMN_PORT, ppi->i_port, 
                        COLUMN_REFRESH, ppi->d_refresh,
                        -1);
    /*
     * Create hash table for easy access to status info and widgets   */
    ppi->pht_Status =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    ppi->pht_Widgets =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  }

  return model;
}

/* 
 * Force enablement of the first monitor in the preference list
*/
static gint gapc_preferences_model_enable_one ( PGAPC_CONFIG pcfg )
{
  gboolean b_enabled = TRUE, brc = TRUE;
  gint     i_monitor = 0;
  GtkTreeIter iter;
  
  brc= gtk_tree_model_get_iter_first (GTK_TREE_MODEL(pcfg->prefs_model), &iter);
  gtk_tree_model_get (pcfg->prefs_model, &iter, COLUMN_ENABLED, &b_enabled,
                      COLUMN_MONITOR, &i_monitor, -1);

  i_monitor -= 1;               /* move value to zero base */
  b_enabled = TRUE;  

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (pcfg->prefs_model), &iter, COLUMN_ENABLED,
                      b_enabled, -1);

  pcfg->ppi[i_monitor].cb_enabled = TRUE;
  gapc_preferences_save ((PGAPC_INSTANCE) & (pcfg->ppi[i_monitor]));

  return i_monitor;                                           
}

/*
 * Gets the gconf instance preferences for this program
 * returns FALSE if loaded sucessful
 * returns TRUE if defaults used and save failed
*/
static gboolean gapc_preferences_init (PGAPC_CONFIG pcfg)
{
  GError         *gerror = NULL;
  gboolean        b_rc = FALSE;
  gchar          *pgroup = NULL;

  g_return_val_if_fail (pcfg != NULL, TRUE);

  pgroup =  g_strdup_printf (GAPC_GROUP_KEY, pcfg->cb_instance_num);

  pcfg->client = gconf_client_get_default ();

  gapc_preferences_model_data_init (pcfg);

  /*
   * count number enabled now */
  pcfg->cb_monitors = gapc_monitor_interface_count_enabled (pcfg->prefs_model);
  if (pcfg->cb_monitors == 0)
  {
    gapc_preferences_model_enable_one ( pcfg );
    pcfg->cb_monitors = 1;
    b_rc = TRUE;
  }
  if (pcfg->cb_monitors > GAPC_MONITOR_MAX)
  {
      pcfg->cb_monitors = GAPC_MONITOR_MAX;
  }

  gconf_client_add_dir (pcfg->client, pgroup, GCONF_CLIENT_PRELOAD_RECURSIVE,
                        &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_init",
                             "gconf_client_add_dir() Failed", gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    g_free (pgroup);

    return TRUE;
  }

  pcfg->i_group_id =
          gconf_client_notify_add (pcfg->client, pgroup,
                                   (GConfClientNotifyFunc)
                                   cb_monitor_preferences_changed, pcfg, NULL,
                                   &gerror);
  if (gerror != NULL)
  {
    gapc_util_log_app_error ("gapc_preferences_init",
                             "gconf_client_notify_add(group) Failed",
                             gerror->message);
    g_error_free (gerror);
    gerror = NULL;
    g_free (pgroup);

    return TRUE;
  }

  g_free (pgroup);

  return b_rc;
}

/*
 *  capture the current application related error values
 *  setup a one-shot timer to handle output of message on
 *  the main thread (i.e. this may be a background thread)
*/
static void gapc_util_log_app_error (gchar * pch_func, gchar * pch_topic,
                                     gchar * pch_emsg)
{
  gchar          *pch = NULL;

  g_return_if_fail (pch_func != NULL);

  pch = g_strdup_printf ("%s(%s) emsg=%s", pch_func, pch_topic, pch_emsg);

  g_message (pch);

  g_free (pch);

  return;
}

/*
 * Load ICONs and set default icon
 * return TRUE if ok, FALSE otherwise
*/
static gboolean gapc_util_load_icons (PGAPC_CONFIG pcfg)
{
  guint           i_x = 0;
  GError         *gerror = NULL;
  gboolean        b_rc = TRUE;

  gchar          *pch_image_names[] = {
    "/usr/share/pixmaps/online.png",
    "/usr/share/pixmaps/onbatt.png",
    "/usr/share/pixmaps/charging.png",
    "/usr/share/pixmaps/apcupsd.png",
    "/usr/share/pixmaps/unplugged.png",    
    NULL
  };

  g_return_val_if_fail (pcfg != NULL, FALSE);

  for (i_x = 0; (pch_image_names[i_x] != NULL) && (i_x < GAPC_N_ICONS); i_x++)
  {
    pcfg->my_icons[i_x] =
            gdk_pixbuf_new_from_file (pch_image_names[i_x], &gerror);
            
    if (gerror != NULL)
    {
      gchar          *pch = NULL;

      pch = g_strdup_printf ("Get Icon=%s Failed", pch_image_names[i_x]);
      gapc_util_log_app_error ("gapc_applet_create", pch, gerror->message);
      g_error_free (gerror);
      g_free (pch);
      gerror = NULL;
      b_rc = FALSE;
    }
  }

  for (i_x = 0; i_x < GAPC_MONITOR_MAX; i_x++)
  {
    pcfg->ppi[i_x].i_old_icon_index = GAPC_N_ICONS;
    pcfg->ppi[i_x].i_icon_index = GAPC_ICON_DEFAULT;
  }

  return b_rc;
}

/*
 * Changes the Applets icon if needed
 * returns FALSE if OK
 * return TRUE is any errot
*/
static gint gapc_util_change_icons (PGAPC_INSTANCE ppi, gint size)
{
  GdkPixbuf      *pixbuf;
  GdkPixbuf      *scaled;
  GtkWidget      *window;
  PGAPC_CONFIG    pcfg = (PGAPC_CONFIG) ppi->gp;

  g_return_val_if_fail (ppi != NULL, TRUE);

  window = g_hash_table_lookup (ppi->pht_Widgets, "InformationWindow");

  if (ppi->i_icon_index >= GAPC_N_ICONS)
  {
    ppi->i_icon_index = GAPC_ICON_ONLINE;
  } else if (ppi->i_old_icon_index == ppi->i_icon_index)
         {
            return FALSE;
         }

  ppi->i_old_icon_index = ppi->i_icon_index; /* save it for next time */

  pixbuf = pcfg->my_icons[ppi->i_icon_index];
  if (pixbuf)
  {
    scaled = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);

    gtk_image_set_from_pixbuf (GTK_IMAGE (ppi->image), scaled);
    gtk_widget_show (ppi->image);

    gdk_pixbuf_unref (scaled);
    if (window != NULL)
      gtk_window_set_icon (GTK_WINDOW (window), pixbuf);
  }

  return FALSE;
}

/*
 * Destroys a single monitor icon set from  ppbox
 * True if Done, False if error  or monitor is enabled
*/
static gboolean gapc_monitor_interface_remove (PGAPC_INSTANCE ppi)
{
  gboolean        b_rc = TRUE;
  PGAPC_CONFIG    pcfg = (PGAPC_CONFIG) ppi->gp;

  g_return_val_if_fail (ppi != NULL, FALSE);

  if (ppi->cb_enabled == TRUE)
    return FALSE;
  
  pcfg->cb_monitors--;

  /*
   * stop History Page refresh*/
  if (ppi->phs.timer_id)
  {
    g_source_remove (ppi->phs.timer_id);   
    ppi->phs.timer_id = 0;
  }
  if ( ppi->phs.glg != NULL )
  {  
      if (ppi->phs.glg->tooltip_id)
      {
        g_source_remove (ppi->phs.glg->tooltip_id);    
        ppi->phs.glg->tooltip_id = 0;
      }
      if (ppi->phs.glg->expose_id)
      {
        g_source_remove (ppi->phs.glg->expose_id);        
        ppi->phs.glg->expose_id = 0;
      }
  }
  /* end History Page
   */

  ppi->b_run = FALSE;
  b_rc = g_source_remove (ppi->tid_automatic_refresh); /*stop timers */

  if (ppi->window != NULL)
  {
    gtk_widget_destroy (ppi->window);
    ppi->window = NULL;
  }

  gtk_widget_destroy (ppi->evbox); /* kill widgets including tooltip */

  gdk_flush ();

  ppi->evbox = NULL;
  ppi->tid_automatic_refresh = 0;

  g_free (ppi->tooltips);
  ppi->tooltips = NULL;

  return b_rc;
}

/*
 * Creates a single monitor icon set and places it into ppbox
 * return True if OK, False if error or not enabled
*/
static gboolean gapc_monitor_interface_create (PGAPC_INSTANCE ppi)
{
  GdkPixbuf      *scaled = NULL;
  gchar          *pch = NULL;
  PGAPC_CONFIG    pcfg = (PGAPC_CONFIG) ppi->gp;
  gboolean        b_rc = TRUE;

  if (ppi->cb_enabled == FALSE) 
    return FALSE;

  ppi->b_run = TRUE;
  ppi->b_network_changed = TRUE;
  pcfg->cb_monitors++; 
  ppi->phs.gp = (gpointer)ppi;
  ppi->i_old_icon_index = GAPC_N_ICONS;
  ppi->i_icon_index = GAPC_ICON_DEFAULT;  
  ppi->evbox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (pcfg->ppbox), ppi->evbox, TRUE, TRUE, 0);
  g_signal_connect (ppi->evbox, "button-press-event",
                    G_CALLBACK (cb_monitor_handle_icon_clicked), ppi);

  ppi->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (ppi->evbox), ppi->image);

  scaled = gdk_pixbuf_scale_simple (pcfg->my_icons[GAPC_ICON_DEFAULT], 24, 24,
                                    GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (ppi->image), scaled);
  gtk_widget_show (ppi->image);
  gdk_pixbuf_unref (scaled);

  if (ppi->tooltips)
  {
      g_free (ppi->tooltips);
  }
  ppi->tooltips = gtk_tooltips_new ();
  pch = g_strdup_printf("Monitor(%d) Waiting for Refresh", ppi->cb_monitor_num);
  gtk_tooltips_set_tip (ppi->tooltips, GTK_WIDGET (ppi->evbox), pch, NULL);
  g_free (pch);
                         
  g_async_queue_push (pcfg->q_network, (gpointer) ppi);

  ppi->tid_automatic_refresh =
          g_timeout_add ((guint) (ppi->d_refresh * GAPC_REFRESH_FACTOR_1K),
                         (GSourceFunc) cb_monitor_automatic_refresh, ppi);

  gtk_widget_show_all (GTK_WIDGET (ppi->evbox));

  g_timeout_add (250, (GSourceFunc) cb_monitor_dedicated_one_time_refresh, ppi);

  return b_rc;
}

/*
 * Create cb_num_monitors icons for applet panel display
*/
static GtkWidget *gapc_applet_interface_create (PGAPC_CONFIG pcfg)
{
  guint           i_x = 0;
  GtkWidget      *ppbox = NULL, *ppframe = NULL;
  PGAPC_INSTANCE  ppi = NULL;


  g_return_val_if_fail (pcfg != NULL, NULL);

  /*
   * Start the central network thread */
  pcfg->q_network = g_async_queue_new ();
  g_return_val_if_fail (pcfg->q_network != NULL, NULL);

  pcfg->b_run = TRUE;
  pcfg->tid_thread_qwork =
         g_thread_create ((GThreadFunc)gapc_net_thread_qwork, pcfg, TRUE, NULL);

  ppframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (ppframe), GTK_SHADOW_NONE);
      gtk_container_add (GTK_CONTAINER (pcfg->applet), ppframe);
      gtk_widget_show (GTK_WIDGET(ppframe));

  pcfg->orientation = panel_applet_get_orient (PANEL_APPLET (pcfg->applet));
  switch (pcfg->orientation)
  {
    case PANEL_APPLET_ORIENT_LEFT:
    case PANEL_APPLET_ORIENT_RIGHT:
      ppbox = gtk_vbox_new (TRUE, 1);
      break;
    case PANEL_APPLET_ORIENT_DOWN:
    case PANEL_APPLET_ORIENT_UP:
    default:
      ppbox = gtk_hbox_new (TRUE, 3);
      break;
  }
  gtk_container_add (GTK_CONTAINER (ppframe), ppbox);
  pcfg->ppframe = ppframe;
  pcfg->ppbox = ppbox;
  
  /*
   * Create the monitors that are enabled */
  for (i_x = 0; i_x < GAPC_MONITOR_MAX; i_x++)
  {
    ppi = (PGAPC_INSTANCE) & (pcfg->ppi[i_x]);

    if (ppi->cb_enabled)
    {
      gapc_monitor_interface_create (ppi);
    }
  }

  return (ppframe);
}

/*
 * populates the applet panel and associated windows
 * This is the efffective main() routine
*/
static gboolean gapc_applet_create (PanelApplet * applet, PGAPC_CONFIG pcfg)
{
  const BonoboUIVerb gapc_applet_menu_verbs[] = {
    BONOBO_UI_UNSAFE_VERB ("gapc_about", cb_applet_menu_verbs),
    BONOBO_UI_UNSAFE_VERB ("gapc_preferences", cb_applet_menu_verbs),
    BONOBO_UI_VERB_END          /* Popup menu on the applet */
  };

  const gchar     gp_apcmon_context_menu_xml[] =
          "<popup name=\"button3\">\n"
          " <menuitem name=\"About Item\" verb=\"gapc_about\""
          " _label=\"_About ...\"\n"
          "   pixtype=\"stock\" pixname=\"gnome-stock-about\"/>\n"
          " <menuitem name=\"Preferences\" verb=\"gapc_preferences\""
          " _label=\"_Preferences ...\"\n"
          "   pixtype=\"stock\" pixname=\"gtk-preferences\"/>\n" "</popup>\n";
  GtkWidget      *result = NULL;

  gapc_util_load_icons (pcfg);

  result = gapc_applet_interface_create (pcfg);
  if (result == NULL)
    return FALSE;               /* indicate a failure */

  panel_applet_setup_menu (PANEL_APPLET (applet), gp_apcmon_context_menu_xml,
                           gapc_applet_menu_verbs, pcfg);

  g_signal_connect (pcfg->applet, "change-size",
                    G_CALLBACK (cb_applet_change_size), pcfg);
  g_signal_connect (pcfg->applet, "change-orient",
                    G_CALLBACK (cb_applet_change_orientation), pcfg);
  g_signal_connect (pcfg->applet, "destroy", 
                    G_CALLBACK (cb_applet_destroy), pcfg);

  gtk_widget_show (GTK_WIDGET (applet));
  gtk_widget_show_all (GTK_WIDGET (pcfg->ppframe));

  return TRUE;
}

/*
 * Entry routine from panel control object
 * This routine initializes data structs and determine if starting an/another
 * instance is appropiate.  Called by the - add to panel - user action
*/
static gboolean gapc_applet_factory (PanelApplet * applet, const gchar * iid,
                                     gpointer data)
{
  static guint    i_instance_count = 0;
  PGAPC_CONFIG    pcfg = NULL;
  gboolean        b_rc = FALSE;

  if (g_str_equal (iid, GAPC_PROG_IID))
  {
    pcfg = g_new0 (GAPC_CONFIG, 1);

    g_return_val_if_fail ( pcfg != NULL, FALSE);

    pcfg->cb_instance_num = i_instance_count;
    pcfg->applet = applet;

    gapc_preferences_init (pcfg);

    if ( (b_rc = gapc_applet_create (applet, pcfg)) )
    {
      i_instance_count++;
    }
    
    return b_rc;
  }

  return FALSE;
}

PANEL_APPLET_BONOBO_FACTORY (GAPC_PROG_IID_FACTORY, PANEL_TYPE_APPLET,
                             GAPC_PROG_NAME, GAPC_VERSION, gapc_applet_factory,
                             NULL)
