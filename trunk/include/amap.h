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
