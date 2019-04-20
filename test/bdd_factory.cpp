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



#include "bdd_factory.h"

#include <stdexcept>

namespace test
{

  BddWrapper::BddWrapper(bdd_ptr myBdd, DdManager * manager) :
    m_bdd(myBdd),
    m_manager(manager)
  { }

  BddWrapper::BddWrapper(BddWrapper const & that):
    m_bdd(that.getCountedBdd()),
    m_manager(that.m_manager)
  { }

  BddWrapper & BddWrapper::operator = (BddWrapper const & that)
  {
    bdd_free(m_manager, m_bdd);
    m_manager = that.m_manager;
    m_bdd = bdd_dup(that.m_bdd);
    return *this;
  }

  BddWrapper::~BddWrapper()
  {
    bdd_free(m_manager, m_bdd);
  }

  BddWrapper BddWrapper::operator + (BddWrapper const & that) const
  {
    if (m_manager != that.m_manager)
      throw std::runtime_error("managers must match for bdd operations");
    return BddWrapper(bdd_or(m_manager, m_bdd, that.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::operator * (BddWrapper const & that) const
  {
    if (m_manager != that.m_manager)
      throw std::runtime_error("managers must match for bdd operations");
    return BddWrapper(bdd_and(m_manager, m_bdd, that.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::operator - () const
  {
    return BddWrapper(bdd_not(m_manager, m_bdd), m_manager);
  }

  bdd_ptr BddWrapper::operator ! () const
  {
    return m_bdd;
  }

  bdd_ptr BddWrapper::operator * () const
  {
    return bdd_dup(m_bdd);
  }

  bdd_ptr BddWrapper::getUncountedBdd() const
  {
    return m_bdd;
  }

  bdd_ptr BddWrapper::getCountedBdd() const
  {
    return bdd_dup(m_bdd);
  }


} // end namespace test
