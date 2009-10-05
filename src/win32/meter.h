#ifndef __METER_H
#define __METER_H

#include <windows.h>

class Meter
{
public:
   Meter(HWND hwnd, UINT id, int warn, int critical, int level = 0);
   ~Meter() {}

   void Set(int level);

private:
   HWND _hwnd;
   int _warn;
   int _critical;
   int _level;

   static const COLORREF GREEN = RGB(115, 190, 49);
   static const COLORREF RED = RGB(214, 56, 57);
   static const COLORREF YELLOW = RGB(214, 186, 57);
};

#endif
