/* gapcmon_common.h     serial-0054-7 ******************************************

  Gnome applet for monitoring the apcupsd.sourceforge.net package.
  
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

#ifndef _GP_APCMON_H_
#define _GP_APCMON_H_

#ifndef VERSION
#define GAPC_VERSION "0.0.7"
#else
#define GAPC_VERSION VERSION
#endif

#ifndef _GPANEL_APCMON_H_
#define GAPC_CONFIG_FILE "%s/.gapcmon.user.conf"	/* users dir will be prepended */
#define GAPC_GROUP_KEY      "gapcmon"
#define GAPC_GROUP_TITLE    "<i>GAPCMON</i>"
#define GAPC_WINDOW_TITLE   "gapcmon UPS Monitor"
#else
#define GAPC_CONFIG_FILE "%s/.gpanel_apcmon.instance_%d.conf"	/* users dir will be prepended */
#define GAPC_GROUP_KEY      "gpanel_apcmon"
#define GAPC_GROUP_TITLE    "<i>GPANEL_APCMON</i>"
#define GAPC_WINDOW_TITLE   "gpanel_apcmon UPS Monitor"
#endif

#define GAPC_MAX_ARRAY         256
#define GAPC_MAX_TEXT          256
#define GAPC_MIN_INCREMENT     4	/* Minimum refresh cycle seconds */
#define GAPC_THREAD_CYCLE      15000000	/* 15 sec Network update timer 1M=1 */
#define GAPC_REFRESH_INCREMENT 30000	/* 30 sec Display update timer 1K=1 */

#define GAPC_HOST_KEY       "Host_Name"
#define GAPC_PORT_KEY       "Host_Port"
#define GAPC_TIMER_KEY      "Refresh_Interval"
#define GAPC_HOST_VALUE_DEFAULT "localhost"
#define GAPC_PORT_VALUE_DEFAULT 3551

/* NOTE:   GdkColor   color = {0,69*255,0,255*255}; // blue */
enum
{
  GAPC_ICON_ONLINE,
  GAPC_ICON_ONBATT,
  GAPC_ICON_CHARGING,
  GAPC_ICON_DEFAULT,
  GAPC_ICON_UNPLUGGED,
  GAPC_N_ICONS
} GAPC_IconType;

enum
{
  GAPC_TIMER_AUTO,
  GAPC_TIMER_DEDICATED,
  GAPC_TIMER_CONTROL,
  GAPC_N_TIMERS
} GAPC_TimerIDType;


typedef struct _GAPC_H_CHART
{
  gdouble d_value;
  gboolean b_center_text;
  gchar c_text[GAPC_MAX_TEXT];
  GdkRectangle rect;
} GAPC_BAR_H, *PGAPC_BAR_H;

typedef struct _GAPC_CONFIG
{
  guint 			cb_id;
  GnomeVFSAddress 	*gapc_last_address; 
  GtkWidget 	  	*applet;
  GtkWidget 	  	*image;
  GtkWidget 		*frame;
  GtkTooltips 		*tooltips;
  GtkWindow 		*window;
  GtkDialog 		*about;  
  GdkPixbuf 		*my_icons[GAPC_N_ICONS];
  guint     		i_timer_ids[GAPC_N_TIMERS];
  guint 			i_info_context;		/* StatusBar Context */

  gchar 			*pach_status[GAPC_MAX_ARRAY];	/* Str Array holding full line of status text */
  gchar 			*pach_events[GAPC_MAX_ARRAY];	/* Str Array holding full line of event text */
  GHashTable 		*pht_Status;	/* hashtable holding status key=values */
  GHashTable 		*pht_Widgets;	/* hashtable holding wdiget ptrs  */

  gchar 			*pch_key_filename;   /* config file name */
  GThread 			*tid_refresh;		/* Background Thread */
  GMutex 			*gm_update;		/* Control mutex  for hashtables and pcfg */

  gchar 			*pch_host;		/* Configuration Value */
  gint 				i_port;			/* Configuration Value */
  gdouble 			d_refresh;		/* Configuration Value */

  gboolean 			b_run;		/* main run flag to all threads and timers */
  gboolean 			b_timer_control;	/* If true, stop running timer and restart */
  gboolean 			b_refresh_button;	/* Flag to thread to immediately service an update */
  gboolean 			b_window_visible;	/* Flag indicating if the main window is visible */
  gboolean 			b_network_changed;	/* Flag signaling a change in host or port number  */  
  gboolean 			b_data_available;	/* Flag indicating that netowrk is online */  

  gchar 			*lang;

  gint   			size;            /* icon related parms */
  guint  			i_icon_index;
  gint   			i_old_icon_index;            
} GAPC_CONFIG, *PGAPC_CONFIG;

#endif /*_GP_APCMON_H_ */
