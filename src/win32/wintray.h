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

// The tray menu class itself
class upsMenu
{
public:
   upsMenu(HINSTANCE appinst);
   ~upsMenu();

protected:
   // Tray icon handling
   void AddTrayIcon();
   void DelTrayIcon();
   void UpdateTrayIcon();
   void SendTrayMsg(DWORD msg);

   // Message handler for the tray window
   static LRESULT CALLBACK WndProc(
      HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

   // Dialogs for About, Status, and Events
   upsAbout                m_about;
   upsStatus               m_status;
   upsEvents               m_events;

   HWND                    m_hwnd;
   HMENU                   m_hmenu;
   NOTIFYICONDATA          m_nid;
   UINT                    m_balloon_timer;

   // The icon handles
   HICON                   m_online_icon;
   HICON                   m_onbatt_icon;
   HICON                   m_charging_icon;
   HICON                   m_commlost_icon;
};


#endif // WINTRAY_H
