/* gapcmon.h */

#ifndef _GAPCMON_H_
#define _GAPCMON_H_
#define GAPC_MAX_ARRAY 256
#define GAPC_THREAD_CYCLE 10000000          /* 10 seconds 1M */
#define GAPC_REFRESH_INCREMENT 5000  /* 12 seconds  1K */

typedef struct _GAPC_CONFIG
{
    GHashTable *pht_Status;
    GHashTable *pht_Widgets;

    GThread *tid_refresh;
    GMutex  *gm_update;
    gboolean b_run;
    gulong   i_update_cycle;
        
    guint   ui_refresh_timer;    
    guint   i_info_context;
    
    gchar   *pch_host;
    gchar   *pch_port;
    gchar   *pch_interval;        
    gchar   *pch_config_file;   
    gchar   *pach_status[GAPC_MAX_ARRAY];
    gchar   *pach_events[GAPC_MAX_ARRAY];
    
} GAPC_CONFIG, *PGAPC_CONFIG;

#endif //_GAPCMON_H_

#ifndef   INADDR_NONE
#define   INADDR_NONE    -1
#endif
