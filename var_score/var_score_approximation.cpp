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
  
  using dd::BddWrapper;
  using dd::BddVectorWrapper;

  class ExactImpl : public var_score::ApproximationMethod
  {
    public:
      void process(
          BddWrapper const & q,
          BddWrapper const & t1,
          BddWrapper const & t2,
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        if (vsq.neighboringFactors(q).size() == 2)
        {
          blif_solve_log(DEBUG, "found var with exactly two factors");
          auto t = BddWrapper(bdd_and_exists(manager, 
                                             t1.getUncountedBdd(), 
                                             t2.getUncountedBdd(), 
                                             q.getUncountedBdd()), 
                              manager);
          vsq.removeFactor(t1);
          vsq.removeFactor(t2);
          vsq.removeVar(q);
          vsq.addFactor(t);
        }
        else
        {
          blif_solve_log(DEBUG, "merging two factors");
          auto t = t1 * t2;
          vsq.removeFactor(t1);
          vsq.removeFactor(t2);
          vsq.addFactor(t);
        }
      }
  };


  class EarlyQuantificationImpl: public var_score::ApproximationMethod
  {
    public:
      void process(
          BddWrapper const & q,
          BddWrapper const &, // not needed
          BddWrapper const &, // not needed
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        blif_solve_log(DEBUG, "early quantification of a variable");
        auto neigh = vsq.neighboringFactors(q);
        for (auto n: neigh)
        {
          auto newn = n.existentialQuantification(q);
          vsq.removeFactor(n);
          vsq.addFactor(newn);
        }
        vsq.removeVar(q);
         
      }
  };



  struct NewDummyVar {
    BddWrapper dummyVar;
    BddWrapper equalityFactor;
    NewDummyVar(const BddWrapper & dv, const BddWrapper & ef):
      dummyVar(dv),
      equalityFactor(ef)
    { }
  };

  class FactorGraphModifier {
    public:
      FactorGraphModifier(DdManager * manager, const BddWrapper & q, int greatestIndex) :
        m_manager(manager),
        m_q(q),
        m_idx(greatestIndex),
        m_map(),
        m_newFactors(manager),
        m_varsToBeGrouped(manager),
        m_origVars(manager),
        m_newVars(manager)
      { }

      BddWrapper equality(const BddWrapper & a, const BddWrapper & b)
      {
        return (a * b) + ((-a) * (-b));
      }

      NewDummyVar getNewDummyVar(const BddWrapper & var)
      {
        auto mapIt = m_map.find(var);
        if (mapIt == m_map.end())
        {
          m_map[var].push_back(var);
          mapIt = m_map.find(var);
        }
        BddWrapper prevDummyVar = mapIt->second.back();
        BddWrapper resultDv(bdd_new_var_with_index(m_manager, ++m_idx), m_manager);
        mapIt->second.push_back(resultDv);
        auto resultEf = equality(prevDummyVar, resultDv);
        return NewDummyVar(resultDv, resultEf);
      }

      void addNonQFactor(const BddWrapper & factor)
      {
        m_newFactors.push_back(factor);
      }

      void addQFactor(const BddWrapper & factor)
      {
        // replace each non-'q' neighbor 'v' of 'factor' with unique var 'x'
        // and create a new factor 'equalityFactor' asserting 'x' is equal to 'v'
        BddVectorWrapper varsToBeSubstituted(m_manager);
        BddVectorWrapper varsToSubstituteWith(m_manager);
        BddWrapper one(bdd_one(m_manager), m_manager);
        m_varsToBeGrouped.push_back(one);
        BddWrapper equalityFactor = one;
        BddWrapper nfSup = factor.support();
        while (nfSup != one)
        {
          // get nextVar from support and remove it from support
          BddWrapper nextVar = nfSup.varWithLowestIndex();
          nfSup = nfSup.cubeDiff(nextVar);

          // skip q
          if (nextVar == m_q)
            continue;

          // get a unique dummy var for "nextVar"
          auto newDummyVar = getNewDummyVar(nextVar);
          varsToBeSubstituted.push_back(nextVar);
          varsToSubstituteWith.push_back(newDummyVar.dummyVar);
          m_origVars.push_back(nextVar);
          m_newVars.push_back(newDummyVar.dummyVar);
          auto lastIndex = m_varsToBeGrouped->size() - 1;
          m_varsToBeGrouped.set(lastIndex, m_varsToBeGrouped.get(lastIndex) * newDummyVar.dummyVar);
          equalityFactor = equalityFactor * newDummyVar.equalityFactor;
        }
        BddWrapper newFactor(
            bdd_substitute_vars(
              m_manager, 
              factor.getUncountedBdd(), 
              &(varsToBeSubstituted->front()), 
              &(varsToSubstituteWith->front()), 
              varsToBeSubstituted->size()),
            m_manager);
        m_newFactors.push_back(newFactor);
        m_newFactors.push_back(equalityFactor);
      }

      BddVectorWrapper const & getNewFactors() const
      {
        return m_newFactors;
      }

      BddVectorWrapper const & getVarsToBeGrouped() const
      {
        return m_varsToBeGrouped;
      }

      BddWrapper reverseSubstitute(const BddWrapper & factor)
      {
        return BddWrapper(
            bdd_substitute_vars(m_manager, factor.getUncountedBdd(), &(m_newVars->front()), &(m_origVars->front()), m_newVars->size()),
            m_manager);
      }

      static void testDummyVarMap(DdManager * manager)
      {
        BddWrapper q(bdd_new_var_with_index(manager, 5), manager);
        BddWrapper a(bdd_new_var_with_index(manager, 10), manager);
        BddWrapper b(bdd_new_var_with_index(manager, 20), manager);
        BddWrapper c(bdd_new_var_with_index(manager, 30), manager);
        
        FactorGraphModifier fgm(manager, q, 30);
        auto a1 = fgm.getNewDummyVar(a);
        auto a1dv_expected = BddWrapper(bdd_new_var_with_index(manager, 31), manager);
        assert(a1.dummyVar  == a1dv_expected);
        auto z = a1.equalityFactor * a * (-a1.dummyVar);
        BddWrapper zero(bdd_zero(manager), manager);
        assert(z == zero);

        auto b1 = fgm.getNewDummyVar(b);
        auto b1dv_expected = BddWrapper(bdd_new_var_with_index(manager, 32), manager);
        assert(b1.dummyVar == b1dv_expected);
        z = b1.equalityFactor * b * (-b1.dummyVar);
        assert(z == zero);

        auto a2 = fgm.getNewDummyVar(a);
        auto a2dv_expected = BddWrapper(bdd_new_var_with_index(manager, 33), manager);
        assert(a2.dummyVar == a2dv_expected);
        z = a2.equalityFactor * a1.dummyVar  * (-a2.dummyVar);
        assert(z == zero);
      }
      

    private:
      DdManager * m_manager;
      BddWrapper m_q;
      int m_idx;
      std::map<BddWrapper, std::vector<BddWrapper> > m_map;
      BddVectorWrapper m_newFactors;
      BddVectorWrapper m_varsToBeGrouped;
      BddVectorWrapper m_origVars;
      BddVectorWrapper m_newVars;
  };





  class FactorGraphImpl: public var_score::ApproximationMethod
  {
    public:

      FactorGraphImpl(int largestSupportSet)
        : m_largestSupportSet(largestSupportSet)
      { }

      void process(
          BddWrapper const & q,
          BddWrapper const &, // unused parameter
          BddWrapper const &, // unused parameter
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        // create modified factor graph
        // with subsituted variables
        // and equality factors
        auto start = blif_solve::now();
        auto qneigh = vsq.neighboringFactors(q);
        auto factors = vsq.getFactorCopies();
        FactorGraphModifier fgm(manager, q, findLargestIndex(factors, manager));
        for (auto factor: factors)
        {
          if (qneigh.count(factor) > 0)
            fgm.addQFactor(factor);
          else
            // not a neighbor, copy as is
            fgm.addNonQFactor(factor);
        }
        auto newFactors = fgm.getNewFactors();
        auto varsToBeGrouped = fgm.getVarsToBeGrouped();
        auto fg = factor_graph_new(manager, &newFactors->front(), newFactors->size());
        for (auto vtbg: *varsToBeGrouped)
          factor_graph_group_vars(fg, vtbg);
        blif_solve_log(INFO, "var_score/FactorGraphImpl: factor graph created in " 
                             << blif_solve::duration(start) 
                             << " sec");

        // run message passing
        start = blif_solve::now();
        int numIterations = factor_graph_converge(fg);
        blif_solve_log(INFO, "var_score/FactorGraphImpl: factor grpah converged in "
                             << blif_solve::duration(start)
                             << " sec");

        // remove old factors from vsq
        for (auto const & neigh: qneigh)
          vsq.removeFactor(neigh);
        vsq.removeVar(q);

        // collect messages, reverse substitute, and insert into vsq
        std::vector<BddWrapper> messageVec;
        for (auto vtbg: *varsToBeGrouped)
        {
          auto node = factor_graph_get_varnode(fg, vtbg);
          int numMessages;
          auto messages = factor_graph_incoming_messages(fg, node, &numMessages);
          for (auto im = 0; im < numMessages; ++im)
          {
            auto finalFactor = fgm.reverseSubstitute(BddWrapper(bdd_dup(messages[im]), manager));
            vsq.addFactor(finalFactor);
            bdd_free(manager, messages[im]);
          }
          free(messages);
        }

        // free up stuff
        factor_graph_delete(fg);
      } // end of FactoGraphImpl::process


      static int findLargestIndex(const std::vector<BddWrapper> & factors, DdManager* manager)
      {
        int largestIndex = 0;
        BddWrapper one(bdd_one(manager), manager);
        for (const auto f: factors)
        {
          BddWrapper sup = f.support();
          while (sup != one)
          {
            int idx = bdd_get_lowest_index(manager, sup.getUncountedBdd());
            BddWrapper v(bdd_new_var_with_index(manager, idx), manager);
            largestIndex = std::max(largestIndex, idx);
            sup = sup.cubeDiff(v);
          }
        }
        return largestIndex;
      }

    private:
      int m_largestSupportSet;
  };





  void testFindLargestIndex(DdManager * manager)
  {
    BddWrapper a(bdd_new_var_with_index(manager, 10), manager);
    BddWrapper b(bdd_new_var_with_index(manager, 20), manager);
    BddWrapper c(bdd_new_var_with_index(manager, 30), manager);
    BddWrapper ab = a * b;
    BddWrapper bc = b * c;

    std::vector<BddWrapper> factors{ab, bc};
    int greatestIndex = FactorGraphImpl::findLargestIndex(factors, manager);
    assert(greatestIndex == 30);
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
    FactorGraphModifier::testDummyVarMap(manager);
  }



} // end namespace var_score
