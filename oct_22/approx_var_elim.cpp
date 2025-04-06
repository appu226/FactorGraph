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

#include "approx_var_elim.h"

namespace oct_22
{

  ApproxVarElim::Ptr ApproxVarElim::parseQdimacs(dd::Qdimacs const& qdimacs)
  {
    throw std::runtime_error("ApproxVarElim::parseQdimacs not yet implemented");
  }


  void ApproxVarElim::addClause(const AveClausePtr& clause)
  {
    throw std::runtime_error("ApproxVarElim::addClause not yet implemented");
  }

  void ApproxVarElim::removeClause(const AveClausePtr& clause)
  {
    throw std::runtime_error("ApproxVarElim::removeClause not yet implemented");
  }

  void ApproxVarElim::removeVar(const AveVarPtr& var)
  {
    throw std::runtime_error("ApproxVarElim::removeVar not yet implemented");
  }

  void ApproxVarElim::approximatelyEliminateVar(int var)
  {
    // for every quantified variable x
    // find set XP clauses that have x, and set XM clauses that have -x
    // take some sub-set of the cross product conjunction of XP and XM
    throw std::runtime_error("ApproxVarElim::approximatelyEliminateVar not yet implemented");
  }

  AveVarPtr ApproxVarElim::getVar(int var) const
  {
    throw std::runtime_error("ApproxVarElim::getVar not yet implemented");
  }

  AveClausePtrVec const& ApproxVarElim::getClauses() const
  {
    throw std::runtime_error("ApproxVarElim::getClauses not yet implemented");
  }



} // end namespace oct_22