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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "balloonmgr.h"
#include "winres.h"

#define MAX_TIMEOUT 10000
#define MIN_TIMEOUT 2000

#define ARRAY_SIZE(x) ( sizeof(x) / sizeof((x)[0]) )

BalloonMgr::BalloonMgr()
   : m_exit(false),
     m_active(false)
{
   m_mutex = CreateMutex(NULL, false, NULL);
   m_event = CreateEvent(NULL, false, false, NULL);
   m_timer = CreateWaitableTimer(NULL, false, NULL);

   DWORD tid;
   m_thread = CreateThread(NULL, 0, &BalloonMgr::Thread, this, 0, &tid);
}

BalloonMgr::~BalloonMgr()
{
   // Request thread exit
   m_exit = true;
   signal();

   // Wait for thread exit and force if necessary
   if (m_thread) {
      if (WaitForSingleObject(m_thread, 5000) == WAIT_TIMEOUT)
         TerminateThread(m_thread, 0);
      CloseHandle(m_thread);
   }

   CloseHandle(m_mutex);
   CloseHandle(m_event);
   CloseHandle(m_timer);
}

void BalloonMgr::PostBalloon(HWND hwnd, const char *title, const char *text)
{
   lock();

   Balloon balloon = {hwnd, title, text};
   m_pending.push_back(balloon);
   signal();

   unlock();
}

void BalloonMgr::lock()
{
   WaitForSingleObject(m_mutex, INFINITE);
}

void BalloonMgr::unlock()
{
   ReleaseMutex(m_mutex);
}

void BalloonMgr::signal()
{
   SetEvent(m_event);
}

void BalloonMgr::post()
{
   if (m_pending.empty())
      return;  // No active balloon!?

   // Post balloon tip
   Balloon &balloon = m_pending.front();
   NOTIFYICONDATA nid;
   nid.hWnd = balloon.hwnd;
   nid.cbSize = sizeof(nid);
   nid.uID = IDI_APCUPSD;
   nid.uFlags = NIF_INFO;
   astrncpy(nid.szInfo, balloon.text.c_str(), sizeof(nid.szInfo));
   astrncpy(nid.szInfoTitle, balloon.title.c_str(), sizeof(nid.szInfoTitle));
   nid.uTimeout = MAX_TIMEOUT;
   nid.dwInfoFlags = NIIF_INFO;
   Shell_NotifyIcon(NIM_MODIFY, &nid);

   // Set a timeout to clear the balloon
   LARGE_INTEGER timeout;
   if (m_pending.size() > 1)  // More balloons pending: use minimum timeout
      timeout.QuadPart = -(MIN_TIMEOUT * 10000);
   else  // No other balloons pending: Use maximum timeout
      timeout.QuadPart = -(MAX_TIMEOUT * 10000);
   SetWaitableTimer(m_timer, &timeout, 0, NULL, NULL, false);

   // Remember the time at which we started the timer
   gettimeofday(&m_time, NULL);
}

void BalloonMgr::clear()
{
   if (m_pending.empty())
      return;  // No active balloon!?

   // Clear active balloon
   Balloon &balloon = m_pending.front();
   NOTIFYICONDATA nid;
   nid.hWnd = balloon.hwnd;
   nid.cbSize = sizeof(nid);
   nid.uID = IDI_APCUPSD;
   nid.uFlags = NIF_INFO;
   nid.uTimeout = 0;
   nid.szInfoTitle[0] = '\0';
   nid.szInfo[0] = '\0';
   nid.dwInfoFlags = 0;
   Shell_NotifyIcon(NIM_MODIFY, &nid);

   // Remove vector entry for active balloon
   m_pending.erase(m_pending.begin());
}

DWORD WINAPI BalloonMgr::Thread(LPVOID param)
{
   BalloonMgr *_this = (BalloonMgr*)param;
   HANDLE handles[] = {_this->m_event, _this->m_timer};
   LARGE_INTEGER timeout;
   struct timeval now;
   DWORD index;
   long diff;

   while (1) {
      // Wait for timeout or new balloon request
      index = WaitForMultipleObjects(
         ARRAY_SIZE(handles), handles, false, INFINITE);

      // Exit if we've been asked to do so
      if (_this->m_exit)
         break;

      switch (index) {
      // New balloon request has arrived
      case WAIT_OBJECT_0 + 0:
         _this->lock();

         if (!_this->m_active) {
            // No balloon active: Post new balloon immediately
            if (!_this->m_pending.empty()) {
               _this->post();
               _this->m_active = true;
            }
         } else {
            // A balloon is active: Shorten timer to minimum
            CancelWaitableTimer(_this->m_timer);
            gettimeofday(&now, NULL);
            diff = TV_DIFF_MS(_this->m_time, now);
            if (diff >= MIN_TIMEOUT) {
               // Min timeout already expired
               timeout.QuadPart = -1;
            } else {
               // Wait enough additional time to meet minimum timeout
               timeout.QuadPart = -((MIN_TIMEOUT - diff) * 10000);
            }
            SetWaitableTimer(_this->m_timer, &timeout, 0, NULL, NULL, false);
         }

         _this->unlock();
         break;

      // Timeout ocurred
      case WAIT_OBJECT_0 + 1:
         _this->lock();

         // Clear active balloon
         _this->clear();

         // Post next balloon if there is one
         if (!_this->m_pending.empty()) {
            _this->post();
            _this->m_active = true;
         } else {
            _this->m_active = false;
         }

         _this->unlock();
         break;

      default:
         // Should never happen...but if it does, sleep a bit to prevent
         // spinning.
         Sleep(1000);
         break;
      }
   }
}
