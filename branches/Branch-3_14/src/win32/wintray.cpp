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
#include <arpa/inet.h>

// Remove apctray from registry autorun list
// Defined in apctray.cpp
extern int Remove();

// Implementation
upsMenu::upsMenu(HINSTANCE appinst, char* host, unsigned long port, int refresh)
   : m_statmgr(new StatMgr(host, port)),
     m_about(appinst),
     m_status(appinst, m_statmgr),
     m_events(appinst, m_statmgr),
     m_interval(refresh),
     m_wait(NULL),
     m_thread(NULL)
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

   // Determine window title
   char title[1024];
   unsigned int inaddr = inet_addr(host);
   if ((inaddr != INADDR_NONE &&
         ((ntohl(inaddr) & 0xff000000) == 0x7f000000)) ||
       strcasecmp(host, "localhost") == 0) {
      // Talking to local apcupsd: Use plain title
      astrncpy(title, APCTRAY_WINDOW_NAME, sizeof(title));
   } else {
      // Talking to remote apcuspd: Use annotated title
      asnprintf(title, sizeof(title), "%s-%s:%u",
         APCTRAY_WINDOW_NAME, host, port);
   }

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

   // Install the tray icon!
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
      ReleaseMutex(m_wait);
      if (WaitForSingleObject(m_thread, 5000) == WAIT_TIMEOUT)
         TerminateThread(m_thread, 0);
   }

   // Remove the tray icon
   SendTrayMsg(NIM_DELETE);

   // Destroy the loaded menu
   if (m_hmenu != NULL)
      DestroyMenu(m_hmenu);
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

   // Get current status
   int battstat;
   char statstr[128];
   FetchStatus(battstat, statstr, sizeof(statstr));

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
   asnprintf(m_nid.szTip, sizeof(m_nid.szTip), "Apcupsd - %s", statstr);
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

      case ID_CLOSE:
         // User selected Close from the tray menu
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;

      case ID_REMOVE:
         // User wants to remove apctray from registry
         Remove();
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;
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

         // Make the first menu item the default (bold font)
         SetMenuDefaultItem(submenu, 0, TRUE);

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
         // double click: execute first menu item
         SendMessage(_this->m_nid.hWnd, WM_COMMAND, GetMenuItemID(submenu, 0), 0);
      }

      return 0;

   case WM_BALLOONSHOW:
      // A balloon notice was shown, so set a timer to clear it
      if (_this->m_balloon_timer != 0)
         KillTimer(_this->m_hwnd, _this->m_balloon_timer);
      _this->m_balloon_timer = SetTimer(_this->m_hwnd, 2, wParam, NULL);
      return 0;

   case WM_CLOSE:
      break;

   case WM_DESTROY:
      // The user wants Apcupsd to quit cleanly...
      PostQuitMessage(0);
      return 0;
   }

   // Unknown message type
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void upsMenu::FetchStatus(int &battstat, char *statstr, int len)
{
   // Fetch data from the UPS
   if (!m_statmgr->Update()) {
      battstat = -1;
      astrncpy(statstr, "COMMLOST", len);
      return;
   }

   // Lookup the STATFLAG key
   char *statflag = m_statmgr->Get("STATFLAG");
   if (!statflag || *statflag == '\0') {
      battstat = -1;
      astrncpy(statstr, "COMMLOST", len);
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

   // Now output status in human readable form
   astrncpy(statstr, "", len);
   if (status & UPS_calibration)
      astrncat(statstr, "CAL ", len);
   if (status & UPS_trim)
      astrncat(statstr, "TRIM ", len);
   if (status & UPS_boost)
      astrncat(statstr, "BOOST ", len);
   if (status & UPS_online)
      astrncat(statstr, "ONLINE ", len);
   if (status & UPS_onbatt)
      astrncat(statstr, "ON BATTERY ", len);
   if (status & UPS_overload)
      astrncat(statstr, "OVERLOAD ", len);
   if (status & UPS_battlow)
      astrncat(statstr, "LOWBATT ", len);
   if (status & UPS_replacebatt)
      astrncat(statstr, "REPLACEBATT ", len);
   if (!status & UPS_battpresent)
      astrncat(statstr, "NOBATT ", len);

   // This overrides the above
   if (status & UPS_commlost) {
      astrncpy(statstr, "COMMLOST", len);
      battstat = -1;
   }

   // This overrides the above
   if (status & UPS_shutdown)
      astrncpy(statstr, "SHUTTING DOWN", len);

   // Remove trailing space, if present
   char *tmp = statstr + strlen(statstr) - 1;
   while (tmp >= statstr && isspace(*tmp))
      *tmp-- = '\0';
}

DWORD WINAPI upsMenu::StatusPollThread(LPVOID param)
{
   upsMenu* _this = (upsMenu*)param;
   DWORD status;

   while (1) {
      // Delay for configured interval
      status = WaitForSingleObject(_this->m_wait, _this->m_interval * 1000);
      if (status != WAIT_TIMEOUT)
         break;


      // Update the tray icon
      _this->UpdateTrayIcon();
   }
}
