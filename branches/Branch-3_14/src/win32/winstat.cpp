// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2009) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the Status dialog

#include <windows.h>
#include <commctrl.h>
#include "winstat.h"
#include "resource.h"
#include "statmgr.h"
#include "meter.h"
#include "listview.h"

// Constructor/destructor
upsStatus::upsStatus(HINSTANCE appinst, StatMgr *statmgr) :
   m_hwnd(NULL),
   m_appinst(appinst),
   m_statmgr(statmgr)
{
}

upsStatus::~upsStatus()
{
}

// Dialog box handling functions
void upsStatus::Show()
{
   if (!m_hwnd)
   {
      DialogBoxParam(m_appinst,
                     MAKEINTRESOURCE(IDD_STATUS),
                     NULL,
                     (DLGPROC)DialogProc,
                     (LONG)this);
   }
}

BOOL CALLBACK upsStatus::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   upsStatus *_this;

   // Retrieve virtual 'this' pointer. When we come in here the first time for
   // the WM_INITDIALOG message, the pointer is in lParam. We then store it in
   // the user data so it can be retrieved on future calls.
   if (uMsg == WM_INITDIALOG)
   {
      // Set dialog user data to our "this" pointer which comes in via lParam.
      // On subsequent calls, this will be retrieved by the code below.
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsStatus *)lParam;
   }
   else
   {
      // We've previously been initialized, so retrieve pointer from user data
      _this = (upsStatus *)GetWindowLong(hwnd, GWL_USERDATA);
   }

   // Call thru to non-static member function
   return _this->DialogProcess(hwnd, uMsg, wParam, lParam);
}

BOOL upsStatus::DialogProcess(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   switch (uMsg) {
   case WM_INITDIALOG:
      // Silly: Save initial window size for use as minimum size. There's 
      // probably some programmatic way to fetch this from the resource when
      // we need it, but I can't find it. So we'll save it at runtime.
      GetWindowRect(hwnd, &m_rect);

      // Initialize control wrappers
      _bmeter = new Meter(hwnd, IDC_BATTERY, 25, 15);
      _lmeter = new Meter(hwnd, IDC_LOAD, 75, 90);
      _grid = new ListView(hwnd, IDC_STATUSGRID, 2);

      // Save a copy of our window handle for later use.
      // Important to do this AFTER everything needed by FillStatusBox() is
      // initialized and ready to go since that function may be called at any
      // time from the wintray timer thread.
      m_hwnd = hwnd;

      // Show the dialog
      FillStatusBox();
      SetForegroundWindow(hwnd);
      return TRUE;

   case WM_GETMINMAXINFO:
      // Restrict minimum size to initial window size
      MINMAXINFO *mmi = (MINMAXINFO*)lParam;
      mmi->ptMinTrackSize.x = m_rect.right - m_rect.left;
      mmi->ptMinTrackSize.y = m_rect.bottom - m_rect.top;
      return TRUE;

   case WM_SIZE:
   {
      // Fetch new window size (esp client area size)
      WINDOWINFO wininfo;
      wininfo.cbSize = sizeof(wininfo);
      GetWindowInfo(hwnd, &wininfo);

      // Fetch current listview position
      HWND ctrl = GetDlgItem(hwnd, IDC_STATUSGRID);
      RECT gridrect;
      GetWindowRect(ctrl, &gridrect);

      // Calculate new position and size of listview
      int left = gridrect.left - wininfo.rcClient.left;
      int top = gridrect.top - wininfo.rcClient.top;
      int width = wininfo.rcClient.right - wininfo.rcClient.left - 2*left;
      int height = wininfo.rcClient.bottom - wininfo.rcClient.top - top - left;

      // Resize listview
      SetWindowPos(
         ctrl, NULL, left, top, width, height, SWP_NOZORDER | SWP_SHOWWINDOW);

      return TRUE;
   }

   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case IDCANCEL:
      case IDOK:
         EndDialog(hwnd, TRUE);
         return TRUE;
      }
      break;

   case WM_DESTROY:
      _mutex.lock();
      m_hwnd = NULL;
      delete _bmeter;
      delete _lmeter;
      delete _grid;
      _mutex.unlock();
      return TRUE;
   }

   return FALSE;
}

void upsStatus::FillStatusBox()
{
   // Bail if window is not open
   _mutex.lock();
   if (!m_hwnd)
   {
      _mutex.unlock();
      return;
   }

   // Fetch full status from apcupsd
   alist<astring> status;
   if (!m_statmgr->GetAll(status) || status.empty())
   {
      _mutex.unlock();
      return;
   }

   // The simple way to update the listview would be to remove all items and
   // then add them again. However, that causes the control to flicker and the
   // scrollbar to reset to the top every time, which makes it pretty much
   // unusable. To prevent that, we update the items in-place, adding new ones
   // and removing unused ones as necessary. That way the scroll position stays
   // put and only the items that change are redrawn.

   // Get current item count and prepare to update the listview
   int num = _grid->NumItems();
   int count = 0;

   // Add each status line to the listview
   alist<astring>::const_iterator iter;
   for (iter = status.begin(); iter != status.end(); ++iter)
   {
      // Split "key: value" string into separate strings
      // (Eventually statmgr should do this for us)
      int idx = (*iter).strchr(':');
      astring key = (*iter).substr(0, idx).trim();
      astring value = (*iter).substr(idx+1).trim();

      // Set main item (left column). This will be an insert if there is no
      // existing item at this position or an update if an item already exists.
      if (count >= num)
         _grid->AppendItem(key);
      else
         _grid->UpdateItem(count, 0, key);

      // Set subitem (right column). This is always an update since the item
      // itself is guaranteed to exist by the code above.
      _grid->UpdateItem(count, 1, value);

      // On to the next item
      count++;
   }

   // Remove any leftover items that are no longer needed. This is needed for
   // when apcupsd suddenly emits fewer status items, such as when COMMLOST.
   while (count < num)
   {
      _grid->DeleteItem(count);
      num--;
   }

   // Autosize listview columns
   _grid->Autosize();

   // Update battery
   _bmeter->Set(atoi(m_statmgr->Get("BCHARGE")));

   // Update load
   _lmeter->Set(atoi(m_statmgr->Get("LOADPCT")));

   // Update status
   char str[128];
   astring stat = m_statmgr->Get("STATUS");
   SendDlgItemMessage(m_hwnd, IDC_STATUS, WM_GETTEXT, sizeof(str), (LONG)str);
   if (stat != str)
      SendDlgItemMessage(m_hwnd, IDC_STATUS, WM_SETTEXT, 0, (LONG)stat.str());

   // Update runtime
   astring runtime = m_statmgr->Get("TIMELEFT");
   runtime = runtime.substr(0, runtime.strchr(' '));
   SendDlgItemMessage(m_hwnd, IDC_RUNTIME, WM_GETTEXT, sizeof(str), (LONG)str);
   if (runtime != str)
      SendDlgItemMessage(m_hwnd, IDC_RUNTIME, WM_SETTEXT, 0, (LONG)runtime.str());

   // Update title bar
   astring name;
   name.format("Status for UPS: %s", m_statmgr->Get("UPSNAME").str());
   SendMessage(m_hwnd, WM_SETTEXT, 0, (LONG)name.str());

   _mutex.unlock();
}
