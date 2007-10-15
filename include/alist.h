#ifndef __ALIST_H
#define __ALIST_H

#include <stdlib.h>

template <class T> class alist
{
private:

   class node
   {
   public:
      node(const T& elem) : _next(NULL), _prev(NULL), _elem(elem) {}

      ~node() {  if (_prev) _prev->_next = _next;
                 if (_next) _next->_prev = _prev; }

      operator T&() { return _elem; }

      void next(node *link) { link->_next = _next;
                              link->_prev = this;
                              if (_next) _next->_prev = link;
                              _next = link; }
      void prev(node *link) { link->_next = this;
                              link->_prev = _prev;
                              if (_prev) _prev->_next = link;
                              _prev = link; }

      node *next() { return _next; }
      node *prev() { return _prev; }

      bool operator==(const node &rhs) const
         { return _next == rhs._next && _prev == rhs._prev; }

   private:
      node *_next, *_prev;
      T _elem;
   };

public:

   alist() : _head(NULL), _tail(NULL), _size(0) {}

   ~alist()
   {
      while (!empty()) remove(begin());
      _size = 0;
   }

   T& first() { return *_head; }
   T& last() { return *_tail; }

   bool empty() const { return _size <= 0; }
   int size() const { return _size; }

   void append(const T& elem)
   {
      node *nd = new node(elem);
      if (_tail)
         _tail->next(nd);
      else
         _head = nd;
      _tail = nd;
      _size++;
   }

   void prepend(const T& elem)
   {
      node *nd = new node(elem);
      if (_head)
         _head->prev(nd);
      else
         _tail = nd;
      _head = nd;
      _size++;
   }

   void remove_first() { if (!empty()) remove(_head); }
   void remove_last() { if (!empty()) remove(_tail); }

   class iterator
   {
   public:
      iterator() {}
      iterator(const iterator &rhs) : _node(rhs._node) {}

      iterator &operator++() { _node = _node->next(); return *this; }
      iterator operator++(int) { iterator tmp(_node); ++(*this); return tmp; }
      iterator &operator--() { _node = _node->prev(); return *this; }
      iterator operator--(int) { iterator tmp(_node); --(*this); return tmp; }

      T& operator*() { return *_node; }

      bool operator==(const iterator &rhs) const { return _node == rhs._node; }
      bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

      iterator &operator=(const iterator &rhs)
         { if (&rhs != this) _node = rhs._node; return *this; }

   private:
      friend class alist;
      iterator(node *node) : _node(node) {}
      node *_node;
   };

   iterator begin() const { return iterator(_head); }
   iterator end() const { return iterator(NULL); }

   iterator remove(iterator iter) {
      if (iter == _head) _head = iter._node->next();
      if (iter == _tail) _tail = iter._node->prev();
      iterator newiter(iter._node->next());
      delete iter._node;
      _size--;
      return newiter;
   }

   iterator find(const T& needle) {
      iterator iter;
      for (iter = begin(); iter != end(); ++iter)
         if (*iter == needle) break;
      return iter;
   }

private:

   node *_head, *_tail;
   int _size;
};

#endif
