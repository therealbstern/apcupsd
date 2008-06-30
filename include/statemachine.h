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

#include "apc.h"
#include "UpsValue.h"
#include "atimer.h"
#include "athread.h"


class UpsStateMachine: public athread
{
public:

   UpsStateMachine(UPSINFO *ups);
   ~UpsStateMachine();

   void Start();

private:

  enum State
   {
      STATE_IDLE,
      STATE_POWERFAIL,
      STATE_ONBATT,
      STATE_SELFTEST,
      STATE_SHUTDOWN_LOADLIMIT,
      STATE_SHUTDOWN_RUNLIMIT,
      STATE_SHUTDOWN_BATTLOW,
      NUM_STATES // MUST BE LAST
   };

   class BaseState
   {
   public:
      virtual void OnEvent(UpsDatum &event) = 0;
      virtual void OnEnter() = 0;
      virtual void OnExit() = 0;

   protected:
      BaseState(UpsStateMachine &parent) : _parent(parent) {}
      virtual ~BaseState() {}
      void ChangeState(State newstate) { _parent.ChangeState(newstate); }
      UpsStateMachine &_parent;
   };

   class StateIdle: public BaseState
   {
   public:
      StateIdle(UpsStateMachine &parent) : BaseState(parent) {}
      virtual ~StateIdle() {}
      virtual void OnEvent(UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
   };

   class StatePowerfail: public BaseState, public atimer::client
   {
   public:
      StatePowerfail(UpsStateMachine &parent)
         : BaseState(parent), _timer(*this) {}
      virtual ~StatePowerfail() {}
      virtual void OnEvent(UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void HandleTimeout(int id);
   private:
      atimer _timer;
   };

   class StateOnbatt: public BaseState, public atimer::client
   {
   public:
      StateOnbatt(UpsStateMachine &parent)
         : BaseState(parent), _timer(*this) {}
      virtual ~StateOnbatt() {}
      virtual void OnEvent(UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void HandleTimeout(int id);
   private:
      atimer _timer;
   };

   class StateSelftest: public BaseState, public atimer::client
   {
   public:
      StateSelftest(UpsStateMachine &parent)
         : BaseState(parent), _timer(*this) {}
      virtual ~StateSelftest() {}
      virtual void OnEvent(UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void HandleTimeout(int id);
   private:
      atimer _timer;
   };

   class StateShutdown: public BaseState
   {
   public:
      StateShutdown(UpsStateMachine &parent, int cmd)
         : BaseState(parent), _sdowncmd(cmd) {}
      virtual ~StateShutdown() {}
      virtual void OnEvent(UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
   private:
      int _sdowncmd;
   };

   // All state classes are friends
   friend class BaseState;

   virtual void body();

   bool HandleEventStateAny(UpsDatum &event);
   static const char *StateToText(State state);
   void ChangeState(State newstate);

   UPSINFO *_ups;
   BaseState *_states[NUM_STATES];
   State _state;
};

#endif   /* _STATEMACHINE_H */
