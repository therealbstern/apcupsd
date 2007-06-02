// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the About dialog

#include <windows.h>
#include "winabout.h"
#include "winres.h"

// Constructor/destructor
upsAbout::upsAbout(HINSTANCE appinst)
{
   m_dlgvisible = FALSE;
   m_appinst = appinst;
}

upsAbout::~upsAbout()
{
}

// Initialisation
BOOL upsAbout::Init()
{
   return TRUE;
}

// Dialog box handling functions
void upsAbout::Show(BOOL show)
{
   if (show) {
      if (!m_dlgvisible) {
         DialogBoxParam(m_appinst,
                        MAKEINTRESOURCE(IDD_ABOUT), 
                        NULL,
                       (DLGPROC) DialogProc,
                       (LONG) this);
      }
   }
}

BOOL CALLBACK upsAbout::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   // We use the dialog-box's USERDATA to store a _this pointer
   // This is set only once WM_INITDIALOG has been recieved, though!
   upsAbout *_this = (upsAbout *)GetWindowLong(hwnd, GWL_USERDATA);

   switch (uMsg)
   {
   case WM_INITDIALOG:
      // Retrieve the Dialog box parameter and use it as a pointer
      // to the calling vncProperties object
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsAbout *)lParam;

      // Show the dialog
      SetForegroundWindow(hwnd);
      _this->m_dlgvisible = TRUE;
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
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
