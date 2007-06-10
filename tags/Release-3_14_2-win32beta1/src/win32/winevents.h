// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINEVENTS_H
#define WINEVENTS_H

#include <windows.h>

// Forward declarations
class StatMgr;

// Object implementing the Events dialogue box for apcupsd
class upsEvents
{
public:
   // Constructor/destructor
   upsEvents(HINSTANCE appinst, StatMgr *statmgr);
   ~upsEvents();

   // Initialisation
   BOOL Init();

   // General
   void Show(BOOL show);

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(
      HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   void FillEventsBox(HWND hwnd, int id_list);

   // Private data
   BOOL m_dlgvisible;
   HINSTANCE m_appinst;
   StatMgr *m_statmgr;
};

#endif // WINEVENTS_H
