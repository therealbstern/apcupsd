/* gapcmon.h 
 * 
 * GPL 
 * Copyright (C) 2006, James Scott, Jr. <skoona@users.sourceforge.net> 
 * 
*/

#ifndef _GAPCMON_H_
#define _GAPCMON_H_

#ifndef VERSION
  #define GAPC_VERSION "0.0.0"
#else
  #define GAPC_VERSION VERSION
#endif

#define GAPC_MAX_ARRAY         256
#define GAPC_MAX_TEXT          256
#define GAPC_THREAD_CYCLE      15000000  /* 15 sec Network update timer 1M=1 */
#define GAPC_REFRESH_INCREMENT 30000     /* 30 sec Display update timer 1K=1 */

#define GAPC_CONFIG_FILE "/.gapcmon.config.user"   /* users dir will be prepended */
#define GAPC_GROUP_KEY      "gapcmon"
#define GAPC_HOST_KEY       "Host"
#define GAPC_PORT_KEY       "Port"
#define GAPC_HOST_VALUE_DEFAULT "localhost"
#define GAPC_PORT_VALUE_DEFAULT 3551

/* NOTE:   GdkColor   color = {0,69*255,0,255*255}; // blue */

typedef struct _GAPC_H_CHART
{
    gdouble     d_value;
    gboolean    b_center_text;
    gchar       c_text[GAPC_MAX_TEXT]; 
    GdkRectangle rect;
} GAPC_BAR_H, *PGAPC_BAR_H;

typedef struct _GAPC_CONFIG
{
    GHashTable *pht_Status;
    GHashTable *pht_Widgets;
    gchar      *pch_key_filename;   
    
    GThread *tid_refresh;
    GMutex  *gm_update;
    gboolean b_run;
        
    guint   i_info_context;
    
    gchar   *pch_host;
    gint     i_port;    
    
    gchar   *pach_status[GAPC_MAX_ARRAY];
    gchar   *pach_events[GAPC_MAX_ARRAY];
    
    gboolean b_data_available;
    
} GAPC_CONFIG, *PGAPC_CONFIG;

#endif //_GAPCMON_H_
