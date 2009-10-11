// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald


// This class handles creation of a system-tray icon & menu

#ifndef WINTRAY_H
#define WINTRAY_H

#include <windows.h>
#include "winabout.h"
#include "winstat.h"
#include "winevents.h"
#include "winconfig.h"
#include "astring.h"
#include "instmgr.h"
#include "amutex.h"

// Forward declarations
class StatMgr;
class BalloonMgr;

// The tray menu class itself
class upsMenu
{
public:
   upsMenu(HINSTANCE appinst, MonitorConfig &mcfg, BalloonMgr *balmgr,
           InstanceManager *instmgr);
   ~upsMenu();
   void Destroy();
   void Redraw();
   void Reconfigure(const MonitorConfig &mcfg);

protected:
   // Tray icon handling
   void AddTrayIcon();
   void DelTrayIcon();
   void UpdateTrayIcon();
   void SendTrayMsg(DWORD msg);

   // Message handler for the tray window
   static LRESULT CALLBACK WndProc(
      HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

   // Fetch UPS status info
   bool FetchStatus(int &battstat, astring &statstr, astring &upsname);

   // Thread to poll for UPS status changes
   static DWORD WINAPI StatusPollThread(LPVOID param);

   HWND                    m_hwnd;           // Window handle
   HMENU                   m_hmenu;          // Menu handle
   StatMgr                *m_statmgr;        // Manager for UPS stats
   HANDLE                  m_thread;         // Handle to status polling thread
   HANDLE                  m_wait;           // Handle to wait mutex
   astring                 m_upsname;        // Cache UPS name
   astring                 m_laststatus;     // Cache previous status string
   BalloonMgr             *m_balmgr;         // Balloon tip manager
   UINT                    m_tbcreated_msg;  // Id of TaskbarCreated message
   HINSTANCE               m_appinst;        // Application instance handle
   MonitorConfig           m_config;         // Configuration (host, port, etc.)
   bool                    m_runthread;      // Run the poll thread?
   amutex                  m_mutex;          // Lock to protect statmgr
   WPARAM                  m_generation;

   // Dialogs for About, Status, Config, and Events
   upsAbout                m_about;
   upsStatus               m_status;
   upsConfig               m_configdlg;
   upsEvents               m_events;

   // The icon handles
   HICON                   m_online_icon;
   HICON                   m_onbatt_icon;
   HICON                   m_charging_icon;
   HICON                   m_commlost_icon;
};

#endif // WINTRAY_H
