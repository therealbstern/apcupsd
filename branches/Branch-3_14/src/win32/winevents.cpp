// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the Events dialogue 

#include <windows.h>
#include "winevents.h"
#include "winres.h"

extern void FillEventsBox(HWND hwnd, int id_list);

// Constructor/destructor
upsEvents::upsEvents(HINSTANCE appinst)
{
    m_dlgvisible = FALSE;
    m_appinst = appinst;
}

upsEvents::~upsEvents()
{
}

// Initialisation
BOOL upsEvents::Init()
{
    return TRUE;
}

// Dialog box handling functions
void upsEvents::Show(BOOL show)
{
   if (show) {
      if (!m_dlgvisible) {
         DialogBoxParam(m_appinst,
                        MAKEINTRESOURCE(IDD_EVENTS),
                        NULL,
                        (DLGPROC)DialogProc,
                        (LONG)this);
      }
   }
}

BOOL CALLBACK upsEvents::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
    // We use the dialog-box's USERDATA to store a _this pointer
    // This is set only once WM_INITDIALOG has been recieved, though!
    upsEvents *_this = (upsEvents *)GetWindowLong(hwnd, GWL_USERDATA);

    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowLong(hwnd, GWL_USERDATA, lParam);
        _this = (upsEvents *) lParam;

        // Show the dialog
        SetForegroundWindow(hwnd);
        _this->m_dlgvisible = TRUE;
        FillEventsBox(hwnd, IDC_LIST);
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
