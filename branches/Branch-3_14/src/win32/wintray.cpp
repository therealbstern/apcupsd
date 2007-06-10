// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000-2005) Kern E. Sibbald

// Implementation of a system tray icon & menu for Apcupsd

#include "apc.h"
#include <windows.h>
#include "winups.h"
#include "winres.h"
#include "wintray.h"
#include "statmgr.h"
#include <string>

// Implementation
upsMenu::upsMenu(HINSTANCE appinst, char* host, unsigned long port, int refresh, bool notify)
   : m_statmgr(new StatMgr(host, port)),
     m_about(appinst),
     m_status(appinst, m_statmgr),
     m_events(appinst, m_statmgr),
     m_interval(refresh),
     m_wait(NULL),
     m_thread(NULL),
     m_hmenu(NULL),
     m_notify(notify),
     m_upsname("<unknown>")
{
   // Create a dummy window to handle tray icon messages
   WNDCLASSEX wndclass;
   wndclass.cbSize = sizeof(wndclass);
   wndclass.style = 0;
   wndclass.lpfnWndProc = upsMenu::WndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = appinst;
   wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wndclass.lpszMenuName = (const char *)NULL;
   wndclass.lpszClassName = APCTRAY_WINDOW_CLASS;
   wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
   RegisterClassEx(&wndclass);

   // If we're set to receive local apcupsd notifications, set window
   // title to APCTRAY_WINDOW_NAME (used by popup.exe). Otherwise make 
   // a unique window title as 'host:port'.
   char title[1024];
   if (notify)
      asnprintf(title, sizeof(title), "%s", APCTRAY_WINDOW_NAME);
   else
      asnprintf(title, sizeof(title), "%s:%d", host, port);

   // Create System Tray menu window
   m_hwnd = CreateWindow(APCTRAY_WINDOW_CLASS, title, WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, NULL, NULL,
                         appinst, NULL);
   if (m_hwnd == NULL) {
      PostQuitMessage(0);
      return;
   }

   // record which client created this window
   SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);

   // No balloon timer yet
   m_balloon_timer = 0;

   // Load the icons for the tray
   m_online_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_ONLINE));
   m_onbatt_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_ONBATT));
   m_charging_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_CHARGING));
   m_commlost_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_COMMLOST));

   // Load the popup menu
   m_hmenu = LoadMenu(appinst, MAKEINTRESOURCE(IDR_TRAYMENU));
   if (m_hmenu == NULL) {
      PostQuitMessage(0);
      return;
   }

   // Install the tray icon. Although it's tempting to let this happen
   // on the poll thread, we do it here so its synchronous and all icons
   // are consistently created in the same order.
   AddTrayIcon();

   // Create a locked mutex to use for interruptible waiting
   m_wait = CreateMutex(NULL, true, NULL);
   if (m_wait == NULL) {
      PostQuitMessage(0);
      return;
   }

   // Thread to poll UPS status and update tray icon
   DWORD tid;
   m_thread = CreateThread(NULL, 0, &upsMenu::StatusPollThread, this, 0, &tid);
   if (m_thread == NULL)
      PostQuitMessage(0);
}

upsMenu::~upsMenu()
{
   // Kill status polling thread
   if (m_thread) {
      if (WaitForSingleObject(m_thread, 5000) == WAIT_TIMEOUT)
         TerminateThread(m_thread, 0);
   }

   // Remove the tray icon
   SendTrayMsg(NIM_DELETE);

   // Destroy the loaded menu
   if (m_hmenu != NULL)
      DestroyMenu(m_hmenu);
}

void upsMenu::Destroy()
{
   if (m_wait)
      ReleaseMutex(m_wait);
}

void upsMenu::AddTrayIcon()
{
   SendTrayMsg(NIM_ADD);
}

void upsMenu::DelTrayIcon()
{
   SendTrayMsg(NIM_DELETE);
}

void upsMenu::UpdateTrayIcon()
{
   SendTrayMsg(NIM_MODIFY);
}

