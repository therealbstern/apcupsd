/*
 * Copyright (C) 2009 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

// Implementation of the Config dialog

#include <windows.h>
#include <commctrl.h>
#include "winconfig.h"
#include "resource.h"
#include "instmgr.h"

// Constructor/destructor
upsConfig::upsConfig(HINSTANCE appinst, InstanceManager *instmgr) :
   m_hwnd(NULL),
   m_appinst(appinst),
   m_instmgr(instmgr)
{
}

upsConfig::~upsConfig()
{
}

// Dialog box handling functions
void upsConfig::Show(MonitorConfig &mcfg)
{
   if (!m_hwnd)
   {
      m_config = mcfg;
      m_hostvalid = true;
      m_portvalid = true;
      m_refreshvalid = true;

      DialogBoxParam(m_appinst,
                     MAKEINTRESOURCE(IDD_CONFIG),
                     NULL,
                     (DLGPROC)DialogProc,
                     (LONG)this);
   }
}

BOOL CALLBACK upsConfig::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   upsConfig *_this;

   // Retrieve virtual 'this' pointer. When we come in here the first time for
   // the WM_INITDIALOG message, the pointer is in lParam. We then store it in
   // the user data so it can be retrieved on future calls.
   if (uMsg == WM_INITDIALOG)
   {
      // Set dialog user data to our "this" pointer which comes in via lParam.
      // On subsequent calls, this will be retrieved by the code below.
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsConfig *)lParam;
   }
   else
   {
      // We've previously been initialized, so retrieve pointer from user data
      _this = (upsConfig *)GetWindowLong(hwnd, GWL_USERDATA);
   }

   // Call thru to non-static member function
   return _this->DialogProcess(hwnd, uMsg, wParam, lParam);
}

BOOL upsConfig::DialogProcess(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   char tmp[256] = {0};

   switch (uMsg) {
   case WM_INITDIALOG:
      // Save a copy of our window handle for later use
      m_hwnd = hwnd;

      // Fetch handles for all controls. We'll use these multiple times later
      // so it makes sense to cache them.
      m_hhost = GetDlgItem(hwnd, IDC_HOSTNAME);
      m_hport = GetDlgItem(hwnd, IDC_PORT);
      m_hrefresh = GetDlgItem(hwnd, IDC_REFRESH);
      m_hpopups = GetDlgItem(hwnd, IDC_POPUPS);

      // Initialize fields with current config settings
      SendMessage(m_hhost, WM_SETTEXT, 0, (LONG)m_config.host.str());
      snprintf(tmp, sizeof(tmp), "%d", m_config.port);
      SendMessage(m_hport, WM_SETTEXT, 0, (LONG)tmp);
      snprintf(tmp, sizeof(tmp), "%d", m_config.refresh);
      SendMessage(m_hrefresh, WM_SETTEXT, 0, (LONG)tmp);
      SendMessage(m_hpopups, BM_SETCHECK, 
         m_config.popups ? BST_CHECKED : BST_UNCHECKED, 0);

      // Show the dialog
      SetForegroundWindow(hwnd);
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDOK:
      {
         // Fetch and validate hostname
         SendMessage(m_hhost, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         astring host(tmp);
         m_hostvalid = !host.trim().empty();

         // Fetch and validate port
         SendMessage(m_hport, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         int port = atoi(tmp);
         m_portvalid = (port >= 1 && port <= 65535);

         // Fetch and validate refresh
         SendMessage(m_hrefresh, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         int refresh = atoi(tmp);
         m_refreshvalid = (refresh >= 1);

         // Fetch popups on/off
         bool popups = SendMessage(m_hpopups, BM_GETCHECK, 0, 0) == BST_CHECKED;

         if (m_hostvalid && m_portvalid && m_refreshvalid)
         {
            // Config is valid: Save it and close the dialog
            m_config.host = host;
            m_config.port = port;
            m_config.refresh = refresh;
            m_config.popups = popups;
            m_instmgr->UpdateInstance(m_config);
            EndDialog(hwnd, TRUE);
         }
         else
         {
            // Set keyboard focus to first invalid field
            if (!m_hostvalid)      SetFocus(m_hhost);
            else if (!m_portvalid) SetFocus(m_hport);
            else                   SetFocus(m_hrefresh);

            // Force redraw to get background color change
            RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT|RDW_INVALIDATE);
         }
         return TRUE;
      }

      case IDCANCEL:
         // Close the dialog
         EndDialog(hwnd, TRUE);
         return TRUE;
      }
      break;

   case WM_CTLCOLOREDIT:
   {
      // Set edit control background red if data was invalid
      if (((HWND)lParam == m_hhost    && !m_hostvalid) ||
          ((HWND)lParam == m_hport    && !m_portvalid) ||
          ((HWND)lParam == m_hrefresh && !m_refreshvalid))
      {
         SetBkColor((HDC)wParam, RGB(255,0,0));
         return (BOOL)CreateSolidBrush(RGB(255,0,0));
      }
      return FALSE;
   }

   case WM_DESTROY:
      m_hwnd = NULL;
      return TRUE;
   }

   return 0;
}
