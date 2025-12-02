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
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>

// #define AVEDBG(x) std::cout << x << std::endl;
#define AVEDBG(x)

namespace oct_22
{

  AveClause::AveClause(AveIntVec v_literals)
    : literals(std::move(v_literals)),
    numFlippedQuantifiedLiterals(0),
    numFlippedNonQuantifiedLiterals(0),
    hash(0),
    resolvableList(),
    resolvableListIter(),
    isEnabled(true)
  {
    std::sort(literals.begin(), literals.end());
    auto last = std::unique(literals.begin(), literals.end());
    literals.erase(last, literals.end());
    for (auto const v: literals)
    {
      hash ^= static_cast<size_t>(v) + 0x9e3b79b9 + (hash<<6) + (hash>>2);
    }
  }

  AveSeedModification::AveSeedModification(
    AveIntVec const& oldSeed,
    AveIntVec const& clause, 
    AveIntVec const& quantifiedVariables,
    AveClausePtr& newSeed)
  {
    // create a new seed clause
    newSeed = std::make_shared<AveClause>(AveIntVec{});
    auto& newSeedLiterals = newSeed->literals;
    newSeedLiterals.reserve(oldSeed.size() + clause.size());

    // check for flipped quantified literal
    pivotSeedLiteral = 0;
    {
      // iterators to search through oldSeed, clause and quantifiedVariables
      auto osit = oldSeed.cbegin(), osend = oldSeed.cend();
      auto ncit = clause.crbegin(), cend = clause.crend(); 
      while (osit != osend && ncit != cend)
      {
        auto oldSeedLit = *osit;
        auto negatedClauseLit = -*ncit;
        if (oldSeedLit < negatedClauseLit)
        {
          ++osit;
        }
        else if (negatedClauseLit < oldSeedLit)
        {
          ++ncit;
        }
        else
        {
          // found a negated literal
          if (pivotSeedLiteral != 0 && pivotSeedLiteral != oldSeedLit)
          {
            throw std::runtime_error("Multiple flipped literals found in seed and clause.");
          }
          pivotSeedLiteral = oldSeedLit;
          quantifiedLiteralsToRemove.push_back(oldSeedLit);
          break;
        }
      }
    }

    // populate the new seed literals
    {
      // iterators to search through oldSeed, clause and quantifiedVariables
      auto osit = oldSeed.begin(), osend = oldSeed.end();
      auto cit = clause.begin(), cend = clause.end(); 
      auto qvit = quantifiedVariables.begin(), qvend = quantifiedVariables.end();
      auto nqvit = quantifiedVariables.rbegin(), nqvend = quantifiedVariables.rend();
      while (osit != osend || cit != cend)
      {
        int next_literal = 0;
        bool is_newly_added = false;
        if (cit == cend || (osit != osend && *osit < *cit))
        {
          // add old seed literal
          next_literal = *osit;
          ++osit;
        }
        else if (osit == osend || (cit != cend && *cit < *osit))
        {
          // add clause literal
          next_literal = *cit;
          is_newly_added = true;
          ++cit;
        }
        else
        {
          // both are equal, add one of them
          next_literal = *osit;
          ++osit;
          ++cit;
        }
        if (std::abs(next_literal) == std::abs(pivotSeedLiteral))
        {
          // this is the flipped literal, skip it
          continue;
        }
        else
        {
          bool is_quantified = false;
          if (next_literal > 0)
          {
            // positive literal
            while (qvit != qvend && *qvit < next_literal)
            {
              ++qvit;
            }
            if (qvit != qvend && *qvit == next_literal)
            {
              is_quantified = true;
              ++qvit;
            }
          }
          else
          {
            // negative literal
            while (nqvit != nqvend && -*nqvit < next_literal)
            {
              ++nqvit;
            }
            if (nqvit != nqvend && -*nqvit == next_literal)
            {
              is_quantified = true;
              ++nqvit;
            }
          }
          
          if (is_quantified)
          {
            if (is_newly_added)
            {
              quantifiedLiteralsToAdd.push_back(next_literal);
            }
          }
          else
          {
            if (is_newly_added)
            {
              nonQuantifiedLiteralsToAdd.push_back(next_literal);
            }
          }
          newSeedLiterals.push_back(next_literal);
        }
      }
    } 
  }

