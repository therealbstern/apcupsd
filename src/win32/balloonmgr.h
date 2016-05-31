/*
 * Copyright (C) 2007 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#ifndef BALLOONMGR_H
#define BALLOONMGR_H

#include <windows.h>
#include "astring.h"
#include "alist.h"

class BalloonMgr
{
public:

   BalloonMgr();
   ~BalloonMgr();

   void PostBalloon(HWND hwnd, const char *title, const char *text);
   static DWORD WINAPI Thread(LPVOID param);

private:

   void post();
   void clear();
   void lock();
   void unlock();
   void signal();

   struct Balloon {
      HWND hwnd;
      astring title;
      astring text;
   };

   alist<Balloon>       _pending;
   HANDLE               _mutex;
   bool                 _exit;
   bool                 _active;
   HANDLE               _event;
   HANDLE               _timer;
   struct timeval       _time;
   HANDLE               _thread;
};

#endif // BALLOONMGR_H
