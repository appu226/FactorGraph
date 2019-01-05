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

      bdd_ptr getUncountedBdd() const;
      bdd_ptr getCountedBdd() const;

    private:
      bdd_ptr m_bdd;
      DdManager * m_manager;

  }; // end class BddWrapper

} // end namespace test