  ApproxVarElim::Ptr ApproxVarElim::parseQdimacs(dd::Qdimacs const& qdimacs)
  {
    Ptr result = std::make_shared<ApproxVarElim>();
    result->m_positiveLiterals.reserve(qdimacs.numVariables);
    result->m_negativeLiterals.reserve(qdimacs.numVariables);
    result->m_isLiteralUsedAsPivot = std::make_shared<AveLiteralBoolMap>(qdimacs.numVariables);
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
    std::sort(result->m_varsToEliminate.begin(), result->m_varsToEliminate.end());
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
      lit->clauses.push_back(clause);
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

  AveClausePtrVec const& ApproxVarElim::getClausesWithLiteral(int literal) const
  {
    return getLiteral(literal)->clauses;
  }

  AveClausePtrSet const& ApproxVarElim::getResultClauses() const
  {
    return m_resultClauses;
  }

  void ApproxVarElim::approximatelyEliminateAllVariables(size_t maxClauseTreeSize, size_t numMaxSeconds)
  {
    // deadline flag (false -> not expired). Use a shared_ptr so the
    // detached timer thread can safely outlive this stack frame without
    // creating a dangling reference.
    auto hasExpired = std::make_shared<std::atomic<bool>>(false);

    // if a max seconds is specified, spawn a thread that waits and sets the flag
    if (numMaxSeconds != 0)
    {
      std::shared_ptr<std::atomic<bool>> timerFlag = hasExpired; // copy for thread
      std::thread([numMaxSeconds, timerFlag]() {
        std::this_thread::sleep_for(std::chrono::seconds(static_cast<long>(numMaxSeconds)));
        // mark as expired
        timerFlag->store(true, std::memory_order_release);
      }).detach();
    }
    // results = [c for c in input_clauses if not c.has_any(vars_to_elim)]
    // filtered_inputs = [c for c in input_clauses if c.has_any(vars_to_elim)]
    std::vector<AveClausePtr> terminalClauses;
    AveClauseListPtr resolvableClauses = std::make_shared<AveClauseList>();
    std::vector<AveClauseListNodePtr> iterStore;
    auto const& vte = m_varsToEliminate;
    for (auto const& clause: m_clauses)
    {
      // check deadline each iteration
      if (hasExpired->load(std::memory_order_acquire))
      {
        // already expired -> stop processing
        break;
      }
      auto varsToEliminate = intersection(clause->literals, vte);
      if (varsToEliminate.empty())
      {
        m_resultClauses.insert(clause);
      }
      else
      {
        iterStore.push_back(resolvableClauses->push_back(clause));
        resolvableClauses->pop_back();
        clause->resolvableListIter = iterStore.back();
        clause->resolvableList = resolvableClauses;
        if (varsToEliminate.size() == 1)
        {
          terminalClauses.push_back(clause);
        }
      }
    }


    // for c in filtered_inputs:
    //     elim_helper(c, filtered_inputs \ {c}, vars_to_elim, results, 0, maxClauseTreeSize)
    //     filtered_inputs = filtered_inputs \ {c}   # remove c, as we have explored all results with c
    for (auto const& terminalClause: terminalClauses)
    {
      // check deadline each iteration
      if (hasExpired->load(std::memory_order_acquire))
      {
        // already expired -> stop processing
        break;
      }
      // set clause as seed
      AveClausePtr newSeed;
      AveSeedModification seedModification({}, terminalClause->literals, m_varsToEliminate, newSeed);
      applySeedModification(seedModification);

      // recurse and grow the seed
      elimHelper(newSeed, *resolvableClauses, maxClauseTreeSize, hasExpired);

      // reset the seed
      seedModification.flip();
      applySeedModification(seedModification);

      // never look at this clause again
      terminalClause->isEnabled = false;

    }
  }


  // recursive helper function
  // to grow a seed clause
  // as we try to eliminate vars
  void ApproxVarElim::elimHelper(
    AveClausePtr const& resultSeed,
    AveClauseList& resolvableClauses,
    size_t maxClauseTreeSize,
    std::shared_ptr<std::atomic<bool>> const& hasExpired
  )
  {
    // # find literals that still need to be eliminated
    auto resultLiteralsToElim = intersection(resultSeed->literals, m_varsToEliminate);

    // # check if seed is already good enough
    // # resultSeed should not be empty
    // # and resultLiteralsToElim should be empty
    if (!resultSeed->literals.empty()
        && resultLiteralsToElim.empty())
    {
      // std::cout << "Adding result seed: ";
      // for (auto const& lit: resultSeed->literals)
      // {
      //   std::cout << lit << " ";
      // }
      // std::cout << std::endl;
      m_resultClauses.insert(resultSeed);
      return;
    }

    if (0 == maxClauseTreeSize)
    return;

    // check for deadline before growing seed
    if (hasExpired->load(std::memory_order_acquire))
    {
      return;
    }

    std::vector<AveClausePtr> resolvableClausesVec;
    for(auto cit = resolvableClauses.begin(); cit != resolvableClauses.end(); cit = cit->next)
    {
      resolvableClausesVec.push_back(cit->value);
    }

    // # check each input clause to see if it can be used to grow the seed
    for (auto const& c: resolvableClausesVec)
    {
      // per-iteration deadline check
      if (hasExpired->load(std::memory_order_acquire))
      {
        break;
      }
      // # check if c can be used to grow seed -> there should be exactly one negated literal, 
      // # and it should be in varsToEliminate
      if (!c->isResolvable())
      {
        throw std::runtime_error("Clause is not resolvable, but should be.");
      }
      if  (!c->isEnabled)
      {
        continue;
      }
      // # grow the seed
      AveClausePtr newSeed;
      AveSeedModification seedModification(resultSeed->literals, c->literals, m_varsToEliminate, newSeed);
      if (m_isLiteralUsedAsPivot->get(seedModification.pivotSeedLiteral))
      {
        continue;
      }

      applySeedModification(seedModification);
      m_isLiteralUsedAsPivot->set(seedModification.pivotSeedLiteral, true);
      
      // # recursive step
      elimHelper(
        newSeed,
        resolvableClauses,
        maxClauseTreeSize - 1,
        hasExpired
      );

      // reset the seed
      m_isLiteralUsedAsPivot->set(seedModification.pivotSeedLiteral, false);
      seedModification.flip();
      applySeedModification(seedModification);
    }
               
  }

  AveClausePtr ApproxVarElim::resolve(
    AveIntVec const& c1,
    AveIntVec const& c2,
    int pivot
  )
  {
    // create a new clause that is the result of resolving c1 and c2 on pivot
    AveIntVec result;
    if (c1.empty() && c2.empty())
        return std::make_shared<AveClause>(std::move(result));
    result.reserve(c1.size() + c2.size() - 1); // -1 because we will remove the pivot

    auto it1 = c1.cbegin();
    auto it2 = c2.cbegin();

    while (it1 != c1.cend() || it2 != c2.cend())
    {
      if (it2 == c2.cend() || (it1 != c1.cend() && *it1 < *it2))
      {
        if (std::abs(*it1) != std::abs(pivot))
        {
          result.push_back(*it1);
        }
        ++it1;
      }
      else if (it1 == c1.cend() || (it2 != c2.cend() && *it2 < *it1))
      {
        if (std::abs(*it2) != std::abs(pivot))
        {
          result.push_back(*it2);
        }
        ++it2;
      }
      else
      {

        if (std::abs(*it1) != std::abs(pivot))
        {
          // both are equal, but not the pivot
          result.push_back(*it1);
        }
        ++it1;
        ++it2;
      }
    }

    return std::make_shared<AveClause>(std::move(result));
  }

  AveIntVec ApproxVarElim::negatedLiterals(
    AveIntVec const& clauseLiterals,
    AveIntVec const& seedLiterals
  )
  {
    // ensure non-empty
    if (clauseLiterals.empty() || seedLiterals.empty())
      return {};

    // reserve space for result
    AveIntVec result;
    result.reserve(std::min(clauseLiterals.size(), seedLiterals.size()));

    auto cit = clauseLiterals.cbegin();
    auto sit = seedLiterals.crbegin();
    while (cit < clauseLiterals.cend() && sit != seedLiterals.crend())
    {
      int cl = *cit;
      int sl = -1 * *sit;
      if (cl < sl)
      {
        ++cit;
      }
      else if (sl < cl)
      {
        ++sit;
      }
      else
      {
        // found a negated literal
        result.push_back(cl);
        ++cit;
        ++sit;
      }
    }

    return std::move(result);
  }

  AveIntVec ApproxVarElim::intersection(
    AveIntVec const& literals,
    AveIntVec const& variables
  )
  {
    // ensure non-empty
    if (literals.empty() || variables.empty())
      return {};
    
    // reserve space for result
    AveIntVec result;
    result.reserve(std::min(literals.size(), variables.size()));

    // segregate negative and positive literals
    auto firstPositiveLiteralIt = literals.cbegin();
    while (firstPositiveLiteralIt < literals.cend() && *firstPositiveLiteralIt < 0)
      ++firstPositiveLiteralIt;

    // search for variables for neg literal from back to front
    auto reverseSearchIt = variables.crbegin();
    for (auto negLitSearchIt = literals.cbegin(); negLitSearchIt < firstPositiveLiteralIt && reverseSearchIt != variables.crend();)
    {
      int negLitVar = std::abs(*negLitSearchIt);
      if (negLitVar < *reverseSearchIt)
      {
        ++reverseSearchIt;
      }
      else if (*reverseSearchIt < negLitVar)
      {
        ++negLitSearchIt;
      }
      else
      {
        result.push_back(*negLitSearchIt);
        ++negLitSearchIt;
        ++reverseSearchIt;
      }
    }


    // search for variables for positive literals from front to back
    auto forwardSearchIt = variables.cbegin();
    for (auto posLitSearchIt = firstPositiveLiteralIt; posLitSearchIt < literals.cend() && forwardSearchIt != variables.cend();)
    {
      if (*posLitSearchIt < *forwardSearchIt)
      {
        ++posLitSearchIt;
      }
      else if (*forwardSearchIt < *posLitSearchIt)
      {
        ++forwardSearchIt;
      }
      else
      {
        result.push_back(*posLitSearchIt);
        ++posLitSearchIt;
        ++forwardSearchIt;
      }
    }

    return std::move(result);
  }

  void ApproxVarElim::applySeedModification(
    AveSeedModification const& seedModification
  )
  {
    using ClauseOperation = std::function<void(AveClause&)>;
    auto applyOperation = [this](AveIntVec const& flippedLiterals, ClauseOperation operation)-> void
    {
      for (auto const& lit: flippedLiterals)
      {
        auto& clauses = getClausesWithLiteral(-lit);
        for (auto& clause: clauses)
        {
          clause->update(operation);
        }
      }
    };

    applyOperation(
      seedModification.quantifiedLiteralsToAdd,
      [](AveClause& clause) -> void { ++clause.numFlippedQuantifiedLiterals; }
    );
    applyOperation(
      seedModification.nonQuantifiedLiteralsToAdd,
      [](AveClause& clause) -> void { ++clause.numFlippedNonQuantifiedLiterals; }
    );
    applyOperation(
      seedModification.quantifiedLiteralsToRemove,
      [](AveClause& clause) -> void { --clause.numFlippedQuantifiedLiterals; }
    );
    applyOperation(
      seedModification.nonQuantifiedLiteralsToRemove,
      [](AveClause& clause) -> void { --clause.numFlippedNonQuantifiedLiterals; }
    );
  }



} // end namespace oct_22