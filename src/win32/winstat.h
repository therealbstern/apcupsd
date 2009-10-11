// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2009) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINSTAT_H
#define WINSTAT_H

#include <windows.h>
#include "amutex.h"

// Forward declarations
class StatMgr;
class Meter;
class ListView;

// Object implementing the Status dialogue for apcupsd
class upsStatus
{
public:
   // Constructor/destructor
   upsStatus(HINSTANCE appinst);
   ~upsStatus();

   // General
   void Show();
   void Update(StatMgr *statmgr);

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
   BOOL DialogProcess(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   // Private data
   HWND m_hwnd;
   HINSTANCE m_appinst;
   RECT m_rect;
   Meter *_bmeter;
   Meter *_lmeter;
   ListView *_grid;
   amutex _mutex;
};

#endif // WINSTAT_H
