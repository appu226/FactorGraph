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

#include "dd.h"

#include <vector>
#include <set>

namespace dd
{


  class BddWrapper
  {
    public:
      BddWrapper(bdd_ptr elem_bdd, DdManager * manager);
      BddWrapper(BddWrapper const & that);
      BddWrapper & operator = (BddWrapper const & that);

      ~BddWrapper();

      BddWrapper operator + (BddWrapper const & that) const;
      BddWrapper operator * (BddWrapper const & that) const;
      BddWrapper operator - () const;

      bdd_ptr operator ! () const;
      bdd_ptr operator * () const;

      BddWrapper cubeIntersection(const BddWrapper & that) const;
      BddWrapper cubeUnion(const BddWrapper & that) const;
      BddWrapper cubeDiff(const BddWrapper & that) const;

      BddWrapper support() const;

      BddWrapper existentialQuantification(const BddWrapper & variables) const;

      BddWrapper varWithLowestIndex() const;

      bdd_ptr getUncountedBdd() const;
      bdd_ptr getCountedBdd() const;
      DdManager * getManager() const;

      static std::vector<BddWrapper> fromVector(const std::vector<bdd_ptr>& bddVec, DdManager * manager);
      static std::set<BddWrapper> fromSet(const std::set<bdd_ptr> & bddSet, DdManager * manager);

      bool operator < (const BddWrapper & that) const;
      bool operator == (const BddWrapper & that) const { return m_bdd == that.m_bdd; }
      bool operator != (const BddWrapper & that) const { return m_bdd != that.m_bdd; }


    private:
      bdd_ptr m_bdd;
      DdManager * m_manager;

  }; // end class BddWrapper



  class BddVectorWrapper
  {
    public:
      BddVectorWrapper(DdManager * manager);
      BddVectorWrapper(const std::vector<bdd_ptr> & bddVector,
                       DdManager * manager);
      BddVectorWrapper(const BddVectorWrapper& that);
      BddVectorWrapper & operator = (BddVectorWrapper & that);

      std::vector<bdd_ptr> const & operator * () const { return m_vector; }
      std::vector<bdd_ptr>       & operator * ()       { return m_vector; }

      std::vector<bdd_ptr> const * operator -> () const { return &m_vector; }
      std::vector<bdd_ptr>       * operator -> ()       { return &m_vector; }

      void push_back (BddWrapper const & f) { m_vector.push_back (f.getCountedBdd()); }

      BddWrapper get(size_t index) const { return {bdd_dup(m_vector[index]), m_manager}; }
      void set(size_t index, BddWrapper const & value) { bdd_free(m_manager, m_vector[index]); m_vector[index] = value.getCountedBdd(); }

      ~BddVectorWrapper()
      {
        for (auto p: m_vector)
          bdd_free(m_manager, p);
      }

    private:
      std::vector<bdd_ptr> m_vector;
      DdManager * m_manager;

  }; // end of class BddVectorWrapper

} // end namespace dd
