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


  bdd_ptr BddWrapper::getUncountedBdd() const
  {
    return m_bdd;
  }

  bdd_ptr BddWrapper::getCountedBdd() const
  {
    return bdd_dup(m_bdd);
  }


} // end namespace test
