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



  struct NewDummyVar {
    bdd_ptr dummyVar;
    bdd_ptr equalityFactor;
  };

  class DummyVarMap {
    public:
      DummyVarMap(DdManager * manager, int greatestIndex) :
        m_manager(manager),
        m_idx(greatestIndex),
        m_map()
      { }

      ~DummyVarMap()
      {
        for (const auto & kv: m_map)
        {
          bdd_free(m_manager, kv.first);
          for (auto v: kv.second)
            bdd_free(m_manager, v);
        }
      }

      bdd_ptr equality(bdd_ptr a, bdd_ptr b)
      {
        auto a_and_b = bdd_and(m_manager, a, b);
        auto nota = bdd_not(a);
        auto notb = bdd_not(b);
        auto nota_and_notb = bdd_and(m_manager, nota, notb);
        bdd_free(m_manager, nota);
        bdd_free(m_manager, notb);
        auto result = bdd_or(m_manager, a_and_b, nota_and_notb);
        bdd_free(m_manager, a_and_b);
        bdd_free(m_manager, nota_and_notb);
        return result;
      }

      NewDummyVar getNewDummyVar(bdd_ptr var)
      {
        auto mapIt = m_map.find(var);
        if (mapIt == m_map.end())
        {
          m_map[bdd_dup(var)].push_back(bdd_dup(var));
          mapIt = m_map.find(var);
        }
        bdd_ptr prevDummyVar = mapIt->second.back();
        NewDummyVar result;
        result.dummyVar = bdd_new_var_with_index(m_manager, ++m_idx);
        mapIt->second.push_back(bdd_dup(result.dummyVar));
        result.equalityFactor = equality(prevDummyVar, result.dummyVar);
        return result;
      }


      static void testDummyVarMap(DdManager * manager)
      {
        bdd_ptr a = bdd_new_var_with_index(manager, 10);
        bdd_ptr b = bdd_new_var_with_index(manager, 20);
        bdd_ptr c = bdd_new_var_with_index(manager, 30);
        
        DummyVarMap dvm(manager, 30);
        auto a1 = dvm.getNewDummyVar(a);
        auto a1dv_expected = bdd_new_var_with_index(manager, 31);
        assert(a1.dummyVar  == a1dv_expected);
        auto z = bdd_and(manager, a1.equalityFactor, a);
        auto not_a1 = bdd_not(a1.dummyVar);
        bdd_and_accumulate(manager, &z, not_a1);
        auto zero = bdd_zero(manager);
        assert(z == zero);
        bdd_free(manager, z);
        bdd_free(manager, not_a1);
        bdd_free(manager, a1dv_expected);
        bdd_free(manager, a1.equalityFactor);

        auto b1 = dvm.getNewDummyVar(b);
        auto b1dv_expected = bdd_new_var_with_index(manager, 32);
        assert(b1.dummyVar == b1dv_expected);
        z = bdd_and(manager, b1.equalityFactor, b);
        auto not_b1 = bdd_not(b1.dummyVar);
        bdd_and_accumulate(manager, &z, not_b1);
        assert(z == zero);
        bdd_free(manager, z);
        bdd_free(manager, not_b1);
        bdd_free(manager, b1dv_expected);
        bdd_free(manager, b1.equalityFactor);
        bdd_free(manager, b1.dummyVar);

        auto a2 = dvm.getNewDummyVar(a);
        auto a2dv_expected = bdd_new_var_with_index(manager, 33);
        assert(a2.dummyVar == a2dv_expected);
        z = bdd_and(manager, a2.equalityFactor, a1.dummyVar);
        auto not_a2 = bdd_not(a2.dummyVar);
        bdd_and_accumulate(manager, &z, not_a2);
        assert(z == zero);
        bdd_free(manager, a2dv_expected);
        bdd_free(manager, not_a2);
        bdd_free(manager, z);
        bdd_free(manager, a2.dummyVar);
        bdd_free(manager, a2.equalityFactor);
        bdd_free(manager, a1.dummyVar);

        bdd_free(manager, zero);
        bdd_free(manager, c);
        bdd_free(manager, b);
        bdd_free(manager, a);
      }
      

    private:
      DdManager * m_manager;
      int m_idx;
      std::map<bdd_ptr, std::vector<bdd_ptr> > m_map;
  };





  class FactorGraphImpl: public var_score::ApproximationMethod
  {
    public:

      FactorGraphImpl(int largestSupportSet)
        : m_largestSupportSet(largestSupportSet)
      { }

      void process(
          bdd_ptr q,
          bdd_ptr, // unused parameter
          bdd_ptr, // unused parameter
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        auto & qneigh = vsq.neighboringFactors(q);
        auto varsToProjectOn = findVarsToProjectOn(qneigh, q, manager);
        auto factors = vsq.getFactorCopies();
        DummyVarMap dummyVarMap(manager, findLargestIndex(factors, manager));
        std::vector<bdd_ptr> newFactors;
        std::vector<bdd_ptr> varsToBeGrouped;
        for (auto factor: factors)
        {
          if (qneigh.count(factor) > 0)
          {
            // 'factor' is a neighbor of 'q'
            // replace each non-'q' neighbor 'v' of 'factor' with unique var 'x'
            // and create a new factor 'factorx' asserting 'x' is equal to 'v'
            std::vector<bdd_ptr> varsToBeSubstituted;
            std::vector<bdd_ptr> varsToSubstituteWith;
            varsToBeGrouped.push_back(bdd_one(manager));
            bdd_ptr equalityFactor = bdd_one(manager);
            bdd_ptr nfSup = bdd_support(manager, factor);
            bdd_ptr one = bdd_one(manager);
            while (nfSup != one)
            {
              // get nextVar from support and remove it from support
              bdd_ptr nextVar = bdd_new_var_with_index(manager, bdd_get_lowest_index(manager, nfSup));
              bdd_ptr nfSupNext = bdd_cube_diff(manager, nfSup, nextVar);
              bdd_free(manager, nfSup);
              nfSup = nfSupNext;
              
              // skip q
              if (nextVar == q)
              {
                bdd_free(manager, nextVar);
                continue;
              }

              // get a unique dummy var for "nextVar"
              auto newDummyVar = dummyVarMap.getNewDummyVar(nextVar);
              varsToBeSubstituted.push_back(nextVar);
              varsToSubstituteWith.push_back(newDummyVar.dummyVar);
              bdd_and_accumulate(manager, &varsToBeGrouped.back(), newDummyVar.dummyVar);
              bdd_and_accumulate(manager, &equalityFactor, newDummyVar.equalityFactor);
              bdd_free(manager, newDummyVar.equalityFactor);
              bdd_free(manager, nextVar);
            }
            bdd_free(manager, nfSup);
            bdd_free(manager, one);
            bdd_ptr newFactor = bdd_substitute_vars(
                manager, 
                factor, 
                &(varsToBeSubstituted.front()), 
                &(varsToSubstituteWith.front()), 
                varsToBeSubstituted.size());
            newFactors.push_back(newFactor);
            newFactors.push_back(equalityFactor);
          }
          else
            // not a neighbor, copy as is
            newFactors.push_back(bdd_dup(factor));
        }




        for (auto factor: newFactors)
          bdd_free(manager, factor);
        for (auto vtbg: varsToBeGrouped)
          bdd_free(manager, vtbg);
        for (auto factor: factors)
          bdd_free(manager, factor);

        throw std::runtime_error("FactorGraphImpl::process not yet implemented");
      } // end of FactoGraphImpl::process


      static int findLargestIndex(const std::vector<bdd_ptr> & factors, DdManager* manager)
      {
        int largestIndex = 0;
        for (const auto f: factors)
        {
          bdd_ptr sup = bdd_support(manager, f);
          bdd_ptr one = bdd_one(manager);
          while (sup != one)
          {
            int idx = bdd_get_lowest_index(manager, sup);
            bdd_ptr v = bdd_new_var_with_index(manager, idx);
            largestIndex = std::max(largestIndex, idx);
            bdd_ptr nextSup = bdd_cube_diff(manager, sup, v);
            bdd_free(manager, sup);
            bdd_free(manager, v);
            sup = nextSup;
          }
          bdd_free(manager, sup);
          bdd_free(manager, one);
        }
        return largestIndex;
      }

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

    private:
      int m_largestSupportSet;
  };





  void testFindLargestIndex(DdManager * manager)
  {
    bdd_ptr a = bdd_new_var_with_index(manager, 10);
    bdd_ptr b = bdd_new_var_with_index(manager, 20);
    bdd_ptr c = bdd_new_var_with_index(manager, 30);
    bdd_ptr ab = bdd_and(manager, a, b);
    bdd_ptr bc = bdd_and(manager, b, c);

    std::vector<bdd_ptr> factors{ab, bc};
    int greatestIndex = FactorGraphImpl::findLargestIndex(factors, manager);
    assert(greatestIndex == 30);

    bdd_free(manager, a);
    bdd_free(manager, b);
    bdd_free(manager, c);
    bdd_free(manager, ab);
    bdd_free(manager, bc);
  }


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

  void ApproximationMethod::runUnitTests(DdManager * manager)
  {
    testFindLargestIndex(manager);
    DummyVarMap::testDummyVarMap(manager);
  }



} // end namespace var_score
