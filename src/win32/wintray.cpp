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
#include "resource.h"
#include "wintray.h"
#include "statmgr.h"
#include "balloonmgr.h"

// Implementation
upsMenu::upsMenu(HINSTANCE appinst, MonitorConfig &mcfg, BalloonMgr *balmgr,
                 InstanceManager *instmgr)
   : m_statmgr(NULL),
     m_about(appinst),
     m_status(appinst, m_statmgr),
     m_events(appinst, m_statmgr),
     m_configdlg(appinst, instmgr),
     m_wait(NULL),
     m_thread(NULL),
     m_hmenu(NULL),
     m_upsname("<unknown>"),
     m_balmgr(balmgr),
     m_appinst(appinst),
     m_hwnd(NULL),
     m_config(mcfg),
     m_runthread(true),
     m_generation(0),
     m_reconfig(true)
{
   // Determine message id for "TaskbarCreate" message
   m_tbcreated_msg = RegisterWindowMessage("TaskbarCreated");

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

   // Make unique window title as 'host:port'.
   char title[1024];
   asnprintf(title, sizeof(title), "%s:%d", mcfg.host.str(), mcfg.port);

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

   // Create a semaphore to use for interruptible waiting
   m_wait = CreateSemaphore(NULL, 0, 1, NULL);
   if (m_wait == NULL) {
      PostQuitMessage(0);
      return;
   }

   // Thread to poll UPS status and update tray icon
   m_thread = CreateThread(NULL, 0, &upsMenu::StatusPollThread, this, 0, NULL);
   if (m_thread == NULL)
      PostQuitMessage(0);
}

upsMenu::~upsMenu()
{
   // Kill status polling thread
   if (WaitForSingleObject(m_thread, 10000) == WAIT_TIMEOUT)
      TerminateThread(m_thread, 0);
   CloseHandle(m_thread);

   // Destroy the mutex
   CloseHandle(m_wait);

   // Destroy the status manager
   delete m_statmgr;

   // Remove the tray icon
   DelTrayIcon();

   // Destroy the window
   DestroyWindow(m_hwnd);

   // Destroy the loaded menu
   DestroyMenu(m_hmenu);

   // Unregister the window class
   UnregisterClass(APCTRAY_WINDOW_CLASS, m_appinst);
}

