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

namespace dd
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
    return BddWrapper(bdd_not(m_bdd), m_manager);
  }

  bdd_ptr BddWrapper::operator ! () const
  {
    return m_bdd;
  }

  bdd_ptr BddWrapper::operator * () const
  {
    return bdd_dup(m_bdd);
  }

  BddWrapper BddWrapper::support() const
  {
    return BddWrapper(bdd_support(m_manager, m_bdd), m_manager);
  }

  BddWrapper BddWrapper::cubeIntersection(const BddWrapper & that) const
  {
    return BddWrapper(bdd_cube_intersection(m_manager, m_bdd, that.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::cubeUnion(const BddWrapper & that) const
  {
    return BddWrapper(bdd_cube_union(m_manager, m_bdd, that.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::cubeDiff(const BddWrapper & that) const
  {
    return BddWrapper(bdd_cube_diff(m_manager, m_bdd, that.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::existentialQuantification(const BddWrapper & variables) const
  {
    return BddWrapper(bdd_forsome(m_manager, m_bdd, variables.m_bdd), m_manager);
  }

  BddWrapper BddWrapper::varWithLowestIndex() const
  {
    return BddWrapper(bdd_new_var_with_index(m_manager, bdd_get_lowest_index(m_manager, getUncountedBdd())), m_manager);
  }

  bdd_ptr BddWrapper::getUncountedBdd() const
  {
    return m_bdd;
  }

  bdd_ptr BddWrapper::getCountedBdd() const
  {
    return bdd_dup(m_bdd);
  }

  bool BddWrapper::operator <(const BddWrapper & that) const
  {
    return m_bdd < that.m_bdd;
  }

  std::vector<BddWrapper> BddWrapper::fromVector(const std::vector<bdd_ptr> & bddVec, DdManager * manager)
  {
    std::vector<BddWrapper> result;
    for (auto p: bddVec)
      result.push_back(BddWrapper(p, manager));
    return result;
  }

  std::set<BddWrapper> BddWrapper::fromSet(const std::set<bdd_ptr> & bddSet, DdManager * manager)
  {
    std::set<BddWrapper> result;
    for (auto p: bddSet)
      result.insert(BddWrapper(p, manager));
    return result;
  }






  BddVectorWrapper::BddVectorWrapper(DdManager * manager):
    m_vector(),
    m_manager(manager)
  { }

  BddVectorWrapper::BddVectorWrapper(const std::vector<bdd_ptr> & bddVector,
      DdManager * manager):
    m_vector(bddVector),
    m_manager(manager)
  { }

  BddVectorWrapper::BddVectorWrapper(const BddVectorWrapper& that):
    m_vector(that.m_vector),
    m_manager(that.m_manager)
  {
    for (auto p: m_vector)
      bdd_dup(p);
  }

  BddVectorWrapper & BddVectorWrapper::operator = (BddVectorWrapper & that)
  {
    while(!m_vector.empty())
    {
      bdd_free(m_manager, m_vector.back());
      m_vector.pop_back();
    }
    m_manager = that.m_manager;
    for (auto p: that.m_vector)
      m_vector.push_back(bdd_dup(p));
    return *this;
  }

} // end namespace dd
