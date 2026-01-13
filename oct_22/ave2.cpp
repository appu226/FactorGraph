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



#include "ave2.h"

#include <algorithm>
#include <stdexcept>


namespace {

  using namespace oct_22;

  // Struct to try level 2 resolution,
  //   i.e., use a previously used resolver clause
  //   to resolve re-introduced literals.
  // To be used only if no clause can be resolved 
  //   without reintroducing eliminated literals
  struct Level2ResolutionAttempt {
    Ave2ClauseCPtr resolvingClause;
    Ave2ClauseCPtr resolvent;
    Ave2ClauseCPtr reintroducedLiteralsToEliminate;
    Ave2ClauseCPtr newEliminatedLiterals;
  };


  // struct to backtrack insertion into literalToResolverMap
  struct LiteralToResolverMapInsertionBackTracker {
    std::unordered_map<int, Ave2ClauseCPtr>& literalToResolverMap;
    int insertedLiteral;

    LiteralToResolverMapInsertionBackTracker(
        std::unordered_map<int, Ave2ClauseCPtr>& v_literalToResolverMap,
        int v_insertedLiteral,
        Ave2ClauseCPtr const& v_resolver)
      : literalToResolverMap(v_literalToResolverMap),
        insertedLiteral(v_insertedLiteral)
    {
        v_literalToResolverMap.insert({ insertedLiteral, v_resolver });
    }

    ~LiteralToResolverMapInsertionBackTracker() {
      literalToResolverMap.erase(insertedLiteral);
    }
  };


} // end anonymous namespace


namespace oct_22
{





  Ave2Clause::Ave2Clause(std::vector<int> v_literals, bool sortAndUnique)
    : literals(std::move(v_literals))
  {

    if (sortAndUnique)
    {
      std::sort(literals.begin(), literals.end());
      auto last = std::unique(literals.begin(), literals.end());
      literals.erase(last, literals.end());
    }
    hash = 0;
    for (auto const v: literals)
    {
        hash ^= static_cast<size_t>(v) + 0x9e3b79b9 + (hash<<6) + (hash>>2);
    }
  }


  void Ave2ClauseMap::insert(Ave2ClauseCPtr const& clause)
  {
    for (auto literal: clause->literals)
    {
      auto it = clauseMap.find(literal);
      if (it == clauseMap.end())
      {
        Ave2ClauseVec clauseVec = std::make_shared<std::vector<Ave2ClauseCPtr> >();
        clauseVec->push_back(clause);
        clauseMap.insert({ literal, clauseVec });
      }
      else
      {
        it->second->push_back(clause);
      }
    }
  }






  Ave2ClauseCPtr Ave2Clause::intersect(const Ave2Clause& that) const
  {
    std::vector<int> resultLiterals;
    auto it1 = this->literals.cbegin();
    auto it2 = that.literals.cbegin();
    while (it1 != this->literals.cend() && it2 != that.literals.cend())
    {
      if (*it1 < *it2)
      {
        ++it1;
      }
      else if (*it2 < *it1)
      {
        ++it2;
      }
      else
      {
        resultLiterals.push_back(*it1);
        ++it1;
        ++it2;
      }
    }
    return std::make_shared<Ave2Clause>(resultLiterals, false);
  }







  Ave2::Ptr Ave2::parseQdimacs(const dd::Qdimacs& qdimacs)
  {
    Ave2::Ptr result = std::make_shared<Ave2>();
    result->m_clauses = std::make_shared<std::vector<Ave2ClauseCPtr> >();
    for (auto const& inClause: qdimacs.clauses)
    {
      auto outClause = std::make_shared<Ave2Clause>(inClause);
      result->m_clauses->push_back(outClause);
      result->m_literalToClause.insert(outClause);
    }
    if (qdimacs.quantifiers.empty())
    {
      throw std::runtime_error("At least one quantifier is required");
    }
    if (qdimacs.quantifiers.back().quantifierType != dd::Quantifier::Exists)
    {
      throw std::runtime_error("Innermost quantifier must be an existential quantifier");
    }
    result->m_varsToEliminate = std::make_shared<Ave2Clause>(qdimacs.quantifiers.back().variables);
    std::vector<int> allLiteralsToEliminateVec;
    allLiteralsToEliminateVec.reserve(result->m_varsToEliminate->literals.size() * 2);
    for (int i = result->m_varsToEliminate->literals.size() - 1; i >= 0; --i)
      allLiteralsToEliminateVec.push_back(-result->m_varsToEliminate->literals[i]);
    for (auto v: result->m_varsToEliminate->literals)
      allLiteralsToEliminateVec.push_back(v);
    result->m_literalsToEliminate = std::make_shared<Ave2Clause>(allLiteralsToEliminateVec);
    return result;
  }





