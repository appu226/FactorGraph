/*

Copyright 2025 Parakram Majumdar

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

#include "approx_var_elim.h"
#include <dd/max_heap.h>

#include <algorithm>
#include <stdexcept>
#include <iostream>

// #define AVEDBG(x) std::cout << x << std::endl;
#define AVEDBG(x)

namespace oct_22
{

  AveClause::AveClause(AveIntVec v_literals)
    : literals(std::move(v_literals)), isAcceptable(false), numFlippedLiterals(0), hash(0)
  {
    std::sort(literals.begin(), literals.end());
    auto last = std::unique(literals.begin(), literals.end());
    literals.erase(last, literals.end());
    for (auto const v: literals)
    {
      hash ^= static_cast<size_t>(v) + 0x9e3b79b9 + (hash<<6) + (hash>>2);
    }
  }

  bool AveClause::operator==(const AveClause& that) const
  {
    if (hash != that.hash)
    {
      return false;
    }
    return this->literals == that.literals;
  }
  bool AveClause::operator!=(const AveClause& that) const
  {
    if (hash != that.hash)
    {
      return true;
    }
    return this->literals != that.literals;
  }
  bool AveClause::operator<(const AveClause& that) const
  {
    if (hash < that.hash)
    {
      return true;
    }
    else if (hash > that.hash)
    {
      return false;
    }
    return this->literals < that.literals;
  }
  bool AveClause::operator>(const AveClause& that) const
  {
    if (hash > that.hash)
    {
      return true;
    }
    else if (hash < that.hash)
    {
      return false;
    }
    return this->literals > that.literals;
  }
  bool AveClause::operator<=(const AveClause& that) const
  {
    return !(*this > that);
  }
  bool AveClause::operator>=(const AveClause& that) const
  {
    return !(*this < that);
  }

  ApproxVarElim::Ptr ApproxVarElim::parseQdimacs(dd::Qdimacs const& qdimacs)
  {
    Ptr result = std::make_shared<ApproxVarElim>();
    result->m_positiveLiterals.reserve(qdimacs.numVariables);
    result->m_negativeLiterals.reserve(qdimacs.numVariables);
    for (int i = 1; i <= qdimacs.numVariables; ++i)
    {
      result->m_positiveLiterals.emplace_back(new AveLiteral(i));
      result->m_negativeLiterals.emplace_back(new AveLiteral(-i));
    }
    for (auto const& inClause: qdimacs.clauses)
    {
      auto outClause = std::make_shared<AveClause>(inClause);
      result->addClause(outClause);
    }
    if (qdimacs.quantifiers.empty())
    {
      throw std::runtime_error("At least one quantifier is required");
    }
    if (qdimacs.quantifiers.back().quantifierType != dd::Quantifier::Exists)
    {
      throw std::runtime_error("Innermost quantifier must be an existential quantifier");
    }
    result->m_varsToEliminate = qdimacs.quantifiers.back().variables;
    return result;
  }


  void ApproxVarElim::addClause(const AveClausePtr& clause)
  {
    if (m_clauses.count(clause) > 0)
        return;
    m_clauses.insert(clause);
    for (auto const& literal: clause->literals)
    {
      auto lit = getLiteral(literal);
      lit->clauses.insert(clause);
    }
  }

  void ApproxVarElim::removeClause(const AveClausePtr& clause)
  {
    for (auto const& literal: clause->literals)
    {
      getLiteral(literal)->clauses.erase(clause);
    }
    m_clauses.erase(clause);
  }

  void ApproxVarElim::approximatelyEliminateVar(int x, std::unordered_set<int> const& allVarsToElim, size_t numClauseIncPerVarElim)
  {
    // let XP be clauses with x and XN be clauses with -x
    // create a min_heap MH of capacity |XP| + |XN| + NUM_CLAUSE_INC_PER_VAR_ELIM (min element gets pushed out on overflow)
    // for each clause xp in XP
    //     mark all clauses in XN as acceptable
    //     for each literal L in xp s.t. L != x
    //         mark all clauses that have -L as unacceptable
    //     for each clause xn in XN that is still acceptable
    //         create new clause pn = (xp \ +x) V (xn \ -x)
    //         // count clauses with exactly one flipped literal on a quantified variable
    //         for all literals L in pn // count flipped literals on all clauses
    //             for all clauses C with -L
    //                 C.num_flipped_literals += 1
    //         num_pairings_for_pn = 0
    //         for all literals L in pn such that |L| is a quantified variable
    //             for all clauses C with -L
    //                 if C.num_flipped_literals == 1
    //                     num_pairings_for_pn += 1
    //         for all literals L in pn // reset the counts to zero
    //             for all clauses C with -L
    //                 C.num_flipped_literals = 0
    //         insert pn to MH with weight num_pairings_for_pn
    // remove all clauses from XP and XN, and add clauses from MH
    
    AVEDBG("Eliminating variable " << x);
    AveClausePtrSet XP = getClausesWithLiteral(x);
    AveClausePtrSet XN = getClausesWithLiteral(-x);
    AveClausePtrSet newlyCreatedClauses;
    AVEDBG("XP size: " << XP.size());
    AVEDBG("XN size: " << XN.size());
    size_t discarded_clauses = 0;
    
    // create a min_heap MH of capacity |XP| + |XN| + NUM_CLAUSE_INC_PER_VAR_ELIM (min element gets pushed out on overflow)
    parakram::MaxHeap<AveClausePtr, size_t, std::greater<size_t> > MH;
    size_t const maxHeapMaxSize = XP.size() + XN.size() + numClauseIncPerVarElim;
    
    // for each clause xp in XP
    for (auto const& xp: XP)
    {
    // mark all clauses in XN as acceptable
      for (auto const& xn: XN)
        xn->isAcceptable = true;

    // for each literal L in xp s.t. L != x
      for (auto const& L: xp->literals)
        if (L != x)
          // mark all clauses that have -L as unacceptable
          for (auto const& c_negl: getLiteral(-L)->clauses)
            c_negl->isAcceptable = false;

      // for each clause xn in XN that is still acceptable
      for (auto const& xn: XN)
      {
        if (xn->isAcceptable)
        {
          xn->isAcceptable = false; // reset the flag
          // create new clause pn = (xp \ +x) V (xn \ -x)
          AveIntVec pnLiterals;
          pnLiterals.reserve(xp->literals.size() + xn->literals.size() - 2);
          for (auto const& L: xp->literals)
            if (L != x)
              pnLiterals.push_back(L);
          for (auto const& L: xn->literals)
            if (L != -x)
              pnLiterals.push_back(L);
          auto pn = std::make_shared<AveClause>(pnLiterals);
          if (newlyCreatedClauses.count(pn) > 0)
            continue;
          newlyCreatedClauses.insert(pn);

          // for all literals L in pn
          for (auto const& L: pn->literals)
            // for all clauses C with -L
            for (const auto& C: getLiteral(-L)->clauses)
              // C.num_flipped_literals += 1
              C->numFlippedLiterals += 1;
          
          // num_pairings_for_pn = 0
          size_t numPairingsForPn = 0;

          // for all literals L in pn such that |L| is a quantified variable
          for (auto const& L: pn->literals)
          {
            if (allVarsToElim.count(std::abs(L)) == 0)
              continue;
            // for all clauses C with -L
            for (const auto& C: getLiteral(-L)->clauses)
            {
              // if C.num_flipped_literals == 1
              if (C->numFlippedLiterals == 1)
                // num_pairings_for_pn += 1
                numPairingsForPn += 1;
            }
          }
          // for all literals L in pn // reset the counts to zero
          for (auto const& L: pn->literals)
            // for all clauses C with -L
            for (const auto& C: getLiteral(-L)->clauses)
              // C.num_flipped_literals = 0
              C->numFlippedLiterals = 0;
          // insert pn to MH with weight num_pairings_for_pn
          MH.insert(pn, numPairingsForPn);
          if (MH.size() > maxHeapMaxSize)
          {
            MH.pop();
            ++discarded_clauses;
          }
        }
      }
    }
    // remove all clauses from XP and XN, and add clauses from MH
    for (auto const& xp: XP)
      removeClause(xp);
    for (auto const& xn: XN)
      removeClause(xn);
    AVEDBG("MH size: " << MH.size());
    AVEDBG("Discarded clauses: " << discarded_clauses);
    while(MH.size() > 0)
    {
      auto clause = MH.top();
      MH.pop();
      addClause(clause);
    }
  }

  AveLiteralPtr const& ApproxVarElim::getLiteral(int literal) const
  {
    if (literal == 0)
    {
      throw std::invalid_argument("Literal cannot be zero");
    }
    else if (literal > 0)
    {
      if (literal > static_cast<int>(m_positiveLiterals.size()))
      {
        throw std::out_of_range("Literal out of range");
      }
      else {
        return m_positiveLiterals[literal - 1];
      }
    }
    else
    {
      if (-literal > static_cast<int>(m_negativeLiterals.size()))
      {
        throw std::out_of_range("Literal out of range");
      }
      {
        return m_negativeLiterals[-literal - 1];
      }
    }
  }

  AveClausePtrSet const& ApproxVarElim::getClausesWithLiteral(int literal) const
  {
    return getLiteral(literal)->clauses;
  }

  AveClausePtrSet const& ApproxVarElim::getClauses() const
  {
    return m_clauses;
  }

  void ApproxVarElim::approximatelyEliminateAllVariables(size_t numClaseIncPerVarElim)
  {
    // sort quantified variables on number of non-trivial pairings
    std::unordered_map<int, size_t> nonTrivialPairCount;
    for (auto const& varToElim: m_varsToEliminate)
    {
      nonTrivialPairCount[varToElim] = getNonTrivialPairCount(varToElim);
    }
    std::sort(m_varsToEliminate.begin(), m_varsToEliminate.end(),
              [&nonTrivialPairCount](int a, int b) { return nonTrivialPairCount[a] < nonTrivialPairCount[b]; });
    // eliminate variables in order
    std::unordered_set<int> allVarsToElim(m_varsToEliminate.cbegin(), m_varsToEliminate.cend());
    for (auto const& varToElim: m_varsToEliminate)
    {
      approximatelyEliminateVar(varToElim, allVarsToElim, numClaseIncPerVarElim);
      AVEDBG("remaining clauses: " << m_clauses.size());
    }
  }

  size_t ApproxVarElim::getNonTrivialPairCount(int x) const
  {
    // for each clause xp in XP
    //     mark all clauses in XN as acceptable
    //     for each literal L in xp s.t. L != x
    //          mark all clauses that have -L as unacceptable
    //     non_trivial_pairing_count[x] += number of clauses in XN that are still acceptable
    // mark all clauses in XN as unacceptable
    size_t count = 0;
    for (auto const& xp: getClausesWithLiteral(x))
    {
      for (auto const& xn: getClausesWithLiteral(-x))
      {
        xn->isAcceptable = true;
      }

      for (auto const& L: xp->literals)
      {
        if (L != x)
        {
          for (auto const& c_negl: getLiteral(-L)->clauses)
          {
            c_negl->isAcceptable = false;
          }
        }
      }

      for (auto const& xn: getClausesWithLiteral(-x))
      {
        if (xn->isAcceptable)
        {
          ++count;
          xn->isAcceptable = false;
        }
      }
    }
    return count;
  }



} // end namespace oct_22