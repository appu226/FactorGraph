/*

Copyright 2023 Parakram Majumdar

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
  struct AveVar;
  struct AveClause;

  using AveVarPtr = std::shared_ptr<AveVar>;
  using AveVarPtrVec = std::vector<std::shared_ptr<AveVar> >;
  using AveVarWPtr = std::weak_ptr<AveVar>;
  using AveVarWPtrVec = std::vector<std::weak_ptr<AveVar> >;
  using AveClausePtr = std::shared_ptr<AveClause>;
  using AveClausePtrVec = std::vector<std::shared_ptr<AveClause> >;
  using AveClauseWPtr = std::weak_ptr<AveClause>;
  using AveClauseWPtrVec = std::vector<std::weak_ptr<AveClause> >;

  struct AveVar
  {
    int literal;
    AveClauseWPtrVec clauses;
  };

  struct AveClause
  {
    AveVarWPtrVec vars;
  };

  class ApproxVarElim
  {
    private:
      std::unordered_set<AveClausePtr> m_clauses;
      std::unordered_map<int, AveVarPtrVec> m_vars;


    public:
      using Ptr = std::shared_ptr<ApproxVarElim>;
      static Ptr parseQdimacs(const dd::Qdimacs& qdimacs);
      void approximatelyEliminateVar(int var);
      AveClausePtrVec const& getClauses() const;

    private:
      void addClause(const AveClausePtr& clause);
      void removeClause(const AveClausePtr& clause);
      void removeVar(const AveVarPtr& var);
      AveVarPtr getVar(int var) const;

  }; // end class ApproxVarElim


} // end namespace oct_22