void upsMenu::SendTrayMsg(DWORD msg)
{
   // Create the tray icon message
   m_nid.hWnd = m_hwnd;
   m_nid.cbSize = sizeof(m_nid);
   m_nid.uID = IDI_APCUPSD;        // never changes after construction

   int battstat = -1;
   std::string statstr = "";

   // Get current status
   switch (msg) {
   case NIM_ADD:
   case NIM_DELETE:
      break;
   default:
      // Fetch current UPS status
      FetchStatus(battstat, statstr, m_upsname);
   }

   /* If battstat == 0 we are on batteries, otherwise we are online
    * and the value of battstat is the percent charge.
    */
   if (battstat == -1)
      m_nid.hIcon = m_commlost_icon;
   else if (battstat == 0)
      m_nid.hIcon = m_onbatt_icon;
   else if (battstat >= 100)
      m_nid.hIcon = m_online_icon;
   else
      m_nid.hIcon = m_charging_icon;

   m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
   m_nid.uCallbackMessage = WM_TRAYNOTIFY;

   // Use status as normal tooltip
   asnprintf(m_nid.szTip, sizeof(m_nid.szTip), "%s", statstr.c_str());
   m_nid.uFlags |= NIF_TIP;

   // Send the message
   if (!Shell_NotifyIcon(msg, &m_nid) && msg == NIM_ADD) {
      // The tray icon couldn't be created
      PostQuitMessage(0);
   }
}

