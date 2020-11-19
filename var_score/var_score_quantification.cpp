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


#include "var_score_quantification.h"

#include <log.h>
#include <srt.h>

#include <memory>
#include <algorithm>
#include <cctype>
#include <sstream>





//Declarations for helper functions
namespace {

  double getBddSize(DdManager* manager, bdd_ptr b1, bdd_ptr b2 = NULL);

  class ApproximationMethodImpl {
    public:

      typedef std::shared_ptr<ApproximationMethodImpl const> CPtr;
      static CPtr create(var_score::VarScoreQuantification::ApproximationMethod am);

      virtual void process(
          bdd_ptr q, 
          bdd_ptr f1, 
          bdd_ptr f2, 
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const = 0;

  };

} // end anonymous namespace








namespace var_score {

  /* the VarScore algorithm
   * VARSCOREBASICSTEP(F,Q)
   *   if there exists a variableq∈Qsuch thatqappears inthe support of only one BDD T∈F
   *       F←F\{T}∪{∃q.T}
   *       Q←Q\{q}
   *   else
   *       compute heuristic score VARSCORE for each variable inQ
   *       let q ∈ Q be the variable with the lowest score
   *       let T1, T2 ∈ F be the two smallest BDDs such that q ∈ Supp(T1) ∩ Supp(T2)
   *       if q /∈ ⋃_{Ti∈F\{T1,T2}} Supp(Ti)
   *            // use BDDANDEXISTS for efficiency
   *            F←F\{T1,T2}∪{∃q.T1∧T2}
   *            q←Q\{q}
   *       else
   *            F←F\{T1,T2}∪{T1∧T2}
   *        endif
   *   endif
   *   return(F,Q)
   * 
   * (note: /∈ means "not an element of"
   */
  std::vector<bdd_ptr>
    VarScoreQuantification::varScoreQuantification(const std::vector<bdd_ptr> & F, 
        bdd_ptr Q, 
        DdManager * ddm,
        const int maxBddSize,
        ApproximationMethod approximationMethod)
    {
      VarScoreQuantification vsq(F, Q, ddm);
      auto approxImpl = ::ApproximationMethodImpl::create(approximationMethod);
      auto exactImpl = ::ApproximationMethodImpl::create(None);
      while(!vsq.isFinished())
      {
        // vsq.printState();
        auto q = vsq.findVarWithOnlyOneFactor();
        if (q != NULL)
        {
          blif_solve_log(DEBUG, "found var with only one factor");
          auto tv = vsq.neighboringFactors(q);
          assert(tv.size() == 1);
          auto t = *(tv.cbegin());
          auto t_without_q = bdd_forsome(ddm, t, q);
          vsq.removeFactor(t);
          vsq.removeVar(q);
          vsq.addFactor(t_without_q);
          bdd_free(ddm, t_without_q);
        }
        else
        {
          auto q = vsq.varWithLowestScore();
          auto t1t2 = vsq.smallestTwoNeighbors(q);
          auto t1 = t1t2.first;
          auto t2 = t1t2.second;
          if (getBddSize(ddm, t1, t2) > maxBddSize)
            approxImpl->process(q, t1, t2, vsq, ddm);
          else
            exactImpl->process(q, t1, t2, vsq, ddm);
        }
      }
      // vsq.printState();
      return vsq.getFactorCopies();
    }






  VarScoreQuantification::ApproximationMethod VarScoreQuantification::parseApproximationMethod(std::string approximationMethodAsString)
  {
    std::map<std::string, ApproximationMethod> translationMap;
    translationMap["none"] = None;
    translationMap["early_quantification"] = EarlyQuantification;
    translationMap["factor_graph"] = FactorGraph;
    std::transform(approximationMethodAsString.begin(), approximationMethodAsString.end(), approximationMethodAsString.begin(), [](char c){ return std::tolower(c); });
    auto ittm = translationMap.find(approximationMethodAsString);
    if (ittm == translationMap.end())
    {
      std::stringstream ss;
      ss << "Could not parse approximation method '" << approximationMethodAsString << "'. Expected one of { ";
      for (const auto & kv: translationMap)
        ss << kv.first << ", ";
      ss << "}";
      throw std::runtime_error(ss.str());
    }
    else
      return ittm->second;
  }






