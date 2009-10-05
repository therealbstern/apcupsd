#include <windows.h>
#include <commctrl.h>
#include "meter.h"

Meter::Meter(HWND hwnd, UINT id, int warn, int critical, int level) :
   _warn(warn),
   _critical(critical),
   _level(level)
{
   _hwnd = GetDlgItem(hwnd, id);
}

void Meter::Set(int level)
{
   if (level == _level)
      return;

   SendMessage(_hwnd, PBM_SETPOS, level, 0);
   _level = level;

   // Figure out bar color. 
   COLORREF color;
   if (_warn > _critical)
   {
      // Low is critical
      if (level > _warn)
         color = GREEN;
      else if (level > _critical)
         color = YELLOW;
      else
         color = RED;
   }
   else
   {
      // High is critical
      if (level >= _critical)
         color = RED;
      else if (level >= _warn)
         color = YELLOW;
      else
         color = GREEN;
   }

   SendMessage(_hwnd, PBM_SETBARCOLOR, 0, color);
}