// Process window messages
LRESULT CALLBACK upsMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   // This is a static method, so we don't know which instantiation we're 
   // dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion 
   // from a newsgroup to get the pseudo-this.
   upsMenu *_this = (upsMenu *) GetWindowLong(hwnd, GWL_USERDATA);

   switch (iMsg) {

   // Timer expired
   case WM_TIMER:
      if (wParam == 2) {
         // Balloon timer expired; clear the balloon
         KillTimer(_this->m_hwnd, _this->m_balloon_timer);

         NOTIFYICONDATA nid;
         nid.hWnd = _this->m_hwnd;
         nid.cbSize = sizeof(nid);
         nid.uID = IDI_APCUPSD;
         nid.uFlags = NIF_INFO;
         nid.uTimeout = 0;
         nid.szInfoTitle[0] = '\0';
         nid.szInfo[0] = '\0';
         nid.dwInfoFlags = 0;

         Shell_NotifyIcon(NIM_MODIFY, &nid);
      }
      break;

      // STANDARD MESSAGE HANDLING
   case WM_CREATE:
      return 0;

   case WM_COMMAND:
      // User has clicked an item on the tray menu
      switch (LOWORD(wParam)) {
      case ID_STATUS:
         // Show the status dialog
         _this->m_status.Show(TRUE);
         break;

      case ID_EVENTS:
         // Show the Events dialog
         _this->m_events.Show(TRUE);
         break;

      case ID_ABOUT:
         // Show the About box
         _this->m_about.Show(TRUE);
         break;

      case ID_CLOSEINST:
         // User selected Close from the tray menu
         PostMessage(hwnd, WM_CLOSEINST, 0, (LPARAM)_this);
         return 0;

      case ID_CLOSE:
         // User selected CloseAll from the tray menu
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;

      case ID_REMOVE:
         // User selected Close from the tray menu
         PostMessage(hwnd, WM_REMOVE, 0, (LPARAM)_this);
         return 0;

      case ID_REMOVEALL:
         // User wants to remove all apctray instances from registry
         PostMessage(hwnd, WM_REMOVEALL, 0, 0);
         return 0;

      case ID_NOTIFY:
         _this->m_notify = !_this->m_notify;
         SetWindowText(hwnd, _this->m_notify ? APCTRAY_WINDOW_NAME : "Foo");
         PostMessage(hwnd, WM_BNOTIFY, (WPARAM)_this->m_notify, (LPARAM)_this);
         return 0;
      }
      return 0;

   case WM_TRAYNOTIFY:
      // User has clicked on the tray icon or the menu
      // Get the submenu to use as a pop-up menu
      HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

      // What event are we responding to, RMB click?
      if (lParam == WM_RBUTTONUP) {
         if (submenu == NULL)
            return 0;

         // Make the Status menu item the default (bold font)
         SetMenuDefaultItem(submenu, ID_STATUS, false);

         // Set notify checkmark
         MENUITEMINFO mii;
         memset(&mii, 0, sizeof(mii));
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_STATE;
         mii.fState = _this->m_notify ? MFS_CHECKED : MFS_UNCHECKED;
         SetMenuItemInfo(submenu, ID_NOTIFY, false, &mii);

         // Set UPS name field
         ModifyMenu(submenu, ID_NAME, MF_BYCOMMAND|MF_STRING, ID_NAME,
            ("UPS: " + _this->m_upsname).c_str());

         // Get the current cursor position, to display the menu at
         POINT mouse;
         GetCursorPos(&mouse);

         // There's a "bug"
         // (Microsoft calls it a feature) in Windows 95 that requires calling
         // SetForegroundWindow. To find out more, search for Q135788 in MSDN.
         SetForegroundWindow(_this->m_nid.hWnd);

         // Display the menu at the desired position
         TrackPopupMenu(submenu, 0, mouse.x, mouse.y, 0, _this->m_nid.hWnd, NULL);

         return 0;
      }

      // Or was there a LMB double click?
      if (lParam == WM_LBUTTONDBLCLK) {
         // double click: execute the default item
         SendMessage(_this->m_nid.hWnd, WM_COMMAND, ID_STATUS, 0);
      }

      return 0;

   case WM_BALLOONSHOW:
      // A balloon notice was shown, so set a timer to clear it
      if (_this->m_balloon_timer != 0)
         KillTimer(_this->m_hwnd, _this->m_balloon_timer);
      _this->m_balloon_timer = SetTimer(_this->m_hwnd, 2, wParam, NULL);
      return 0;

   case WM_CLOSE:
   case WM_DESTROY:
      // The user wants Apctray to quit cleanly...
      PostQuitMessage(0);
      return 0;
   }

   // Unknown message type
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void upsMenu::FetchStatus(int &battstat, std::string &statstr, std::string &upsname)
{
   // Fetch data from the UPS
   if (!m_statmgr->Update()) {
      battstat = -1;
      statstr = "COMMLOST";
      return;
   }

   // Lookup the STATFLAG key
   char *statflag = m_statmgr->Get("STATFLAG");
   if (!statflag || *statflag == '\0') {
      battstat = -1;
      statstr = "COMMLOST";
      free(statflag);
      return;
   }
   unsigned long status = strtoul(statflag, NULL, 0);

   // Lookup BCHARGE key
   char *bcharge = m_statmgr->Get("BCHARGE");

   // Determine battery charge percent
   if (status & UPS_onbatt)
      battstat = 0;
   else if (bcharge && *bcharge != '\0')
      battstat = (int)atof(bcharge);
   else
      battstat = 100;

   free(statflag);
   free(bcharge);

   // Fetch UPSNAME
   char *uname = m_statmgr->Get("UPSNAME");
   if (uname)
      upsname = uname;

   // Now output status in human readable form
   statstr = "";
   if (status & UPS_calibration)
      statstr += "CAL ";
   if (status & UPS_trim)
      statstr += "TRIM ";
   if (status & UPS_boost)
      statstr += "BOOST ";
   if (status & UPS_online)
      statstr += "ONLINE ";
   if (status & UPS_onbatt)
      statstr += "ON BATTERY ";
   if (status & UPS_overload)
      statstr += "OVERLOAD ";
   if (status & UPS_battlow)
      statstr += "LOWBATT ";
   if (status & UPS_replacebatt)
      statstr += "REPLACEBATT ";
   if (!status & UPS_battpresent)
      statstr += "NOBATT ";

   // This overrides the above
   if (status & UPS_commlost) {
      statstr = "COMMLOST";
      battstat = -1;
   }

   // This overrides the above
   if (status & UPS_shutdown)
      statstr = "SHUTTING DOWN";

   // Remove trailing space, if present
   statstr.resize(statstr.find_last_not_of(" ") + 1);
}

DWORD WINAPI upsMenu::StatusPollThread(LPVOID param)
{
   upsMenu* _this = (upsMenu*)param;
   DWORD status;

   while (1) {
      // Update the tray icon
      _this->UpdateTrayIcon();

      // Delay for configured interval
      status = WaitForSingleObject(_this->m_wait, _this->m_interval * 1000);
      if (status != WAIT_TIMEOUT)
         break;
   }
}
