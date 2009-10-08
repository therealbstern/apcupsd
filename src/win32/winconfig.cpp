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
   // We use the dialog-box's USERDATA to store a _this pointer
   // This is set only once WM_INITDIALOG has been recieved, though!
   upsConfig *_this = (upsConfig *)GetWindowLong(hwnd, GWL_USERDATA);

   switch (uMsg) {
   case WM_INITDIALOG:
      // Set dialog user data to our "this" pointer which comes in via lParam.
      // On subsequent calls, this will be retrieved by the code above.
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsConfig *)lParam;

      // Save a copy of our window handle for later use
      _this->m_hwnd = hwnd;

      // Update fields
      SendDlgItemMessage(hwnd, IDC_HOSTNAME, WM_SETTEXT, 0,
         (LONG)_this->m_config.host.str());
      char tmp[128];
      snprintf(tmp, sizeof(tmp), "%d", _this->m_config.port);
      SendDlgItemMessage(hwnd, IDC_PORT, WM_SETTEXT, 0, (LONG)tmp);
      snprintf(tmp, sizeof(tmp), "%d", _this->m_config.refresh);
      SendDlgItemMessage(hwnd, IDC_REFRESH, WM_SETTEXT, 0, (LONG)tmp);

      // Show the dialog
      SetForegroundWindow(hwnd);
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDOK:
      {
         char tmp[128];
         SendDlgItemMessage(hwnd, IDC_HOSTNAME, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         _this->m_config.host = tmp;
         SendDlgItemMessage(hwnd, IDC_PORT, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         _this->m_config.port = atoi(tmp);
         SendDlgItemMessage(hwnd, IDC_REFRESH, WM_GETTEXT, sizeof(tmp), (LONG)tmp);
         _this->m_config.refresh = atoi(tmp);
         _this->m_instmgr->UpdateInstance(_this->m_config);
         EndDialog(hwnd, TRUE);
         return TRUE;
      }

      case IDCANCEL:
         // Close the dialog
         EndDialog(hwnd, TRUE);
         return TRUE;
      }
      break;

   case WM_DESTROY:
      _this->m_hwnd = NULL;
      return TRUE;
   }

   return 0;
}
