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

#include "blif_solve_method.h"
#include <dd.h>
#include <memory>

namespace {

  using namespace blif_solve;

  // ***** Class *****
  // ExactAndAccumulate
  // An implementation for BlifSolveMethod
  // Uses cudd to conjoin all the factors and then
  // quantify out the variables
  // *****************
  class ExactAndAccumulate: public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override 
      {
        auto manager = blif_factors.getDdManager();
        auto func = bdd_one(manager);
        bdd_ptr_set result;
        {
          auto factors = blif_factors.getFactors();
          for (auto fi = factors->cbegin(); fi != factors->cend(); ++fi)
            bdd_and_accumulate(manager, &func, *fi);
          auto piVars = blif_factors.getPiVars();
          result.insert(bdd_forsome(manager, func, piVars));
        }
        bdd_free(manager, func);
        return result;
      }
  }; // end class ExactAndAccumulate





  // ***** Class *****
  // ExactAndAbstractMulti
  // An implementation for BlifSolveMethod
  // Uses the newly implemented cudd method Cudd_bddAndAbstractMulti
  //   to conjoin all the factors
  //   and abstract away all primary input variables
  //   in a single pass
  // *****************
  class ExactAndAbstractMulti
    : public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override
      {
        auto manager = blif_factors.getDdManager();
        auto factors = blif_factors.getFactors();
        bdd_ptr_set factor_set(factors->cbegin(), factors->cend());
        auto cube = blif_factors.getPiVars();
        bdd_ptr_set result;
        result.insert(bdd_and_exists_multi(manager, factor_set, cube));
        return result;
      }
  };

}// end anonymous namespace



namespace blif_solve
{
  // ***** Function *****
  // BlifSolveMethod :: createBlifSolveMethod
  // Takes a string and creates an impl of the BlifSolveMethod class
  // ********************
  BlifSolveMethodCptr BlifSolveMethod::createBlifSolveMethod(std::string const & bsmStr)
  {

    if ("ExactAndAccumulate" == bsmStr)
      return std::make_shared<ExactAndAccumulate const>();
    else if ("ExactAndAbstractMulti" == bsmStr)
      return std::make_shared<ExactAndAbstractMulti const>();
    else if (bsmStr == "FactorGraphApprox"
        || bsmStr == "FactorGraphExact"
        || bsmStr == "AcyclicViaForAll"
        || bsmStr == "Skip")
      throw std::runtime_error("BlifSolveMethod for '" + bsmStr + "' not yet implemented.");
    else
      throw std::runtime_error("Invalid BlifSolveMethod '" + bsmStr + "', "
          "expecting one of ExactAndAccumulate/ExactAndAbstractMulti/"
          "FactorGraphApprox/FactorGraphExact/AcyclicViaForAll/Skip");
  }

} // end namespace blif_solve
