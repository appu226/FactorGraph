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

#pragma once

#include <dd/qdimacs.h>
#include <dd/doubly_linked_list.h>

#include <functional>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace oct_22
{
  struct AveLiteral;
  struct AveClause;

  using AveIntVec = std::vector<int>;
  using AveLiteralPtr = std::shared_ptr<AveLiteral>;
  using AveLiteralPtrVec = std::vector<AveLiteralPtr>;
  using AveLiteralRPtr = AveLiteral*;
  using AveLiteralRPtrVec = std::vector<AveLiteralRPtr>;
  using AveClausePtr = std::shared_ptr<AveClause>;
  using AveClauseList = parakram::DlList<AveClausePtr>;
  using AveClauseListPtr = std::shared_ptr<AveClauseList>;
  using AveClauseListWPtr = std::weak_ptr<AveClauseList>;
  using AveClauseListNode = parakram::DlListNode<AveClausePtr>;
  using AveClauseListNodePtr = std::shared_ptr<AveClauseListNode>;
  using AveClauseListNodeWPtr = std::weak_ptr<AveClauseListNode>;
  using AveClauseUpdateFunc = std::function<void(AveClause&)>;

  struct AveLiteralBoolMap {
    std::vector<bool> positiveLiterals;
    std::vector<bool> negativeLiterals;

    using Ptr = std::shared_ptr<AveLiteralBoolMap>;

    AveLiteralBoolMap(size_t numLiterals)
      : positiveLiterals(numLiterals, false),
        negativeLiterals(numLiterals, false)
    {
    }

    bool get(int literal) const
    {
      if (literal > 0)
      {
        return positiveLiterals[literal - 1];
      }
      else
      {
        return negativeLiterals[-literal - 1];
      }
    }

    void set(int literal, bool value)
    {
      if (literal > 0)
      {
        positiveLiterals[literal - 1] = value;
      }
      else
      {
        negativeLiterals[-literal - 1] = value;
      }
    }
  };

  struct AveClause
  {
    AveIntVec literals;
    size_t numFlippedQuantifiedLiterals;
    size_t numFlippedNonQuantifiedLiterals;
    size_t hash;
    AveClauseListWPtr resolvableList;
    AveClauseListNodeWPtr resolvableListIter;
    bool isEnabled;
    
    AveClause(AveIntVec v_literals);

    bool isResolvable() const
    {
      return numFlippedQuantifiedLiterals == 1 && numFlippedNonQuantifiedLiterals == 0;
    }

    void update(AveClauseUpdateFunc operation)
    {
      bool oldIsResolvable = isResolvable();
      operation(*this);
      AveClauseListPtr list = resolvableList.lock();
      if (!list)
        return;
      AveClauseListNodePtr iter = resolvableListIter.lock();
      if (!iter)
        return;
      bool newIsResolvable = isResolvable();
      if (oldIsResolvable && !newIsResolvable)
      {
        list->get_next_and_erase(iter);
      }
      else if (!oldIsResolvable && newIsResolvable && isEnabled)
      {
        list->reinsert_node_before_node(iter, list->end());
      }
    }

    bool operator==(const AveClause& that) const
    {
      if (hash != that.hash)
      {
        return false;
      }
      return this->literals == that.literals;
    }

    bool operator!=(const AveClause& that) const
    {
      if (hash != that.hash)
      {
        return true;
      }
      return this->literals != that.literals;
    }

    bool operator<(const AveClause& that) const
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

    bool operator>(const AveClause& that) const
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

    bool operator<=(const AveClause& that) const
    {
      return !(*this > that);
    }
    
    bool operator>=(const AveClause& that) const
    {
      return !(*this < that);
    }
  };

  struct AveClausePtrHash
  {
    size_t operator()(const AveClausePtr& clause) const
    {
      return clause->hash;
    }
  };
  struct AveClausePtrEqual
  {
    bool operator()(const AveClausePtr& lhs, const AveClausePtr& rhs) const
    {
      return *lhs == *rhs;
    }
  };

  using AveClausePtrSet = std::unordered_set<AveClausePtr, AveClausePtrHash, AveClausePtrEqual>;
  using AveClausePtrVec = std::vector<AveClausePtr>;

  struct AveLiteral
  {
    int literal;
    AveClausePtrVec clauses;
    AveLiteral(int v_literal) : literal(v_literal), clauses() {}
  };

  struct AveSeedModification {
    AveIntVec quantifiedLiteralsToAdd;
    AveIntVec nonQuantifiedLiteralsToAdd;
    AveIntVec quantifiedLiteralsToRemove;
    AveIntVec nonQuantifiedLiteralsToRemove;
    int pivotSeedLiteral;

    AveSeedModification(AveIntVec const& oldSeed,
                        AveIntVec const& clause, 
                        AveIntVec const& quantifiedVariables,
                        AveClausePtr& newSeed);
  
    void flip() {
      std::swap(quantifiedLiteralsToAdd, quantifiedLiteralsToRemove);
      std::swap(nonQuantifiedLiteralsToAdd, nonQuantifiedLiteralsToRemove);
    }
  };
  

  class ApproxVarElim
  {
    private:
      AveClausePtrSet m_clauses;
      AveClausePtrSet m_resultClauses;
      AveLiteralPtrVec m_positiveLiterals;
      AveLiteralPtrVec m_negativeLiterals;
      AveIntVec m_varsToEliminate;
      AveLiteralBoolMap::Ptr m_isLiteralUsedAsPivot;


    public:
      using Ptr = std::shared_ptr<ApproxVarElim>;
      static Ptr parseQdimacs(const dd::Qdimacs& qdimacs);
      AveClausePtrSet const& getResultClauses() const;
      void approximatelyEliminateAllVariables(size_t maxClauseTreeSize, size_t numMaxSeconds = 0);
      
      // Returns all `l` in `literals` such that `abs(l)` is in `variables`
      // literals, variables must be sorted in ascending order
      // result is sorted in ascending order
      static AveIntVec intersection(
        AveIntVec const& literals,
        AveIntVec const& variables
      );
      
      // Returns a vector of literals from the clauseLiterals that whose negatives are present in seedLiterals
      // clauseLiterals, seedLiterals must be sorted in ascending order
      // result is sorted in ascending order
      static AveIntVec negatedLiterals(
        AveIntVec const& clauseLiterals,
        AveIntVec const& seedLiterals
      );

      static AveClausePtr resolve(
        AveIntVec const& c1,
        AveIntVec const& c2,
        int pivot
      );

    private:

      void addClause(const AveClausePtr& clause);
      AveLiteralPtr const& getLiteral(int literal) const;
      AveClausePtrVec const& getClausesWithLiteral(int literal) const;
      void elimHelper(
        AveClausePtr const& resultSeed,
        AveClauseList& resolvableClauses,
        size_t numClauseIncPerVarElim,
        std::shared_ptr<std::atomic<bool>> const& hasExpired);

      void applySeedModification(
        AveSeedModification const& seedModification
      );

  }; // end class ApproxVarElim


} // end namespace oct_22