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
#include "aqueue.h"

class UpsStateMachine: public athread, public atimer::client,
                       public UpsInfo::Client
{
public:

   UpsStateMachine(UPSINFO *ups);
   virtual ~UpsStateMachine();

   void Start();

private:

   // **************************************************************************
   // EVENT CLASSES
   // **************************************************************************
   
   enum EventType
   {
      EVENT_UPSDATUM,
      EVENT_TIMEOUT,
      EVENT_QUIT,
   };

   class BaseEvent
   {
   public:
      EventType type() const { return _type; }
   protected:
      BaseEvent(EventType type) : _type(type) {}
   private:
      EventType _type;
   };

   class EventUpsDatum: public BaseEvent
   {
   public:
      EventUpsDatum(const UpsDatum &datum) :
         BaseEvent(EVENT_UPSDATUM), _datum(datum) {}
      const UpsDatum &datum() const { return _datum; }
   private:
      UpsDatum _datum;
   };

   class EventTimeout: public BaseEvent
   {
   public:
      EventTimeout(int id) :
         BaseEvent(EVENT_TIMEOUT), _id(id) {}
      int id() const { return _id; }
   private:
      int _id;
   };

   class EventQuit: public BaseEvent
   {
   public:
      EventQuit() : BaseEvent(EVENT_QUIT) {}
   };

   // **************************************************************************
   // STATE CLASSES
   // **************************************************************************

   // Timer IDs shared amongst the state classes.
   // These should be unique across the state classes so stale
   // timer events can be identified and ignored.
   enum
   {
      TIMER_POWERFAIL,
      TIMER_SELFTEST,
      TIMER_ONBATT_TIMELIMIT,
      TIMER_ONBATT_BATTLOW,
      TIMER_ONBATT_RUNLIMIT,
      TIMER_ONBATT_LOADLIMIT,
   };

   friend class BaseState;
   class BaseState
   {
   public:
      virtual ~BaseState() {}
      virtual void OnEvent(const UpsDatum &event) = 0;
      virtual void OnEnter() = 0;
      virtual void OnExit() = 0;
      virtual void OnTimeout(int id) = 0;
   protected:
      BaseState(UpsStateMachine &parent) : _parent(parent) {}
      void ChangeState(int newstate) { _parent.ChangeState(newstate); }
      UpsStateMachine &_parent;
   };

   class StateIdle: public BaseState
   {
   public:
      StateIdle(UpsStateMachine &parent)
         : BaseState(parent) {}
      virtual ~StateIdle() {}
      virtual void OnEvent(const UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void OnTimeout(int id);
   };

   class StatePowerfail: public BaseState
   {
   public:
      StatePowerfail(UpsStateMachine &parent)
         : BaseState(parent), _timer(parent, TIMER_POWERFAIL) {}
      virtual ~StatePowerfail() {}
      virtual void OnEvent(const UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void OnTimeout(int id);
   private:
      atimer _timer;
   };

   class StateOnbatt: public BaseState
   {
   public:
      StateOnbatt(UpsStateMachine &parent)
         : BaseState(parent),
           _timer_timelimit(parent, TIMER_ONBATT_TIMELIMIT),
           _timer_battlow(parent, TIMER_ONBATT_BATTLOW),
           _timer_loadlimit(parent, TIMER_ONBATT_LOADLIMIT),
           _timer_runlimit(parent, TIMER_ONBATT_RUNLIMIT) {}
      virtual ~StateOnbatt() {}
      virtual void OnEvent(const UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void OnTimeout(int id);
   private:
      void CheckShutdown();
      atimer _timer_timelimit;
      atimer _timer_battlow;
      atimer _timer_loadlimit;
      atimer _timer_runlimit;
   };

   class StateSelftest: public BaseState
   {
   public:
      StateSelftest(UpsStateMachine &parent)
         : BaseState(parent), _timer(parent, TIMER_SELFTEST) {}
      virtual ~StateSelftest() {}
      virtual void OnEvent(const UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void OnTimeout(int id);
   private:
      atimer _timer;
   };

   class StateShutdown: public BaseState
   {
   public:
      StateShutdown(UpsStateMachine &parent, int cmd)
         : BaseState(parent), _sdowncmd(cmd) {}
      virtual ~StateShutdown() {}
      virtual void OnEvent(const UpsDatum &event);
      virtual void OnEnter();
      virtual void OnExit();
      virtual void OnTimeout(int id);
   private:
      int _sdowncmd;
   };

   // States in the state machine
   enum
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

   virtual void body();
   bool HandleEventStateAny(const UpsDatum &event);
   static const char *StateToText(int state);
   void ChangeState(int newstate);
   virtual void HandleTimeout(int id);
   virtual void HandleUpsDatum(const UpsDatum &datum);

   UPSINFO *_ups;
   BaseState *_states[NUM_STATES];
   int _state;
   aqueue<BaseEvent*> _events;
};

#endif   /* _STATEMACHINE_H */
