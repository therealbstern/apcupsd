// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the Status dialog

#include <windows.h>
#include "winstat.h"
#include "winres.h"
#include "statmgr.h"

// Constructor/destructor
upsStatus::upsStatus(HINSTANCE appinst, StatMgr *statmgr)
{
    m_dlgvisible = FALSE;
    m_appinst = appinst;
    m_statmgr = statmgr;
}

upsStatus::~upsStatus()
{
}

// Initialisation
BOOL upsStatus::Init()
{
    return TRUE;
}

// Dialog box handling functions
void upsStatus::Show(BOOL show)
{
   if (show) {
      if (!m_dlgvisible) {
         DialogBoxParam(m_appinst,
                        MAKEINTRESOURCE(IDD_STATUS),
                        NULL,
                        (DLGPROC)DialogProc,
                        (LONG)this);
      }
   }
}

BOOL CALLBACK upsStatus::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   // We use the dialog-box's USERDATA to store a _this pointer
   // This is set only once WM_INITDIALOG has been recieved, though!
   upsStatus *_this = (upsStatus *)GetWindowLong(hwnd, GWL_USERDATA);

   switch (uMsg) {
   case WM_INITDIALOG:
      // Retrieve the Dialog box parameter and use it as a pointer
      // to the calling vncProperties object
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsStatus *)lParam;

      // Show the dialog
      SetForegroundWindow(hwnd);
      _this->m_dlgvisible = TRUE;
      _this->FillStatusBox(hwnd, IDC_LIST);
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDCANCEL:
      case IDOK:
         // Close the dialog
         EndDialog(hwnd, TRUE);
         _this->m_dlgvisible = FALSE;
         return TRUE;
      }
      break;

   case WM_DESTROY:
      EndDialog(hwnd, FALSE);
      _this->m_dlgvisible = FALSE;
      return TRUE;
   }

   return 0;
}

void upsStatus::FillStatusBox(HWND hwnd, int id_list)
{
   const char* error = "Status not available.";

   if (!m_statmgr->Update()) {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG)error);
      return;
   }

   char *status = m_statmgr->GetAll();
   if (!status || *status == '\0') {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG)error);
      return;
   }

   // GetAll returns newline-separated strings. We need to send separate
   // LB_ADDSTRING messages for each string.
   char *tmp;
   char *str = status;
   while ((tmp = strchr(str, '\n')))
   {
      *tmp = '\0';
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG)str);
      str = tmp + 1;
   }

   free(status);
}
