/*
 * asn.h
 *
 * ASN.1 type classes
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

#ifndef __ASN_H
#define __ASN_H

#include "astring.h"
#include "alist.h"

class AsnInteger;
class AsnObjectId;
class AsnOctetString;
class AsnSequence;

// *****************************************************************************
// AsnObject
// *****************************************************************************
class AsnObject
{
public:

   typedef unsigned char AsnIdentifier;

   // Class field
   static const unsigned char CLASS            = 0xC0;
   static const unsigned char UNIVERSAL        = 0x00;
   static const unsigned char APPLICATION      = 0x40;
   static const unsigned char CONTEXT_SPECIFIC = 0x80;
   static const unsigned char PRIVATE          = 0xC0;

   // Primitive/Constructed field
   static const unsigned char CONSTRUCTED      = 0x20;
   static const unsigned char PRIMITIVE        = 0x00;

   // Tag field
   static const unsigned char TAG              = 0x1f;
   static const unsigned char INTEGER          = 0x02;   // ASN
   static const unsigned char BITSTRING        = 0x03;   // ASN
   static const unsigned char OCTETSTRING      = 0x04;   // ASN
   static const unsigned char NULLL            = 0x05;   // ASN
   static const unsigned char OBJECTID         = 0x06;   // ASN
   static const unsigned char SEQUENCE         = 0x10;   // ASN
   static const unsigned char SET              = 0x11;   // ASN
   static const unsigned char IPADDRESS        = 0x00;   // SNMP (application)
   static const unsigned char COUNTER          = 0x01;   // SNMP (application)
   static const unsigned char GAUGE            = 0x02;   // SNMP (application)
   static const unsigned char TIMETICKS        = 0x03;   // SNMP (application)

   AsnObject(AsnIdentifier type): _type(type) {}
   virtual ~AsnObject() {}

   AsnIdentifier Type() const { return _type; }

   static AsnObject *Demarshal(unsigned char *&buffer, int &buflen);
   virtual bool Marshal(unsigned char *&buffer, int &buflen) const = 0;

   virtual AsnObject *copy() const = 0;

   AsnInteger *AsInteger()         { return (AsnInteger*)this;     }
   AsnObjectId *AsObjectId()       { return (AsnObjectId*)this;    }
   AsnOctetString *AsOctetString() { return (AsnOctetString*)this; }
   AsnSequence *AsSequence()       { return (AsnSequence*)this;    }

   bool IsPrimitive()   { return (_type & CONSTRUCTED) == PRIMITIVE; }
   bool IsInteger()     { return IsPrimitive() && (_type & TAG) == INTEGER; }
   bool IsObjectId()    { return IsPrimitive() && (_type & TAG) == OBJECTID; }
   bool IsOctetString() { return IsPrimitive() && (_type & TAG) == OCTETSTRING; }
   bool IsSequence()    { return !IsPrimitive(); }

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen) = 0;
   bool marshalType(unsigned char *&buffer, int &buflen) const;
   bool marshalLength(unsigned int len, unsigned char *&buffer, int &buflen) const;
   bool demarshalLength(unsigned char *&buffer, int &buflen, int &vallen);

   AsnIdentifier _type;
};

// *****************************************************************************
// AsnInteger
// *****************************************************************************
class AsnInteger: public AsnObject
{
public:

   AsnInteger() : 
      AsnObject(INTEGER), _value(0), _signed(false)         {}
   AsnInteger(unsigned int value) :
      AsnObject(INTEGER), _value(value), _signed(false)     {}
   AsnInteger(int value) :
      AsnObject(INTEGER), _value(value), _signed(value < 0) {}
   virtual ~AsnInteger() {}

   unsigned int UintValue() const { return _value; }
   int IntValue() const { return (int)_value; }

   AsnInteger &operator=(int value)
      { _value = value; _signed = value < 0; return *this; }
   AsnInteger &operator=(unsigned int value)
      { _value = value; _signed = false; return *this; }

   virtual AsnObject *copy() const { return new AsnInteger(*this); }

   virtual bool Marshal(unsigned char *&buffer, int &buflen) const;

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen);

   unsigned int _value;
   bool _signed;
};

// *****************************************************************************
// AsnOctetString
// *****************************************************************************
class AsnOctetString: public AsnObject
{
public:

   AsnOctetString() : AsnObject(OCTETSTRING), _data(NULL), _len(0) {}
   AsnOctetString(const char *value);
   AsnOctetString(const unsigned char *value, unsigned int len);
   virtual ~AsnOctetString() { delete [] _data; }

   AsnOctetString(const AsnOctetString &rhs);
   AsnOctetString &operator=(const AsnOctetString &rhs);
   virtual AsnObject *copy() const { return new AsnOctetString(*this); }

   const unsigned char *Value() const { return _data; }
   const unsigned int Length() const  { return _len; }

   operator const char *() const { return (const char *)_data; }

   virtual bool Marshal(unsigned char *&buffer, int &buflen) const;

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen);

   void assign(const unsigned char *data, unsigned int len);

   unsigned char *_data;
   unsigned int _len;
};

// *****************************************************************************
// AsnObjectId
// *****************************************************************************
class AsnObjectId: public AsnObject
{
public:

   AsnObjectId() : AsnObject(OBJECTID), _value(NULL), _count(0) {}
   AsnObjectId(const int oid[]);
   virtual ~AsnObjectId() { delete [] _value; }

   AsnObjectId(const AsnObjectId &rhs);
   AsnObjectId &operator=(const AsnObjectId &rhs);
   AsnObjectId &operator=(const int oid[]);
   virtual AsnObject *copy() const { return new AsnObjectId(*this); }

   bool operator==(const AsnObjectId &rhs) const;
   bool operator==(const int oid[]) const;
   bool operator!=(const AsnObjectId &rhs) const { return !(*this == rhs); }
   bool operator!=(const int oid[]) const        { return !(*this == oid); }

   virtual bool Marshal(unsigned char *&buffer, int &buflen) const;

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen);
   int numbits(unsigned int num) const;
   void assign(const int oid[]);
   void assign(const int oid[], unsigned int count);

   int *_value;
   unsigned int _count;
};

// *****************************************************************************
// AsnNull
// *****************************************************************************
class AsnNull: public AsnObject
{
public:

   AsnNull() : AsnObject(NULLL) {}
   virtual ~AsnNull() {}

   virtual AsnObject *copy() const { return new AsnNull(*this); }

   virtual bool Marshal(unsigned char *&buffer, int &buflen) const;

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen);
};

// *****************************************************************************
// AsnSequence
// *****************************************************************************
class AsnSequence: public AsnObject
{
public:

   AsnSequence(AsnIdentifier type = CONSTRUCTED|SEQUENCE);
   virtual ~AsnSequence();

   AsnSequence(const AsnSequence &rhs);
   AsnSequence &operator=(const AsnSequence &rhs);
   virtual AsnObject *copy() const { return new AsnSequence(*this); }

   unsigned int Size() { return _size; }
   AsnObject *operator[](unsigned int idx);
   void Append(AsnObject *obj);

   virtual bool Marshal(unsigned char *&buffer, int &buflen) const;

protected:

   virtual bool demarshal(unsigned char *&buffer, int &buflen);
   void assign(const AsnSequence &rhs);
   void clear();

   AsnObject **_data;
   unsigned int _size;
};

#endif