  // main entry point
  Ave2ClauseSet Ave2::approximatelyEliminateAllVariables(size_t searchDepth)
  {
    // convert clauses to set and filter out clauses with no vars to eliminate
    Ave2ClauseSet result = std::make_shared<std::unordered_set<Ave2Clause, Ave2ClauseHash, std::equal_to<Ave2Clause>, std::allocator<Ave2Clause> > >();
    Ave2ClauseVec tempClauses = m_clauses;
    auto filteredClauses = filterOutClausesWithNoVarsToEliminate(tempClauses, *m_literalsToEliminate);
    for (const auto& clause : *filteredClauses)
    {
      result->insert(*clause);
    }

    // a collection to keep track of already eliminated literals
    Ave2Clause alreadyEliminatedLilterals(std::vector<int>{});

    // a map that remembers which clause which was used to resolve a given literal  
    std::unordered_map<int, Ave2ClauseCPtr> literalToResolverMap;

    // recursive algorithm, starting with each clause as seed
    for (auto const& seed: *m_clauses)
    {
      growSeed(seed, result, searchDepth, alreadyEliminatedLilterals, literalToResolverMap);
    }
    return result;
  }

  // recursive function to grow a seed clause
  void Ave2::growSeed(
      Ave2ClauseCPtr const& seed,
      Ave2ClauseSet& resultClauses,
      size_t searchDepth,
      Ave2Clause const& alreadyEliminatedLiterals,
      std::unordered_map<int, Ave2ClauseCPtr>& literalToResolverMap)
  {
    // seed literals to eliminate
    auto seedLte = seed->intersect(*m_literalsToEliminate);
    if (seedLte->literals.empty())
    {
      // all literals eliminated
      resultClauses->insert(*seed);
      return;
    }

    // try to eliminate each literal to be eliminate
    for (int L: seedLte->literals)
    {
      // objects to remember whether we found a simple resolvent
      //   or need to try level 2 resolution
      bool foundSimpleResolvant = false;
      std::vector<Level2ResolutionAttempt> level2Attempts;

      // find all clauses with -L
      auto it = m_literalToClause.clauseMap.find(-L);
      if (it != m_literalToClause.clauseMap.end())
      {

        // required for recursion
        auto newEliminatedLiterals = 
          alreadyEliminatedLiterals.concatenate(Ave2Clause(std::vector<int>{L}));

        auto const& clausesWithNegL = it->second;
        for (auto const& c: *clausesWithNegL)
        {
          // check if resolution was possible
          auto optionalResolvent = seed->resolveOnVar(abs(L), *c);
          if (optionalResolvent.has_value())
          {
            auto resolvent = optionalResolvent.value();
            // make sure no literals to eliminate are re-introduced
            auto reintroducedLiteralsToEliminate = resolvent->intersect(alreadyEliminatedLiterals);
            if (!reintroducedLiteralsToEliminate->literals.empty())
            {
              level2Attempts.push_back({c, resolvent, reintroducedLiteralsToEliminate, newEliminatedLiterals});
              continue;
            }

            // simple resolvent found, let's recurse
            foundSimpleResolvant = true;
            LiteralToResolverMapInsertionBackTracker backTracker(literalToResolverMap, L, c);
            growSeed(resolvent, resultClauses, searchDepth - 1, *newEliminatedLiterals, literalToResolverMap);
          }
        }
      }


      if (!foundSimpleResolvant)
      {
        // no simple resolvent found, try level 2 resolution
        for (auto const& l2a: level2Attempts)
        {
          bool allReintroducedLiteralsCanBeReRemoved = true;
          auto newResolvingClause = l2a.resolvingClause;
          auto newResolvent = l2a.resolvent;

          // try to resolve all re-introduced literals
          for (auto reintroducedLiteral: l2a.reintroducedLiteralsToEliminate->literals)
          {
            if (!allReintroducedLiteralsCanBeReRemoved)
              break;
            if (literalToResolverMap.find(reintroducedLiteral) == literalToResolverMap.end())
            {
              // this case should never happen really:
              // if a literal was removed earlier, there must be a clause that removed it
              allReintroducedLiteralsCanBeReRemoved = false;
              break;
            }

            // check if resolution is possible on the new current seed
            auto possibleReResolver = literalToResolverMap[reintroducedLiteral];
            auto newResolventOpt = newResolvent->resolveOnVar(abs(reintroducedLiteral), *possibleReResolver);
            if (!newResolventOpt.has_value())
            {
              allReintroducedLiteralsCanBeReRemoved = false;
              break;
            }

            // resolution is possible, now check no more literals are re-introduced
            newResolvent = newResolventOpt.value();
            newResolvingClause = possibleReResolver;
            auto level2ReintroducedLiterals = newResolvent->intersect(alreadyEliminatedLiterals);
            if (!level2ReintroducedLiterals->literals.empty())
            {
              allReintroducedLiteralsCanBeReRemoved = false;
              break;
            }
          }

          // all good, let's recurse
          if (allReintroducedLiteralsCanBeReRemoved)
          {
            LiteralToResolverMapInsertionBackTracker backTracker(literalToResolverMap, L, newResolvingClause);
            growSeed(newResolvent, resultClauses, searchDepth - 1, *(l2a.newEliminatedLiterals), literalToResolverMap);
          }
        }
      }
    }
  }






