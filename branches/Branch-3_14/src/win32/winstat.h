// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINSTAT_H
#define WINSTAT_H

#include <windows.h>

// Forward declarations
class StatMgr;

// Object implementing the Status dialogue for apcupsd
class upsStatus
{
public:
   // Constructor/destructor
   upsStatus(HINSTANCE appinst, StatMgr *statmgr);
   ~upsStatus();

   // Initialisation
   BOOL Init();

   // General
   void Show(BOOL show);
   void FillStatusBox();

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   // Private data
   HWND m_hwnd;
   HINSTANCE m_appinst;
   StatMgr *m_statmgr;
   RECT m_rect;
};

#endif // WINSTAT_H
