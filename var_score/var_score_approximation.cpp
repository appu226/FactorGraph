/*

Copyright 2020 Parakram Majumdar

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

#include "var_score_approximation.h"
#include "var_score_quantification.h"

#include <log.h>
#include <approx_merge.h>
#include <factor_graph.h>

#include <algorithm>
#include <sstream>

namespace {

  class ExactImpl : public var_score::ApproximationMethod
  {
    public:
      void process(
          bdd_ptr q,
          bdd_ptr t1,
          bdd_ptr t2,
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        if (vsq.neighboringFactors(q).size() == 2)
        {
          blif_solve_log(DEBUG, "found var with exactly two factors");
          auto t = bdd_and_exists(manager, t1, t2, q);
          vsq.removeFactor(t1);
          vsq.removeFactor(t2);
          vsq.removeVar(q);
          vsq.addFactor(t);
          bdd_free(manager, t);
        }
        else
        {
          blif_solve_log(DEBUG, "merging two factors");
          auto t = bdd_and(manager, t1, t2);
          vsq.removeFactor(t1);
          vsq.removeFactor(t2);
          vsq.addFactor(t);
          bdd_free(manager, t);
        }
      }
  };


  class EarlyQuantificationImpl: public var_score::ApproximationMethod
  {
    public:
      void process(
          bdd_ptr q,
          bdd_ptr t1,
          bdd_ptr t2,
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        blif_solve_log(DEBUG, "early quantification of a variable");
        auto t1_q = bdd_forsome(manager, t1, q);
        vsq.removeFactor(t1);
        vsq.addFactor(t1_q);
        bdd_free(manager, t1_q);
      }
  };


  class FactorGraphImpl: public var_score::ApproximationMethod
  {
    public:

      FactorGraphImpl(int largestSupportSet)
        : m_largestSupportSet(largestSupportSet)
      { }

      void process(
          bdd_ptr q,
          bdd_ptr,
          bdd_ptr,
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        auto & qneigh = vsq.neighboringFactors(q);
        auto varsToProjectOn = findVarsToProjectOn(qneigh, q, manager);
        auto factors = vsq.getFactorCopies();
        auto mergeResults = blif_solve::merge(manager, factors, *varsToProjectOn, m_largestSupportSet);
        for (auto factor: factors)
          bdd_free(manager, factor);
        auto & funcGroups = *mergeResults.factors;
        auto start = blif_solve::now();
        factor_graph * fg = factor_graph_new(manager, &funcGroups.front(), funcGroups.size());
        blif_solve_log(INFO, "Created factor graph with "
            << funcGroups.size() << " functions on "
            << (mergeResults.variables->size() + 1) << " variables in "
            << blif_solve::duration(start) << " secs");
        for (auto funcGroup: funcGroups)
          bdd_free(manager, funcGroup);

        auto & varGroups = *mergeResults.variables;
        for (auto varGroup: varGroups)
          factor_graph_group_vars(fg, varGroup);
        int numIterations = factor_graph_converge(fg);
        blif_solve_log(INFO, "Factor graph convergence took "
            << numIterations << " iterations in " << blif_solve::duration(start) << " secs");


        for (auto neigh: qneigh)
          vsq.removeFactor(neigh);
        vsq.removeVar(q);

        for (auto varToBeProjectedOn: *mergeResults.variables)
        {
          fgnode * V = factor_graph_get_varnode(fg, varToBeProjectedOn);
          int num_messages;
          bdd_ptr *messages = factor_graph_incoming_messages(fg, V, &num_messages);
          for (int mi = 0; mi < num_messages; ++mi)
          {
            vsq.addFactor(messages[mi]);
            bdd_free(manager, messages[mi]);
          }
        }
      } // end of FactoGraphImpl::process


    private:

      int m_largestSupportSet;

      static
        blif_solve::MergeResults::FactorVec 
        findVarsToProjectOn(
            const std::set<bdd_ptr> & factors, 
            bdd_ptr varToQuantify, 
            DdManager * manager)
      {
        auto support = bdd_one(manager);
        for (auto factor: factors)
        {
          auto fs = bdd_support(manager, factor);
          bdd_and_accumulate(manager, &support, fs);
          bdd_free(manager, fs);
        }
        auto result = std::make_shared<std::vector<bdd_ptr> >();
        auto end = bdd_one(manager);
        while(support != end)
        {
          auto nextvar = bdd_new_var_with_index(manager, bdd_get_lowest_index(manager, support));
          if (nextvar == varToQuantify)
            bdd_free(manager, nextvar);
          else
            result->push_back(nextvar);
          auto nextsupport = bdd_cube_diff(manager, support, nextvar);
          bdd_free(manager, support);
          support = nextsupport;
        }
        bdd_free(manager, support);
        bdd_free(manager, end);
        return result;
      }
  };


} // end anonymous namespace





namespace var_score
{

  ApproximationMethod::CPtr ApproximationMethod::createExact()
  {
    return std::make_shared<ExactImpl>();
  }

  ApproximationMethod::CPtr ApproximationMethod::createEarlyQuantification()
  {
    return std::make_shared<EarlyQuantificationImpl>();
  }

  ApproximationMethod::CPtr ApproximationMethod::createFactorGraph(int largestSupportSet)
  {
    return std::make_shared<FactorGraphImpl>(largestSupportSet);
  }



} // end namespace var_score
