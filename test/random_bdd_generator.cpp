/*

Copyright 2019 Parakram Majumdar

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

#include <stdexcept>
#include <log.h>

#include "random_bdd_generator.h"

namespace test {

  RandomBddGenerator::RandomBddGenerator(
      DdManager * manager, 
      int numVars, 
      int seed, 
      double probVarInClause,
      int avgSupportSetSize,
      int minClausesInFunction,
      int maxClausesInFunction) :
    m_manager(manager),
    m_vars(),
    m_randomEngine(seed),
    m_distVarInClause(probVarInClause),
    m_distNegateVarInClause(0.5),
    m_distVarInSupportSet(static_cast<double>(avgSupportSetSize)/numVars),
    m_distNumClausesInFunction(minClausesInFunction, maxClausesInFunction)
  {
    for (int vidx = 0; vidx < numVars; ++vidx)
      m_vars.push_back(bdd_new_var_with_index(m_manager, vidx));
  }
  
  RandomBddGenerator:: ~RandomBddGenerator()
  {
    for (auto var: m_vars)
      bdd_free(m_manager, var);
  }

  bdd_ptr RandomBddGenerator::generateClause(const std::vector<int>& supportSet)
  {
    bdd_ptr clause = bdd_one(m_manager);
    for (int v: supportSet)
    {
      if (v < 0 || v >= m_vars.size())
        throw std::runtime_error("Invalid var index " + std::to_string(v));
      bool pickVarInClause = m_distVarInClause(m_randomEngine);
      if (pickVarInClause)
      {
        bool negateVarInClause = m_distNegateVarInClause(m_randomEngine);
        bdd_ptr var = (negateVarInClause ? bdd_not(m_manager, m_vars[v]) : bdd_dup(m_vars[v]));
        bdd_ptr newClause = bdd_and(m_manager, clause, var);
        bdd_free(m_manager, clause);
        bdd_free(m_manager, var);
        clause = newClause;
      }
    }
    return clause;
  }

  bdd_ptr RandomBddGenerator::generateFunc(int numClauses, std::vector<int> supportSet)
  {
    if (supportSet.empty())
    {
      for (int vidx = 0; vidx < m_vars.size(); ++vidx)
        if (m_distVarInSupportSet(m_randomEngine))
          supportSet.push_back(vidx);
    }
    if (numClauses <= 0)
      numClauses = m_distNumClausesInFunction(m_randomEngine);

    bdd_ptr func = bdd_zero(m_manager);
    for (int i = 0; i < numClauses; ++i)
    {
      bdd_ptr clause = generateClause(supportSet);
      bdd_ptr newFunc = bdd_or(m_manager, clause, func);
      bdd_free(m_manager, func);
      bdd_free(m_manager, clause);
      func = newFunc;
    }
    return func;
  }


  bdd_ptr_set RandomBddGenerator::generateFactors(int numFactors)
  {
    std::vector<bdd_ptr> factorVec;
    factorVec.reserve(numFactors);
    for (int ifact = 0; ifact < numFactors; ++ifact)
    {
      factorVec.push_back(generateFunc());
    }

    bdd_ptr_set result;
    for (auto factor: factorVec)
    {
      if (result.count(factor) > 0)
        bdd_free(m_manager, factor);
      else
      {
        blif_solve_log_bdd(DEBUG, "printing factor", m_manager, factor);
        result.insert(factor);
      }
    }

    return result;

  }



  bdd_ptr_set RandomBddGenerator::getIndependentVars(int numVarsToQuantify) const
  {
    bdd_ptr_set result;
    for (int i = numVarsToQuantify; i < m_vars.size(); ++i)
      result.insert(m_vars[i]);
    return result;
  }



  bdd_ptr RandomBddGenerator::getVarsToQuantify(int numVarsToQuantify) const
  {
    bdd_ptr result = bdd_one(m_manager);
    for (int i = 0; i < numVarsToQuantify; ++i)
    {
      auto temp = bdd_cube_union(m_manager, result, m_vars[i]);
      bdd_free(m_manager, result);
      result = temp;
    }
    return result;
  }





} // end namespace test
