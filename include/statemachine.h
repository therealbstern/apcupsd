/*
 * statemachine.h
 *
 * UPS state machine
 */

/*
 * Copyright (C) 2007 Adam Kropelin <akropel1@rochester.rr.com>
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

#ifndef _STATEMACHINE_H
#define _STATEMACHINE_H

#include "UpsValue.h"
#include "atimer.h"
#include "athread.h"

class UPSINFO;

class UpsStateMachine: public athread
{
public:

   UpsStateMachine(UPSINFO *ups);
   ~UpsStateMachine();
   
   void Start();

private:

   bool HandleEventStateAny(UpsDatum &event);
   void HandleEventStateIdle(UpsDatum &event);
   void HandleEventStatePowerfail(UpsDatum &event);
   void HandleEventStateOnbatt(UpsDatum &event);
   void HandleEventStateShutdownDebounce(UpsDatum &event) {}
   void HandleEventStateShutdown(UpsDatum &event)         {}
   void HandleEventStateSelftest(UpsDatum &event);

   void EnterStateIdle()             {}
   void EnterStatePowerfail();
   void EnterStateOnbatt();
   void EnterStateShutdownDebounce() {}
   void EnterStateShutdown();
   void EnterStateSelftest();

   virtual void body();
   static void TimerTimeout(void *arg);

   enum State
   {
      STATE_IDLE,
      STATE_POWERFAIL,
      STATE_ONBATT,
      STATE_SHUTDOWN_DEBOUNCE,
      STATE_SHUTDOWN,
      STATE_SELFTEST
   };

   void ChangeState(State newstate);
   static const char *StateToText(State state);
   void TransitionShutdown(int cmd);

   UPSINFO *_ups;
   State _state;
   atimer _timer;
   int _sdowncmd;
};

#endif   /* _STATEMACHINE_H */
