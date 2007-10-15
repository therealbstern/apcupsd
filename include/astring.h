#ifndef __ASTRING_H
#define __ASTRING_H

#include <string.h>


class astring
{
public:

   astring() : _data(NULL) { assign(""); }
   astring(const char *str) : _data(NULL) { assign(str); }
   astring(const astring &str) : _data(NULL) { assign(str._data); }
   ~astring() { delete [] _data; }

   int len() const { return _len; }

   int format(const char *format, ...);

   astring &operator=(const astring &rhs);
   astring &operator=(const char *rhs);
   astring &operator=(const char rhs);

   const char &operator[](int index) const;
   char &operator[](int index);

   astring &operator+(const char *rhs);
   astring &operator+=(const char *rhs) { return *this + rhs; }
   astring &operator+(const astring &rhs);
   astring &operator+=(const astring &rhs) { return *this + rhs; }
   astring &operator+(const char rhs);
   astring &operator+=(const char rhs) { return *this + rhs; }

   bool operator==(const char *rhs) const { return !strcmp(_data, rhs); }
   bool operator==(const astring &rhs) const { return *this == rhs._data; }
   bool operator!=(const char *rhs) const { return !(*this == rhs); }
   bool operator!=(const astring &rhs) const { return !(*this == rhs); }

   operator const char *() { return _data; }
   const char *str() const { return _data; }

   void rtrim();
   void ltrim();
   void trim() { ltrim(); rtrim(); }

   bool empty() const { return _len == 0; }

   int compare(const char *rhs) const { return strcmp(_data, rhs); }

private:

   void realloc(unsigned int newlen);
   void assign(const char *str, int len = -1);

   char *_data;
   int _len;
};

inline astring operator+(const char *lhs, const astring &rhs)
   { return astring(lhs) + rhs; }

#endif