  Ave2ClauseVec Ave2::filterOutClausesWithNoVarsToEliminate(Ave2ClauseVec& clauses, Ave2Clause const& literalsToEliminate)
  {
    Ave2ClauseVec result = std::make_shared<std::vector<Ave2ClauseCPtr> >();
    for (size_t i = 0; i < clauses->size();)
    {
      auto const& clause = (*clauses)[i];
      auto intersectedClause = clause->intersect(literalsToEliminate);
      if (intersectedClause->literals.empty()) // nothing to eliminate
      {
        result->push_back(clause);
        (*clauses)[i] = clauses->back();
        clauses->pop_back();
      }
      else
      {
        ++i;
      }
    }
    return result;
  }





  Ave2ClauseCPtr Ave2Clause::concatenate(const Ave2Clause& that) const
  {
    std::vector<int> resultLiterals;
    resultLiterals.reserve(this->literals.size() + that.literals.size());
    auto it1 = this->literals.cbegin();
    auto it2 = that.literals.cbegin();
    while (it1 != this->literals.cend() || it2 != that.literals.cend())
    {
      if (it1 == this->literals.cend())
      {
        resultLiterals.push_back(*it2);
        ++it2;
      }
      else if (it2 == that.literals.cend())
      {
        resultLiterals.push_back(*it1);
        ++it1;
      }
      else if (*it1 < *it2)
      {
        resultLiterals.push_back(*it1);
        ++it1;
      }
      else if (*it2 < *it1)
      {
        resultLiterals.push_back(*it2);
        ++it2;
      }
      else
      {
        resultLiterals.push_back(*it1);
        ++it1;
        ++it2;
      }
    }
    return std::make_shared<Ave2Clause>(resultLiterals, false);
  }







  Ave2ClauseCPtr Ave2Clause::subtract(const Ave2Clause& that) const
  {
    std::vector<int> resultLiterals;
    auto it1 = this->literals.cbegin();
    auto it2 = that.literals.cbegin();
    while (it1 != this->literals.cend())
    {
      if (it2 == that.literals.cend())
      {
        resultLiterals.push_back(*it1);
        ++it1;
      }
      else if (*it1 < *it2)
      {
        resultLiterals.push_back(*it1);
        ++it1;
      }
      else if (*it2 < *it1)
      {
        ++it2;
      }
      else
      {
        // equal
        ++it1;
        ++it2;
      }
    }
    return std::make_shared<Ave2Clause>(resultLiterals, false);
  }







  std::optional<Ave2ClauseCPtr> Ave2Clause::resolveOnVar(size_t var, const Ave2Clause& that) const
  {
    auto it1 = this->literals.cbegin();
    auto it2 = that.literals.crbegin();
    std::vector<int> resultLeft, resultRight;
    resultLeft.reserve(this->literals.size());
    resultRight.reserve(that.literals.size());
    bool resolved = false;
    while (it1 != this->literals.cend() || it2 != that.literals.crend())
    {
      if (it1 == this->literals.cend())
      {
        if (std::abs(*it2) == var)
          return std::nullopt;
        resultRight.push_back(*it2);
        ++it2;
      }
      else if (it2 == that.literals.crend())
      {
        if (std::abs(*it1) == var)
          return std::nullopt;
        resultLeft.push_back(*it1);
        ++it1;
      }
      else if (*it1 < -*it2)
      {
        if (std::abs(*it1) == var)
          return std::nullopt;
        resultLeft.push_back(*it1);
        ++it1;
      }
      else if (-*it2 < *it1)
      {
        if (std::abs(*it2) == var)
          return std::nullopt;
        resultRight.push_back(*it2);
        ++it2;
      }
      else
      {
        // found x and -x
        if (std::abs(*it1) == var)
        {
          resolved = true;
        }
        else
        {
          return std::nullopt;
        }
        ++it1;
        ++it2;
      }
    }
    if (!resolved)
      return std::nullopt;
    std::vector<int> result;
    result.reserve(resultLeft.size() + resultRight.size());
    auto lit = resultLeft.cbegin();
    auto rit = resultRight.crbegin();
    while(lit != resultLeft.cend() || rit != resultRight.crend())
    {
      if (lit == resultLeft.cend())
      {
        result.push_back(*rit);
        ++rit;
      }
      else if (rit == resultRight.crend())
      {
        result.push_back(*lit);
        ++lit;
      }
      else if (*lit < *rit)
      {
        result.push_back(*lit);
        ++lit;
      }
      else if (*rit < *lit)
      {
        result.push_back(*rit);
        ++rit;
      }
      else
      {
        result.push_back(*lit);
        ++lit;
        ++rit;
      }
    }
    return std::make_shared<Ave2Clause>(result);
  }




} // end namespace oct_22