/*
 * astring.h
 *
 * Simple string management class. Like STL, but lighter.
 */

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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "astring.h"

int astring::format(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   int len = vsnprintf(NULL, 0, format, args);
   va_end(args);

   if (len <= 0) {
      assign("");
      return 0;
   }

   delete [] _data;
   _len = len;
   _data = new char[_len+1];

   va_start(args, format);
   vsnprintf(_data, _len+1, format, args);
   va_end(args);

   return _len;
}

astring &astring::operator=(const astring &rhs)
{
   *this = rhs._data;
   return *this;
}

astring &astring::operator=(const char *rhs)
{
   if (rhs != _data)
      assign(rhs);

   return *this;
}

astring &astring::operator=(const char rhs)
{
   assign(&rhs, 1);
   return *this;
}

const char &astring::operator[](int index) const
{
   if (index >= 0 && index < _len)
      return _data[index];
   if (index < 0 && index >= -_len)
      return _data[_len-index];

   // Bogus index, bail
   abort();
}

char &astring::operator[](int index)
{
   if (index >= 0 && index < (int)_len)
      return _data[index];
   if (index < 0 && index >= -_len)
      return _data[_len-index];

   // Bogus index, bail
   abort();
}

astring &astring::operator+(const char *rhs)
{
   if (rhs && rhs != _data) {
      _len += strlen(rhs);
      realloc(_len);
      strcat(_data, rhs);
   }

   return *this;
}

astring &astring::operator+(const astring &rhs)
{
   if (&rhs != this)
      *this += rhs._data;

   return *this;
}

astring &astring::operator+(const char rhs)
{
   realloc(_len+1);
   _data[_len] = rhs;
   _data[++_len] = '\0';
   return *this;
}

void astring::rtrim()
{
   while (_len >= 1 && isspace(_data[_len-1]))
      _data[--_len] = '\0';
}

void astring::ltrim()
{
   size_t trim = strspn(_data, " \t\r\n");
   memmove(_data, _data+trim, _len-trim+1);
   _len -= trim;
}

void astring::realloc(unsigned int newsize)
{
   char *data = new char[newsize+1];
   strncpy(data, _data, newsize);
   data[newsize] = '\0';

   delete [] _data;
   _data = data;
}

void astring::assign(const char *str, int len)
{
   if (!str)
      len = 0;

   delete [] _data;
   _len = (len < 0) ? strlen(str) : len;
   _data = new char[_len+1];
   strncpy(_data, str, _len);
   _data[_len] = '\0';
}
