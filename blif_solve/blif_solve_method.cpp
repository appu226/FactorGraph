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
#include <factor_graph.h>
#include <memory>

namespace {

  using namespace blif_solve;

  // ***** Class *****
  // FactorGraphApprox
  // An implementation for BlifSolveMethod
  // Apply the factor graph algorithm to compute the transition relation
  //   - create the factor_graph using the set of nodes in the network
  //   - merge all the var nodes that are not pi<nnn> (primary inputs) into a single node R
  //   - pass messages, collect the conjunction of messages coming into R
  // *****************
  class FactorGraphApprox:
    public BlifSolveMethod
  {
    public:
      FactorGraphApprox(int varNodeSize):
        m_varNodeSize(varNodeSize)
      { }

      bdd_ptr_set solve(BlifFactors const & blifFactors) const override
      {
        // collect from the network
        // the info required to create a factor graph
        auto funcs = blifFactors.getFactors();      // the set of functions
        auto ddm = blifFactors.getDdManager();

        // create factor graph
        auto start = now();
        factor_graph * fg = factor_graph_new(ddm, &(funcs->front()), funcs->size());
        blif_solve_log(INFO, "Created factor graph with "
                              << funcs->size() << " functions in "
                              << duration(start) << " secs");



        // group the non-pi variables in the factor graph
        int varNodeSize = m_varNodeSize;
        if (0 >= varNodeSize)
          varNodeSize = fg->num_vars;
        auto nonPiVars = blifFactors.getNonPiVars();
        std::vector<bdd_ptr> nonPiVarGroups;
        int lastSize = varNodeSize;
        for (auto npv: *nonPiVars)
        {
          if (lastSize >= varNodeSize)
          {
            nonPiVarGroups.push_back(bdd_one(ddm));
            lastSize = 0;
          }
          bdd_and_accumulate(ddm, &nonPiVarGroups.back(), npv);
          ++lastSize;
        }

        for (auto nonPiVarGroup: nonPiVarGroups)
          factor_graph_group_vars(fg, nonPiVarGroup);

        start = now();
        blif_solve_log(INFO, "Grouped non-pi variables in "
                              << duration(start) << " secs");




        // pass messages till convergence
        start = now();
        factor_graph_converge(fg);
        blif_solve_log(INFO, "Factor graph messages have converged in "
                             << duration(start) << " secs");




        // compute the result by conjoining all incoming messages
        start = now();
        bdd_ptr_set result;
        for (auto nonPiVarCube: nonPiVarGroups)
        {
          fgnode * V = factor_graph_get_varnode(fg, nonPiVarCube);
          int num_messages;
          bdd_ptr *messages = factor_graph_incoming_messages(fg, V, &num_messages);
          for (int mi = 0; mi < num_messages; ++mi)
          {
            result.insert(messages[mi]);
          }
          free(messages);
        }
        blif_solve_log(INFO, "Computed final factor graph result in "
                              << duration(start) << " secs");

        // clean-up and return
        factor_graph_delete(fg);
        return result;

      }

    private:
      int m_varNodeSize;
  }; // end of class FactorGraphApprox



  

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

  BlifSolveMethodCptr BlifSolveMethod::createExactAndAccumulate()
  {
    return std::make_shared<ExactAndAccumulate>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createExactAndAbstractMulti()
  {
    return std::make_shared<ExactAndAbstractMulti>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createFactorGraphApprox(int varNodeMergeLimit)
  {
    return std::make_shared<FactorGraphApprox>(varNodeMergeLimit);
  }


} // end namespace blif_solve
