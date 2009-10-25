/*
 * asn.cpp
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

#include "asn.h"
#include <stdlib.h>
#include <stdio.h>

void debug(const char *foo, int indent)
{
//   while (indent--)
//      printf(" ");
//   printf("%s\n", foo);
}

// *****************************************************************************
// AsnObject
// *****************************************************************************

AsnObject *AsnObject::Demarshal(unsigned char *&buffer, int &buflen)
{
   static int indent = 0;

   // Must have at least one byte to work with
   if (buflen < 1)
      return NULL;

   // Extract type code from stream
   AsnIdentifier type = (AsnIdentifier)*buffer++;
   buflen--;

   // Create proper object based on type code
   AsnObject *obj;

   // Assume all construced types are sequences
   if (type & CONSTRUCTED)
   {
      debug("SEQUENCE", indent);
      indent += 2;
      obj = new AsnSequence(type);
   }
   else if ((type & CLASS) == APPLICATION)
   {
      switch (type & TAG)
      {
      case COUNTER:
      case GAUGE:
      case TIMETICKS:
         debug("COUNTER/GAUGE/TIMETICKS", indent);
         obj = new AsnInteger();
         break;
      case IPADDRESS:
         debug("IPADDRESS", indent);
         obj = new AsnOctetString();
         break;
      default:
         printf("UNKNOWN APPLICATION type=0x%02x\n", type);
         debug("UNKNOWN", indent);
         obj = NULL;
         break;      
      }
   }
   else
   {
      switch (type & TAG)
      {
      case INTEGER:
         debug("INTEGER", indent);
         obj = new AsnInteger();
         break;
      case OCTETSTRING:
         debug("OCTETESTRING", indent);
         obj = new AsnOctetString();
         break;
      case OBJECTID:
         debug("OBJECTID", indent);
         obj = new AsnObjectId();
         break;
      case NULLL:
         debug("NULLL", indent);
         obj = new AsnNull();
         break;
      default:
         printf("UNKNOWN ASN type=0x%02x\n", type);
         debug("UNKNOWN", indent);
         obj = NULL;
         break;
      }
   }

   // Have the object demarshal itself from the stream
   if (obj && !obj->demarshal(buffer, buflen))
   {
      delete obj;
      obj = NULL;
   }

   if (type & CONSTRUCTED)
      indent -= 2;

   return obj;
}

bool AsnObject::marshalLength(unsigned int len, unsigned char *&buffer, int &buflen) const
{
   // Bail if not enough space for data
   if (buflen < 5)
      return false;
   buflen -= 5;

   // We always use long form with 4 bytes for simplicity
   *buffer++ = 0x84;
   *buffer++ = (len >> 24) & 0xff;
   *buffer++ = (len >> 16) & 0xff;
   *buffer++ = (len >>  8) & 0xff;
   *buffer++ = len & 0xff;

   return true;
}

bool AsnObject::marshalType(unsigned char *&buffer, int &buflen) const
{
   // Fail if not enough room for data
   if (buflen < 1)
      return false;
   buflen--;

   // Store type directly in stream
   *buffer++ = _type;
   return true;
}

bool AsnObject::demarshalLength(unsigned char *&buffer, int &buflen, int &vallen)
{
   // Must have at least one byte to work with
   if (buflen < 1)
      return false;
   int datalen = *buffer++;
   buflen--;

   // Values less than 128 are stored directly in the first (only) byte
   if (datalen < 128)
   {
      vallen = datalen;
      return true;
   }

   // For values > 128, first byte-128 indicates number of bytes to follow
   // Bail if not enough data for additional bytes
   datalen -= 128;
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Read data bytes
   vallen = 0;
   while (datalen--)
   {
      vallen <<= 8;
      vallen |= *buffer++;
   }

   return true;
}

// *****************************************************************************
// AsnInteger
// *****************************************************************************

bool AsnInteger::Marshal(unsigned char *&buffer, int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Calculate the number of bytes it will require to store the value
   int datalen = 4;
   unsigned int tmp = _value;
   while (datalen > 1)
   {
      if (( _signed && (tmp & 0xff800000) != 0xff800000) ||
          (!_signed && (tmp & 0xff800000) != 0x00000000))
         break;

       tmp <<= 8;
       datalen--;
   }

   // Special case for unsigned value with MSb set
   if (!_signed && datalen == 4 && (tmp & 0x80000000))
      datalen = 5;

   // Marshal the length
   if (!marshalLength(datalen, buffer, buflen))
      return false;

   // Fail if not enough room for data
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Marshal the value itself
   switch (datalen)
   {
   case 5:
      *buffer++ = 0;
   case 4:
      *buffer++ = (_value >> 24) & 0xff;
   case 3:
      *buffer++ = (_value >> 16) & 0xff;
   case 2:
      *buffer++ = (_value >>  8) & 0xff;
   case 1:
      *buffer++ = _value & 0xff;
   }

   return true;
}

bool AsnInteger::demarshal(unsigned char *&buffer, int &buflen)
{
   // Unmarshal the data length
   int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || datalen > 4 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Determine signedness
   if (*buffer & 0x80)
   {
      // Start with all 1s so result will be sign-extended
      _value = -1;
      _signed = true;
   }
   else
   {
      _value = 0;
      _signed = false;
   }

   // Unmarshal the data
   while (datalen--)
   {
      _value <<= 8;
      _value |= *buffer++;
   }

   return true;
}

// *****************************************************************************
// AsnOctetString
// *****************************************************************************

AsnOctetString::AsnOctetString(const char *value) :
   AsnObject(OCTETSTRING),
   _data(NULL)
{
   assign((const unsigned char *)value, strlen(value));
}

AsnOctetString::AsnOctetString(const unsigned char *value, unsigned int len) :
   AsnObject(OCTETSTRING),
   _data(NULL)
{
   assign(value, len);
}

AsnOctetString::AsnOctetString(const AsnOctetString &rhs) :
   AsnObject(OCTETSTRING),
   _data(NULL)
{
   assign(rhs._data, rhs._len);
}

AsnOctetString &AsnOctetString::operator=(const AsnOctetString &rhs)
{
   if (&rhs != this)
      assign(rhs._data, rhs._len);
   return *this;
}

bool AsnOctetString::Marshal(unsigned char *&buffer, int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Marshal the length
   if (!marshalLength(_len, buffer, buflen))
      return false;

   // Bail if not enough room for data
   if ((unsigned int)buflen < _len)
      return false;
   buflen -= _len;

   // Marshal data
   memcpy(buffer, _data, _len);
   buffer += _len;

   return true;
}

bool AsnOctetString::demarshal(unsigned char *&buffer, int &buflen)
{
   // Unmarshal the data length
   int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Unmarshal the data
   assign(buffer, datalen);
   buffer += datalen;

   return true;
}

void AsnOctetString::assign(const unsigned char *data, unsigned int len)
{
   delete [] _data;
   _data = new unsigned char[len+1];
   memcpy(_data, data, len);
   _data[len] = '\0';
   _len = len;
}

// *****************************************************************************
// AsnObjectId
// *****************************************************************************

AsnObjectId::AsnObjectId(const int oid[]) :
   AsnObject(OBJECTID),
   _value(NULL)
{
   assign(oid);
}

AsnObjectId::AsnObjectId(const AsnObjectId &rhs) :
   AsnObject(OBJECTID),
   _value(NULL)
{
   assign(rhs._value, rhs._count);
}

AsnObjectId &AsnObjectId::operator=(const AsnObjectId &rhs)
{
   if (&rhs != this)
      assign(rhs._value, _count);
   return *this;
}

AsnObjectId &AsnObjectId::operator=(const int oid[])
{
   if (oid != _value)
      assign(oid);
   return *this;
}

void AsnObjectId::assign(const int oid[])
{
   unsigned int count = 0;
   while (oid[count] != -1)
      count++;
   assign(oid, count);
}

void AsnObjectId::assign(const int oid[], unsigned int count)
{
   delete [] _value;
   _value = NULL;
   _count = count;
   if (_count)
   {
      _value = new int[_count];
      memcpy(_value, oid, _count*sizeof(int));
   }
}

bool AsnObjectId::operator==(const AsnObjectId &rhs) const
{
   if (_count != rhs._count)
      return false;

   for (unsigned int i = 0; i < _count; i++)
   {
      if (_value[i] != rhs._value[i])
         return false;
   }

   return true;
}

bool AsnObjectId::operator==(const int oid[]) const
{
   unsigned int i;
   for (i = 0; i < _count && oid[i] != -1; i++)
   {
      if (_value[i] != oid[i])
         return false;
   }

   return i == _count && oid[i] == -1;
}

int AsnObjectId::numbits(unsigned int num) const
{
   if (num == 0)
      return 1;

   int log = 0;
   while (num)
   {
      log++;
      num >>= 1;
   }
   return log;
}

bool AsnObjectId::Marshal(unsigned char *&buffer, int &buflen) const
{
   // Protect from trying to marshal an empty object
   if (!_value || _count < 2)
      return false;

   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // ASN.1 requires a special case for first two identifiers
   int cnt = _count-1;
   unsigned int vals[cnt];
   vals[0] = _value[0] * 40 + _value[1];
   for (unsigned int i = 2; i < _count; i++)
      vals[i-1] = _value[i];

   // Calculate number of octets required to store data
   // We can only store 7 bits of data in each octet, so round accordingly
   int datalen = 0;
   for (int i = 0; i < cnt; i++)
      datalen += (numbits(vals[i]) + 6) / 7;

   // Marshal the length
   if (!marshalLength(datalen, buffer, buflen))
      return false;

   // Bail if data bytes will not fit
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Write data: 7 data bits per octet, bit 7 set on all but final octet
   for (int i = 0; i < cnt; i++)
   {
      unsigned int val = vals[i];
      int noctets = (numbits(val) + 6) / 7;
      switch (noctets)
      {
      case 5:
         *buffer++ = ((val >> 28) & 0x7f) | 0x80;
      case 4:
         *buffer++ = ((val >> 21) & 0x7f) | 0x80;
      case 3:
         *buffer++ = ((val >> 14) & 0x7f) | 0x80;
      case 2:
         *buffer++ = ((val >>  7) & 0x7f) | 0x80;
      case 1:
      case 0:
         *buffer++ = val & 0x7f;
      }
   }

   return true;
}

bool AsnObjectId::demarshal(unsigned char *&buffer, int &buflen)
{
   // Unmarshal the data length
   int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Allocate new value array, sized for worst case. +1 is because of
   // ASN.1 special case of compressing first two ids into one octet.
   delete [] _value;
   _value = new int[datalen+1];

   // Unmarshal identifier values
   int i = 0;
   while (datalen)
   {
      // Accumulate octets into this identifier
      _value[i] = 0;
      unsigned int val;
      do
      {
         datalen--;
         val = *buffer++;
         _value[i] <<= 7;
         _value[i] |= val & 0x7f;
      }
      while (datalen && val & 0x80);

      // Handle special case for first two ids
      if (i++ == 0)
      {
         _value[1] = _value[0] % 40;
         _value[0] /= 40;
         i++;
      }
   }
   _count = i;

   return true;
}

// *****************************************************************************
// AsnNull
// *****************************************************************************

bool AsnNull::Marshal(unsigned char *&buffer, int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Marshal the length
   if (!marshalLength(0, buffer, buflen))
      return false;

   return true;
}

bool AsnNull::demarshal(unsigned char *&buffer, int &buflen)
{
   // Unmarshal the data length
   int datalen;
   return demarshalLength(buffer, buflen, datalen) && datalen == 0;
}

// *****************************************************************************
// AsnSequence
// *****************************************************************************

AsnSequence::AsnSequence(AsnIdentifier type) : 
   AsnObject(type), 
   _data(NULL), 
   _size(0)
{
}

AsnSequence::~AsnSequence()
{
   clear();
}

AsnSequence::AsnSequence(const AsnSequence &rhs) :
   AsnObject(rhs._type),
   _data(NULL)
{
   assign(rhs);
}

void AsnSequence::clear()
{
   for (unsigned int i = 0; i < _size; i++)
      delete _data[i];
   delete [] _data;
   _size = 0;
}

bool AsnSequence::Marshal(unsigned char *&buffer, int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Don't know the length yet, but ensure there's room by inserting 0
   // We remember stream position so we can come back later
   unsigned char *lenloc = buffer;
   if (!marshalLength(0, buffer, buflen))
      return false;

   // Marshal all items in the sequence
   unsigned char *dataloc = buffer;
   for (unsigned int i = 0; i < _size; ++i)
   {
      if (!_data[i]->Marshal(buffer, buflen))
         return false;
   }

   // Now go back and fill in proper length
   int junk = 1000; // we already know there's enough space
   marshalLength(buffer-dataloc, lenloc, junk);

   return true;
}

bool AsnSequence::demarshal(unsigned char *&buffer, int &buflen)
{
   // Free any existing data
   clear();

   // Unmarshal the sequence data length
   int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }

   // Unmarshal items from the stream until sequence data is exhausted
   unsigned char *start = buffer;
   while (datalen)
   {
      AsnObject *obj = AsnObject::Demarshal(buffer, datalen);
      if (!obj)
         return false;
      Append(obj);
   }

   buflen -= buffer-start;
   return true;
}

void AsnSequence::Append(AsnObject *obj)
{
   // realloc ... not efficient, but easy
   AsnObject **tmp = new AsnObject *[_size+1];
   memcpy(tmp, _data, _size * sizeof(_data[0]));
   tmp[_size++] = obj;
   delete [] _data;
   _data = tmp;
}

AsnObject *AsnSequence::operator[](unsigned int idx)
{
   if (idx >= _size)
      return NULL;
   return _data[idx];
}

AsnSequence &AsnSequence::operator=(const AsnSequence &rhs)
{
   if (this != &rhs)
      assign(rhs);
   return *this;
}

void AsnSequence::assign(const AsnSequence &rhs)
{
   clear();

   _type = rhs._type;
   _size = rhs._size;
   _data = new AsnObject *[_size];

   for (unsigned int i = 0; i < _size; i++)
      _data[i] = rhs._data[i]->copy();
}
