/*
 * snmp.h
 *
 * SNMP client interface
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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

#ifndef __SNMP_H
#define __SNMP_H

#include "apc.h"
#include "astring.h"
#include "aarray.h"
#include "alist.h"
#include "asn.h"

namespace Snmp
{
   // **************************************************************************
   // Types
   // **************************************************************************
   struct Variable
   {
//      SnmpType type;
      int i32;
      unsigned int u32;
      astring str;
   };

   // **************************************************************************
   // VarBind
   // **************************************************************************
   class VarBind
   {
   public:
      VarBind(int oid[], Variable *data = NULL);
      VarBind(Asn::Sequence &seq);
      ~VarBind();

      bool Extract(Variable *data);
      Asn::ObjectId &Oid() { return *_oid; }

      Asn::Sequence *GetAsn();

   private:
      Asn::ObjectId *_oid;
      Asn::Object *_data;

      VarBind(const VarBind &rhs);
      VarBind &operator=(const VarBind &rhs);
   };

   // **************************************************************************
   // VarBindList
   // **************************************************************************
   class VarBindList
   {
   public:
      VarBindList() {}
      VarBindList(Asn::Sequence &seq);
      VarBindList(int oid[], Variable *data = NULL);
      ~VarBindList();

      void Append(int oid[], Variable *data = NULL);

      unsigned int Size() const { return _vblist.size(); }
      VarBind *operator[](unsigned int idx) { return _vblist[idx]; }

      Asn::Sequence *GetAsn();

   private:
      aarray<VarBind *> _vblist;

      VarBindList(const VarBindList &rhs);
      VarBindList &operator=(const VarBindList &rhs);
   };

   // **************************************************************************
   // Message
   // **************************************************************************
   class Message
   {
   public:
      virtual ~Message() {}

      Asn::Identifier Type() const { return _type; }
      astring Community() const    { return _community; }

      static Message *Demarshal(unsigned char *&buffer, int &buflen);
      bool Marshal(unsigned char *&buffer, int &buflen);

   protected:
      Message() {}
      Message(Asn::Identifier type, const char *community) : 
         _type(type), _community(community) {}
      static const int SNMP_VERSION_1 = 0;
      virtual Asn::Sequence *GetAsn() = 0;

      Asn::Identifier _type;
      astring _community;
   };

   // **************************************************************************
   // VbListMessage
   // **************************************************************************
   class VbListMessage: public Message
   {
   public:
      VbListMessage(Asn::Identifier type, const char *community, int reqid, 
                    int oid[] = NULL, Variable *data = NULL);
      virtual ~VbListMessage() { delete _vblist; }

      int RequestId()       { return _reqid;     }
      int ErrorStatus()     { return _errstatus; }
      int ErrorIndex()      { return _errindex;  }

      void Append(int oid[], Variable *data = NULL);
      unsigned int Size() const { return _vblist->Size(); }
      VarBind &operator[](unsigned int idx) { return *((*_vblist)[idx]); }

      static VbListMessage *CreateFromSequence(
         Asn::Identifier type, const char *community, Asn::Sequence &seq);

   protected:
      VbListMessage(Asn::Identifier type, const char *community, Asn::Sequence &seq);
      virtual Asn::Sequence *GetAsn();

      int _reqid;
      int _errstatus;
      int _errindex;
      VarBindList *_vblist;
   };

   // **************************************************************************
   // SnmpEngine
   // **************************************************************************
   class SnmpEngine
   {
   public:

      SnmpEngine();
      ~SnmpEngine();

      bool Open(const char *host, unsigned short port, const char *comm);
      void Close();

      struct OidVar
      {
         int *oid;
         Variable data;
      };

      bool Get(int oid[], Variable *data);
      bool Get(alist<OidVar> &oids);

   private:

      bool issue(Message *pdu);
      Message *rspwait(unsigned int msec);

      int _socket;
      int _reqid;
      astring _community;
      struct sockaddr_in _destaddr;
   };
};

#endif