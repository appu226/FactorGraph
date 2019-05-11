/*

Copyright 2019 Parakram Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <memory>


namespace parakram {


  // ***** DisjointSet *****
  // https://en.wikipedia.org/wiki/Disjoint-set_data_structure
  template<typename TElement>
  class DisjointSet 
  {
    public:

      typedef DisjointSet<TElement> * RPtr;
      typedef std::shared_ptr<DisjointSet<TElement> > Ptr;


      // ***** Constructor *****
      // Takes an identifier, and an element
      // and creates a singleton set
      DisjointSet(int id, const TElement & elem):
        m_id(id),
        m_elem(elem),
        m_size(1),
        m_parent(this)
      { }


      // ***** find *****
      // https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Find
      RPtr find()
      {
        if (m_parent != this)
          m_parent = m_parent->find();
        return m_parent;
      }

      // ***** element *****
      // get the underlying element
      TElement element() const { return m_elem; }

      // ***** id *****
      // get the id of this set
      int id() const { return m_id; }

      // ***** size *****
      // return the size of the partition
      int size() const { return m_size; }

      // ***** computeUnion *****
      // https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Union
      RPtr computeUnion(DisjointSet<TElement> & that)
      {
        RPtr thisRoot = this->find();
        RPtr thatRoot = that.find();
        if (thisRoot == thatRoot)
          return thisRoot;
        if (thisRoot->m_size < thatRoot->m_size)
        {
          RPtr tempRoot = thisRoot;
          thisRoot = thatRoot;
          thatRoot = tempRoot;
        }
        thatRoot->m_parent = thisRoot;
        thisRoot->m_size = thisRoot->m_size + thatRoot->m_size;
        return thisRoot;
      }

    private:

      const int m_id;
      TElement m_elem;
      int m_size;
      RPtr m_parent;
  }; // end class DisjointSet


} // end namespace parakram

