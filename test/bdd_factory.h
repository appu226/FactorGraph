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

#include <dd.h>

namespace test
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

      bdd_ptr getUncountedBdd() const;
      bdd_ptr getCountedBdd() const;

    private:
      bdd_ptr m_bdd;
      DdManager * m_manager;

  }; // end class BddWrapper

} // end namespace test
