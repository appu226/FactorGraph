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

#include <blif_solve_lib/log.h>
#include <factor_graph/srt.h>
#include <blif_solve_lib/approx_merge.h>

#include <memory>
#include <algorithm>
#include <cctype>
#include <sstream>





//Declarations for helper functions
namespace {

  double getBddSize(DdManager* manager, const dd::BddWrapper & b1, const dd::BddWrapper & b2);

  std::string printSupportSet(const dd::BddWrapper & bdd)
  {
    auto support = bdd.support(), one = bdd.one();
    std::stringstream out;
    out << "{";
    while (support != one)
    {
      auto next = support.varWithLowestIndex();
      out << " " << next.getIndex();
      support = support.cubeDiff(next);
      if (support != one) 
        out << ",";
    }
    out << " }";
    return out.str();
  }

} // end anonymous namespace








namespace var_score {

  using dd::BddWrapper;

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
  std::vector<BddWrapper>
    VarScoreQuantification::varScoreQuantification(const std::vector<BddWrapper> & F, 
        const BddWrapper & Q, 
        DdManager * ddm,
        const int maxBddSize,
        const ApproximationMethod::CPtr & approxImpl)
    {
      VarScoreQuantification vsq(F, Q, ddm);
      auto exactImpl = ApproximationMethod::createExact();
      while(!vsq.isFinished())
      {
        // vsq.printState();
        auto q1 = vsq.findVarWithOnlyOneFactor();
        if (q1)
        {
          blif_solve_log(DEBUG, "found var with only one factor");
          auto tv = vsq.neighboringFactors(*q1);
          assert(tv.size() == 1);
          auto t = *(tv.cbegin());
          auto t_without_q = t.existentialQuantification(*q1);
          vsq.removeFactor(t);
          vsq.removeVar(*q1);
          vsq.addFactor(t_without_q);
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





  VarScoreQuantification::VarScoreQuantification(const std::vector<BddWrapper> & F, const BddWrapper & Q, DdManager * ddm):
    m_factors(F.cbegin(), F.cend()),
    m_vars(),
    m_ddm(ddm)
  {
    BddWrapper qs = Q;
    BddWrapper one(bdd_one(ddm), ddm);
    while(qs != one)
    {
      // get next q
      BddWrapper q = qs.varWithLowestIndex();
      qs = qs.cubeDiff(q);
      // find neighbors of q
      for (auto f: m_factors)
        if (isNeighbor(f, q))
          m_vars[q].insert(f);
      if (m_vars[q].empty()) // no neighbors, you can ignore
        m_vars.erase(q);
    }
    blif_solve_log(DEBUG, "Created VarScoreQuantification with " << m_vars.size() << " vars and " << m_factors.size() << " factors");
  }







  std::optional<BddWrapper> VarScoreQuantification::findVarWithOnlyOneFactor() const
  {
    for(auto vxfs: m_vars)
      if (vxfs.second.size() == 1)
        return vxfs.first;
    return std::optional<BddWrapper>();
  }





  const std::set<BddWrapper>& VarScoreQuantification::neighboringFactors(const BddWrapper & var) const
  {
    auto qit = m_vars.find(var);
    assert(qit != m_vars.end());
    return qit->second;
  }







  void VarScoreQuantification::removeFactor(const BddWrapper & factor)
  {
    for (auto& vxfs: m_vars)
      vxfs.second.erase(factor);
    if (m_factors.count(factor) == 0)
      return;
    m_factors.erase(factor);
  }






  void VarScoreQuantification::addFactor(const BddWrapper & factor) 
  {
    if (m_factors.count(factor) > 0)
      return;
    m_factors.insert(factor);
    BddWrapper fsup = factor.support();
    BddWrapper one(bdd_one(m_ddm), m_ddm);
    for(auto& vxfs: m_vars)
    {
      BddWrapper q = vxfs.first;
      BddWrapper intersection = q.cubeIntersection(fsup);
      if (intersection != one)
        vxfs.second.insert(factor);
    }
  }





  void VarScoreQuantification::removeVar(const BddWrapper & var)
  {
    auto vit = m_vars.find(var);
    if (vit != m_vars.end())
      m_vars.erase(vit);
  }





  BddWrapper VarScoreQuantification::varWithLowestScore() const
  {
    std::optional<BddWrapper> minq;
    int lowestSize = 0;
    for (auto vxfs: m_vars)
    {
      int size = 0;
      for (auto f: vxfs.second)
        size += bdd_size(f.getUncountedBdd());
      if (!minq || size < lowestSize)
      {
        minq = vxfs.first;
        lowestSize = size;
      }
    }
    assert(minq.has_value());
    return *minq;
  }





  std::pair<BddWrapper, BddWrapper> VarScoreQuantification::smallestTwoNeighbors(const BddWrapper & var) const
  {
    auto qit = m_vars.find(var);
    assert(qit != m_vars.end());
    assert(qit->second.size() >= 2);
    std::optional<BddWrapper> f1, f2;
    int s1 = 0, s2 = 0;
    for (auto fit: qit->second)
    {
      std::optional<BddWrapper> f(fit);
      int s = bdd_size(f->getUncountedBdd());
      if (!f1.has_value() || s < s1)
      {
        std::swap(f1, f);
        std::swap(s1, s);
      }
      if (f.has_value() && (!f2.has_value() || s < s2))
      {
        std::swap(f2, f);
        std::swap(s2, s);
      }
    }
    assert(f1.has_value() && f2.has_value());
    return std::make_pair(*f1, *f2);
  }





  bool VarScoreQuantification::isFinished() const {
    for (const auto & vxfs: m_vars)
      if (!vxfs.second.empty())
        return false;
    return true;
  }






  std::vector<BddWrapper> VarScoreQuantification::getFactorCopies() const
  {
    return std::vector<BddWrapper>(m_factors.cbegin(), m_factors.cend());
  }




  bool VarScoreQuantification::isNeighbor(const BddWrapper & f, const BddWrapper & g) const
  {
    auto fsup = f.support();
    auto gsup = g.support();
    auto common = fsup.cubeIntersection(gsup);
    bool isNotNeighbor = bdd_is_one(m_ddm, common.getUncountedBdd());
    return ! isNotNeighbor;
  }





  void VarScoreQuantification::printState() const
  {
    std::cout << "\n======\nFactors:\n";
    for (const auto & f: m_factors)
    {
      std::cout << f.getUncountedBdd() << " " << printSupportSet(f) << "\n";
    }

    std::cout << "\nVariables:\n" << std::endl;
    for (const auto & vxfs: m_vars)
    {
      std::cout << "var: " << vxfs.first.getUncountedBdd() 
                << " " << printSupportSet(vxfs.first)
                << "\nfuncs:\n";
      for (const auto & f: vxfs.second)
      {
        std::cout << "    " << f.getUncountedBdd() 
                  << " " << printSupportSet(f) 
                  << "\n";
      }
    }
  }




} // end namespace var_score





// definitions for helper functions
namespace {

  using dd::BddWrapper;

  /* Heuristic for predicting the bdd size
   * If no variables are common, then size is nb1 + nb2.
   * If all variables are common then size is nb1 * nb2.
   * Is it a bad heuristic? I don't know. You tell me.
   */
  double getBddSize(DdManager * manager, const BddWrapper & b1, const BddWrapper & b2)
  {
    // support sets
    BddWrapper sp1 = b1.support();
    BddWrapper sp2 = b2.support();
    // intersection support set
    BddWrapper intsn = sp1.cubeIntersection(sp2);

    double nb1 = bdd_size(b1.getUncountedBdd());
    double nb2 = bdd_size(b2.getUncountedBdd());
    double nsp1 = bdd_size(sp1.getUncountedBdd()) - 1;
    double nsp2 = bdd_size(sp2.getUncountedBdd()) - 1;
    double nintsn = bdd_size(intsn.getUncountedBdd()) - 1;

    return 
      nb1 * (nsp1 - nintsn) / nsp1                  // vars unique to b1
      + nb2 * (nsp2 - nintsn) / nsp2                // vars unique to b2
      + nb1 * nb2 * nintsn * nintsn / nsp1 / nsp2;  // vars common to b1 and b2

  }



} // end anonymous namespace