  VarScoreQuantification::VarScoreQuantification(const std::vector<bdd_ptr> & F, bdd_ptr Q, DdManager * ddm):
    m_factors(F.cbegin(), F.cend()),
    m_vars(),
    m_ddm(ddm)
  {
    for (auto f: m_factors)
      bdd_dup(f);
    bdd_ptr qs = bdd_dup(Q);
    bdd_ptr one = bdd_one(ddm);
    while(qs != one)
    {
      // get next q
      bdd_ptr q = bdd_new_var_with_index(m_ddm, bdd_get_lowest_index(m_ddm, qs));
      {
        // compute new qs
        bdd_ptr qs_rem = bdd_forsome(m_ddm, qs, q);
        bdd_free(m_ddm, qs);
        qs = qs_rem;
      }
      // find neighbors of q
      for (auto f: m_factors)
        if (isNeighbor(f, q))
          m_vars[q].insert(f);
      if (m_vars[q].empty()) // no neighbors, you can ignore
      {
        m_vars.erase(q);
        bdd_free(m_ddm, q);
      }
    }
    bdd_free(m_ddm, qs);
    bdd_free(m_ddm, one);
    blif_solve_log(DEBUG, "Created VarScoreQuantification with " << m_vars.size() << " vars and " << m_factors.size() << " factors");
  }







  bdd_ptr VarScoreQuantification::findVarWithOnlyOneFactor() const
  {
    for(auto vxfs: m_vars)
      if (vxfs.second.size() == 1)
        return vxfs.first;
    return NULL;
  }





  const std::set<bdd_ptr>& VarScoreQuantification::neighboringFactors(bdd_ptr var) const
  {
    auto qit = m_vars.find(var);
    assert(qit != m_vars.end());
    return qit->second;
  }







  void VarScoreQuantification::removeFactor(bdd_ptr factor)
  {
    for (auto& vxfs: m_vars)
      vxfs.second.erase(factor);
    if (m_factors.count(factor) == 0)
      return;
    m_factors.erase(factor);
    bdd_free(m_ddm, factor);
  }






  void VarScoreQuantification::addFactor(bdd_ptr factor) 
  {
    if (m_factors.count(factor) > 0)
      return;
    m_factors.insert(bdd_dup(factor));
    bdd_ptr fsup = bdd_support(m_ddm, factor);
    bdd_ptr one = bdd_one(m_ddm);
    for(auto& vxfs: m_vars)
    {
      bdd_ptr q = vxfs.first;
      bdd_ptr intersection = bdd_cube_intersection(m_ddm, q, fsup);
      if (intersection != one)
        vxfs.second.insert(factor);
      bdd_free(m_ddm, intersection);
    }
    bdd_free(m_ddm, fsup);
    bdd_free(m_ddm, one);
  }





  void VarScoreQuantification::removeVar(bdd_ptr var)
  {
    auto vit = m_vars.find(var);
    if (vit != m_vars.end())
    {
      bdd_free(m_ddm, vit->first);
      m_vars.erase(vit);
    }
  }





  bdd_ptr VarScoreQuantification::varWithLowestScore() const
  {
    bdd_ptr minq = NULL;
    int lowestSize = 0;
    for (auto vxfs: m_vars)
    {
      int size = 0;
      for (auto f: vxfs.second)
        size += bdd_size(f);
      if (minq == NULL || size < lowestSize)
      {
        minq = vxfs.first;
        lowestSize = size;
      }
    }
    assert(minq != NULL);
    return minq;
  }





  std::pair<bdd_ptr, bdd_ptr> VarScoreQuantification::smallestTwoNeighbors(bdd_ptr var) const
  {
    auto qit = m_vars.find(var);
    assert(qit != m_vars.end());
    assert(qit->second.size() >= 2);
    bdd_ptr f1 = NULL, f2 = NULL;
    int s1 = 0, s2 = 0;
    for (auto fit: qit->second)
    {
      bdd_ptr f = fit;
      int s = bdd_size(f);
      if (f1 == NULL || s < s1)
      {
        std::swap(f1, f);
        std::swap(s1, s);
      }
      if (f != NULL && (f2 == NULL || s < s2))
      {
        std::swap(f2, f);
        std::swap(s2, s);
      }
    }
    return std::make_pair(f1, f2);
  }





