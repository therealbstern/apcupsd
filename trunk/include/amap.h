/*
 * amap.h
 *
 * Simple map template class build on alist.
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

#ifndef __AMAP_H
#define __AMAP_H
 
#include "alist.h"

template <class K, class V>
class amap
{
private:

   struct keyval
   {
      keyval() {}
      keyval(const K& key) : _key(key) {}
      bool operator==(const keyval &rhs) { return _key == rhs._key; }
      K _key;
      V _val;
   };

public:

   V& operator[](const K& key)
   {
      typename alist<keyval>::iterator iter = _map.find(key);
      if (iter != _map.end())
         return (*iter)._val;
      _map.append(keyval(key));
      return _map.last()._val;
   }

   bool contains(const K& key) { return _map.find(key) != _map.end();  }
   bool empty() { return _map.empty(); }

   class iterator
   {
   public:
      iterator() {}
      iterator(const iterator &rhs) : _iter(rhs._iter) {}

      iterator &operator++() { ++_iter; return *this; }
      iterator operator++(int) { iterator tmp(_iter); ++(*this); return tmp; }
      iterator &operator--() { --_iter; return *this; }
      iterator operator--(int) { iterator tmp(_iter); --(*this); return tmp; }

      V& value() { return (*_iter)._val; }
      K& key() { return (*_iter)._key; }
      V& operator*() { return value(); }

      bool operator==(const iterator &rhs) const { return _iter == rhs._iter; }
      bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

      iterator &operator=(const iterator &rhs)
         { if (&rhs != this) _iter = rhs._iter; return *this;}

   private:
      friend class amap;
      iterator(typename alist<keyval>::iterator iter) : _iter(iter) {}
      typename alist<keyval>::iterator _iter;
   };

   iterator end() { return iterator(_map.end()); }
   iterator begin() { return iterator(_map.begin()); }
   iterator find(const K& key) { return iterator(_map.find(key)); }

private:

   // Should really use a hash table for efficient random lookups,
   // but for now we'll take the easy route and use alist.
   alist<keyval> _map;
};

#endif // __AMAP_H
