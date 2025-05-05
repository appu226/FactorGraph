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

  struct AveClause
  {
    AveIntVec literals;
    bool isAcceptable;
    int numFlippedLiterals;
    size_t hash;
    AveClause(AveIntVec v_literals);

    bool operator==(const AveClause& that) const;
    bool operator!=(const AveClause& that) const;
    bool operator<(const AveClause& that) const;
    bool operator>(const AveClause& that) const;
    bool operator<=(const AveClause& that) const;
    bool operator>=(const AveClause& that) const;
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

  struct AveLiteral
  {
    int literal;
    AveClausePtrSet clauses;
    AveLiteral(int v_literal) : literal(v_literal), clauses() {}
  };

  class ApproxVarElim
  {
    private:
      AveClausePtrSet m_clauses;
      AveLiteralPtrVec m_positiveLiterals;
      AveLiteralPtrVec m_negativeLiterals;
      AveIntVec m_varsToEliminate;


    public:
      using Ptr = std::shared_ptr<ApproxVarElim>;
      static Ptr parseQdimacs(const dd::Qdimacs& qdimacs);
      AveClausePtrSet const& getClauses() const;
      void approximatelyEliminateAllVariables(size_t numClauseIncPerVarElim);

    private:
      void addClause(const AveClausePtr& clause);
      void removeClause(const AveClausePtr& clause);
      AveLiteralPtr const& getLiteral(int literal) const;
      AveClausePtrSet const& getClausesWithLiteral(int literal) const;
      void approximatelyEliminateVar(int x, std::unordered_set<int> const& allVarsToElim, size_t numClauseIncPerVarElim);
      size_t getNonTrivialPairCount(int x) const;

  }; // end class ApproxVarElim


} // end namespace oct_22