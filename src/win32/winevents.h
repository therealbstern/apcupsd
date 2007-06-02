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

// Object implementing the Events dialogue box for apcupsd
class upsEvents
{
public:
   // Constructor/destructor
   upsEvents(HINSTANCE appinst);
   ~upsEvents();

   // Initialisation
   BOOL Init();

   // General
   void Show(BOOL show);

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(
      HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   // Private data
   BOOL m_dlgvisible;
   HINSTANCE m_appinst;
};

#endif // WINEVENTS_H