  bool VarScoreQuantification::isFinished() const {
    for (const auto & vxfs: m_vars)
      if (!vxfs.second.empty())
        return false;
    return true;
  }






  std::vector<bdd_ptr> VarScoreQuantification::getFactorCopies() const
  {
    std::vector<bdd_ptr> result(m_factors.cbegin(), m_factors.cend());
    for (auto f: result)
      bdd_dup(f);
    return result;
  }




  bool VarScoreQuantification::isNeighbor(bdd_ptr f, bdd_ptr g) const
  {
    auto fsup = bdd_support(m_ddm, f);
    auto gsup = bdd_support(m_ddm, g);
    auto common = bdd_cube_intersection(m_ddm, fsup, gsup);
    bool isNotNeighbor = bdd_is_one(m_ddm, common);
    bdd_free(m_ddm, fsup);
    bdd_free(m_ddm, gsup);
    bdd_free(m_ddm, common);
    return ! isNotNeighbor;
  }





  VarScoreQuantification::~VarScoreQuantification()
  {
    for (auto f: m_factors)
      bdd_free(m_ddm, f);
    for (auto vxfs: m_vars)
      bdd_free(m_ddm, vxfs.first);
  }





  void VarScoreQuantification::printState() const
  {
    std::cout << "\n\n======\nFactors:\n\n" << std::endl;
    for (auto f: m_factors)
    {
      bdd_print_minterms(m_ddm, f);
      std::cout << "\n\n" << std::endl;
    }

    std::cout << "\n\n=====\nVariables:\n\n" << std::endl;
    for (const auto & vxfs: m_vars)
    {
      std::cout << "\nvar:\n" << std::endl;
      bdd_print_minterms(m_ddm, vxfs.first);
      std::cout << "\n\nfuncs:\n" << std::endl;
      for (auto f: vxfs.second)
      {
        bdd_print_minterms(m_ddm, f);
        std::cout << "\n\n" << std::endl;
      }
    }
  }




} // end namespace var_score





// definitions for helper functions
namespace {

  /* Heuristic for predicting the bdd size
   * If no variables are common, then size is nb1 + nb2.
   * If all variables are common then size is nb1 * nb2.
   * Is it a bad heuristic? I don't know. You tell me.
   */
  double getBddSize(DdManager * manager, bdd_ptr b1, bdd_ptr b2)
  {
    double s1 = bdd_size(b1);
    if (NULL == b2)
      return s1;
   
    // support sets
    bdd_ptr sp1 = bdd_support(manager, b1);
    bdd_ptr sp2 = bdd_support(manager, b2);
    // intersection support set
    bdd_ptr intsn = bdd_cube_intersection(manager, sp1, sp2);

    double nb1 = bdd_size(b1);
    double nb2 = bdd_size(b2);
    double nsp1 = bdd_size(sp1) - 1;
    double nsp2 = bdd_size(sp2) - 1;
    double nintsn = bdd_size(intsn) - 1;

    bdd_free(manager, sp1);
    bdd_free(manager, sp2);
    bdd_free(manager, intsn);


    return 
      nb1 * (nsp1 - nintsn) / nsp1                  // vars unique to b1
      + nb2 * (nsp2 - nintsn) / nsp2                // vars unique to b2
      + nb1 * nb2 * nintsn * nintsn / nsp1 / nsp2;  // vars common to b1 and b2

  }




  class NoneApproximation : public ApproximationMethodImpl
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


  class EarlyQuantificationImpl: public ApproximationMethodImpl
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


  class FactorGraphImpl: public ApproximationMethodImpl
  {
    public:
      void process(
          bdd_ptr q,
          bdd_ptr,
          bdd_ptr,
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const override
      {
        throw std::runtime_error("FactorGraphImpl not yet implemented");
      }
  };


  ApproximationMethodImpl::CPtr ApproximationMethodImpl::create(var_score::VarScoreQuantification::ApproximationMethod am)
  {
    switch(am)
    {
      case var_score::VarScoreQuantification::None:
        return std::make_shared<NoneApproximation>();
      case var_score::VarScoreQuantification::EarlyQuantification:
        return std::make_shared<EarlyQuantificationImpl>();
      default:
        throw std::runtime_error("Error: unimplemented AppoximationMethodImpl case");
    }
  }

} // end anonymous namespace
