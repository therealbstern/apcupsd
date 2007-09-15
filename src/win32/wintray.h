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
#include <string>

// Forward declarations
class StatMgr;
class BalloonMgr;

// The tray menu class itself
class upsMenu
{
public:
   upsMenu(HINSTANCE appinst, const char *host, unsigned long port,
           int refresh, BalloonMgr *balmgr);
   ~upsMenu();
   void Destroy();

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
   bool FetchStatus(int &battstat, std::string &statstr, std::string &upsname);

   // Thread to poll for UPS status changes
   static DWORD WINAPI StatusPollThread(LPVOID param);

   HWND                    m_hwnd;           // Window handle
   HMENU                   m_hmenu;          // Menu handle
   StatMgr                *m_statmgr;        // Manager for UPS stats
   int                     m_interval;       // How often to poll for status
   HANDLE                  m_thread;         // Handle to status polling thread
   HANDLE                  m_wait;           // Handle to wait mutex
   std::string             m_upsname;        // Cache UPS name
   std::string             m_laststatus;     // Cache previous status string
   BalloonMgr             *m_balmgr;         // Balloon tip manager
   const char             *m_host;
   unsigned short          m_port;
   UINT                    m_tbcreated_msg;  // Id of TaskbarCreated message
   HINSTANCE               m_appinst;        // Application instance handle

   // Dialogs for About, Status, and Events
   upsAbout                m_about;
   upsStatus               m_status;
   upsEvents               m_events;

   // The icon handles
   HICON                   m_online_icon;
   HICON                   m_onbatt_icon;
   HICON                   m_charging_icon;
   HICON                   m_commlost_icon;
};

#endif // WINTRAY_H