void upsMenu::Destroy()
{
   // Trigger status poll thread to shut down. We will wait for
   // the thread to exit later in our destructor.
   m_runthread = false;
   ReleaseSemaphore(m_wait, 1, NULL);
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
   NOTIFYICONDATA nid;
   memset(&nid, 0, sizeof(nid));
   nid.hWnd = m_hwnd;
   nid.cbSize = sizeof(nid);
   nid.uID = IDI_APCUPSD;
   nid.uFlags = NIF_ICON | NIF_MESSAGE;
   nid.uCallbackMessage = WM_APCTRAY_NOTIFY;

   int battstat = -1;
   astring statstr;

   // Get current status
   switch (msg) {
   case NIM_ADD:
   case NIM_DELETE:
      // Process these messages quickly without fetching new status
      break; 
   default:
      // Fetch current UPS status
      m_statmgr->GetSummary(battstat, statstr, m_upsname);
      break;
   }

   /* If battstat == 0 we are on batteries, otherwise we are online
    * and the value of battstat is the percent charge.
    */
   if (battstat == -1)
      nid.hIcon = m_commlost_icon;
   else if (battstat == 0)
      nid.hIcon = m_onbatt_icon;
   else if (battstat >= 100)
      nid.hIcon = m_online_icon;
   else
      nid.hIcon = m_charging_icon;

   // Use status as normal tooltip
   nid.uFlags |= NIF_TIP;
   if (m_upsname == "UPS_IDEN" || m_upsname == "<unknown>")
      asnprintf(nid.szTip, sizeof(nid.szTip), "%s", statstr.str());
   else
      asnprintf(nid.szTip, sizeof(nid.szTip), "%s: %s",
                m_upsname.str(), statstr.str());

   // Display event in balloon tip
   if (m_config.popups && !m_laststatus.empty() && m_laststatus != statstr)
      m_balmgr->PostBalloon(m_hwnd, m_upsname, statstr);
   m_laststatus = statstr;

   // Send the message
   if (!Shell_NotifyIcon(msg, &nid) && msg == NIM_ADD) {
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

   // During creation, we are called before the WindowLong has been set.
   // Just use default processing in that case since _this is not valid.   
   if (!_this)
      return DefWindowProc(hwnd, iMsg, wParam, lParam);

   switch (iMsg) {

   // User has clicked an item on the tray menu
   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDM_STATUS:
         // Show the status dialog
         _this->m_status.Show();
         break;

      case IDM_EVENTS:
         // Show the Events dialog
         _this->m_events.Show();
         break;

      case IDM_ABOUT:
         // Show the About box
         _this->m_about.Show();
         break;

      case IDM_EXIT:
         // User selected Exit from the tray menu
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;

      case IDM_REMOVE:
         // User selected Remove from the tray menu
         PostMessage(hwnd, WM_APCTRAY_REMOVE, 0, (LPARAM)(_this->m_config.id.str()));
         break;

      case IDM_REMOVEALL:
         // User wants to remove all apctray instances from registry
         PostMessage(hwnd, WM_APCTRAY_REMOVEALL, 0, 0);
         break;

      case IDM_ADD:
         // User selected Add from the tray menu
         PostMessage(hwnd, WM_APCTRAY_ADD, 0, 0);
         break;

      case IDM_CONFIG:
         // User selected Config from the tray menu
         _this->m_configdlg.Show(_this->m_config);
         break;
      }
      return 0;

   // User has clicked on the tray icon or the menu
   case WM_APCTRAY_NOTIFY:
      // Get the submenu to use as a pop-up menu
      HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

      // What event are we responding to, RMB click?
      if (lParam == WM_RBUTTONUP) {
         if (submenu == NULL)
            return 0;

         // Make the Status menu item the default (bold font)
         SetMenuDefaultItem(submenu, IDM_STATUS, false);

         // Set UPS name field
         ModifyMenu(submenu, IDM_NAME, MF_BYCOMMAND|MF_STRING, IDM_NAME,
            ("UPS: " + _this->m_upsname).str());

         // Set HOST field
         char buf[100];
         asnprintf(buf, sizeof(buf), "HOST: %s:%d", _this->m_config.host.str(), _this->m_config.port);
         ModifyMenu(submenu, IDM_HOST, MF_BYCOMMAND|MF_STRING, IDM_HOST, buf);

         // Get the current cursor position, to display the menu at
         POINT mouse;
         GetCursorPos(&mouse);

         // There's a "bug" (Microsoft calls it a feature) in Windows 95 that 
         // requires calling SetForegroundWindow. To find out more, search for 
         // Q135788 in MSDN.
         SetForegroundWindow(_this->m_hwnd);

         // Display the menu at the desired position
         TrackPopupMenu(submenu, 0, mouse.x, mouse.y, 0, _this->m_hwnd, NULL);

         return 0;
      }

      // Or was there a LMB double click?
      if (lParam == WM_LBUTTONDBLCLK) {
         // double click: execute the default item
         SendMessage(_this->m_hwnd, WM_COMMAND, IDM_STATUS, 0);
      }

      return 0;

   // The user wants Apctray to quit cleanly...
   case WM_CLOSE:
      PostQuitMessage(0);
      return 0;

   default:
      if (iMsg == _this->m_tbcreated_msg) {
         // Explorer has restarted so we need to redraw the tray icon.
         // We purposely kick this out to the main loop instead of handling it
         // ourself so the icons are redrawn in a consistent order.
         PostMessage(hwnd, WM_APCTRAY_RESET, _this->m_generation++, 0);
      }
      break;
   }

   // Unknown message type
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void upsMenu::Redraw()
{
   AddTrayIcon();
}

void upsMenu::Reconfigure(const MonitorConfig &mcfg)
{
   // Indicate that a config change is pending
   m_mutex.lock();
   m_config = mcfg;
   m_reconfig = true;
   m_mutex.unlock();

   // Kick poll thread so it updates immediately
   ReleaseSemaphore(m_wait, 1, NULL);
}

DWORD WINAPI upsMenu::StatusPollThread(LPVOID param)
{
   upsMenu* _this = (upsMenu*)param;
   DWORD status;

   while (_this->m_runthread)
   {
      // Act on pending config change
      _this->m_mutex.lock();
      if (_this->m_reconfig)
      {
         // Recreate statmgr with new config
         delete _this->m_statmgr;
         _this->m_statmgr = new StatMgr(_this->m_config.host, _this->m_config.port);
         _this->m_reconfig = false;
      }
      _this->m_mutex.unlock();

      // Update the tray icon and status dialog
      _this->UpdateTrayIcon();
      _this->m_status.FillStatusBox();

      // Delay for configured interval
      status = WaitForSingleObject(_this->m_wait, _this->m_config.refresh * 1000);
   }
}
