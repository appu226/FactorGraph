#include <dd.h>

#include <vector>
#include <memory>

namespace blif_solve
{

  struct MergeResults
  {
    typedef std::shared_ptr<std::vector<bdd_ptr> > FactorVec;
    FactorVec factors;
    FactorVec variables;
  };

  MergeResults 
    merge(const std::vector<bdd_ptr> & factors, 
          const std::vector<bdd_ptr> & variables, 
          int largestSupportSet);

} // end namespace blif_solve
