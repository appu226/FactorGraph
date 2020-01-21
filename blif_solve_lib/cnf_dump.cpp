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



#include "cnf_dump.h"

// blif_solve_lib includes
#include "log.h"

// cudd includes
#include <util.h>
#include <cuddInt.h>

// std includes
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <fstream>

// dd includes
#include <dd.h>


namespace {


  // ----------------------- Struct ------------------------
  // FuncProfile
  //   Gives commonly usable info about a bdd
  struct FuncProfile
  {
    int negationFactor; // is -1 for negated bdds, 1 for normal bdds
    int index;          // index of the condition variable of the bdd
    bool isVar;         // whether the bdd is a variable or not
    FuncProfile(DdManager * manager, bdd_ptr func)
    {
      if (func == Cudd_ReadOne(manager) || func == Cudd_ReadLogicZero(manager))
        throw std::runtime_error("Cannot compute profile for one/zero bdd: did you pass all variables to be quantified away?");
      bdd_ptr funcRegular = Cudd_Regular(func);
      negationFactor = (func == funcRegular) ? 1 : -1;
      index = funcRegular->index;
      bdd_ptr condVar = bdd_new_var_with_index(manager, index);
      isVar = Cudd_Regular(condVar) == funcRegular;
      bdd_free(manager, condVar);
    }
  };




  // ----------------------- Class --------------------------
  // CnfDumpCache
  //   A cache containing the various variables that have
  //   been added to represent independent and tseytin vars
  //   
  class CnfDumpCache 
  {
    public:
      void addCnfVarForIndependentVar(int bdd_var_index);
      int getCnfVarForBddVar(int bdd_var_index);
      int getCnfVarForBddVar(bdd_ptr var);
      int getCnfVarForTseytinVar(bdd_ptr func);
      bool isAlreadyWritten(bdd_ptr func) const;
      bool isIndependentVar(int bdd_var_index) const;
      std::vector<int> getAllIndependentCnfVars() const;
      int getNumVars() const;
      void debugAllCnfVars() const;

      CnfDumpCache(DdManager* ddm, bdd_ptr_set const & independentVars);

    private:
      int m_counter;
      std::map<int, int> m_independentVars;
      std::map<int, int> m_dependentVars;
      std::map<bdd_ptr, int> m_tseytinVars;
      DdManager * m_ddm;

  }; // end class CnfDumpCache





  // ----------------------------------------------------
  // function definitions for CnfDumpCache
  CnfDumpCache::CnfDumpCache(
      DdManager * ddm,
      bdd_ptr_set const & independentVars) :
    m_counter(0),
    m_independentVars(),
    m_tseytinVars(),
    m_ddm(ddm)
  {
    for (auto var: independentVars)
      addCnfVarForIndependentVar(Cudd_Regular(var)->index);
  }

  void CnfDumpCache::addCnfVarForIndependentVar(int bdd_var_index)
  {
    if (m_independentVars.end() == m_independentVars.find(bdd_var_index))
    {
      int newCnfVar = ++m_counter;
      m_independentVars[bdd_var_index] = newCnfVar;
    }
  }

  int CnfDumpCache::getCnfVarForBddVar(int bdd_var_index)
  {
    auto resultIt = m_independentVars.find(bdd_var_index);
    if (m_independentVars.end() == resultIt)
    {
      resultIt = m_dependentVars.find(bdd_var_index);
      if (m_dependentVars.end() == resultIt)
      {
        auto result = ++m_counter;
        m_dependentVars[bdd_var_index] = result;
        return result;
      }
      else
        return resultIt->second;
    } 
    else
      return resultIt->second;
  }

  int CnfDumpCache::getCnfVarForBddVar(bdd_ptr var)
  {
    FuncProfile varProfile(m_ddm, var);
    return getCnfVarForBddVar(varProfile.index) * varProfile.negationFactor;
  }

  int CnfDumpCache::getCnfVarForTseytinVar(bdd_ptr func)
  {
    // look for pre-added tseytin func
    auto resultIt = m_tseytinVars.find(func);
    if (m_tseytinVars.end() != resultIt)
      return resultIt->second;
    resultIt = m_tseytinVars.find(Cudd_Not(func));
    if (m_tseytinVars.end() != resultIt)
      return resultIt->second * -1;

    // look for pre-added variable
    FuncProfile funcProfile(m_ddm, func);
    if (funcProfile.isVar)
      return getCnfVarForBddVar(funcProfile.index) * funcProfile.negationFactor;

    // add new tseytin func
    int newCnfVar = ++m_counter;
    m_tseytinVars[func] = newCnfVar;
    return newCnfVar;
  }

  bool CnfDumpCache::isAlreadyWritten(bdd_ptr func) const
  {
    FuncProfile funcProfile(m_ddm, func);
    if (funcProfile.isVar)
      return (m_independentVars.end() != m_independentVars.find(funcProfile.index))
             || (m_dependentVars.end() != m_dependentVars.find(funcProfile.index));
    else
      return (m_tseytinVars.end() != m_tseytinVars.find(func))
              || (m_tseytinVars.end() != m_tseytinVars.find(Cudd_Not(func)));
  }
  
  bool CnfDumpCache::isIndependentVar(int bdd_var_index) const
  {
    return (m_independentVars.end() != m_independentVars.find(bdd_var_index));
  }

  std::vector<int> CnfDumpCache::getAllIndependentCnfVars() const
  {
    std::vector<int> result;
    result.reserve(m_independentVars.size());
    for (auto iv: m_independentVars)
      result.push_back(iv.second);

    return result;
  }

  int CnfDumpCache::getNumVars() const
  {
    return m_counter;
  }

  void CnfDumpCache::debugAllCnfVars() const
  {
    for (auto independentVar: m_independentVars)
    {
      int index = independentVar.first;
      int cnfVar = independentVar.second;
      bdd_ptr varBdd = bdd_new_var_with_index(m_ddm, index);
      blif_solve_log_bdd(DEBUG, "cnfVar " << cnfVar << " represents bdd of index " << index, m_ddm, varBdd);
      bdd_free(m_ddm, varBdd);
    }

    for (auto dependentVar: m_dependentVars)
    {
      int index = dependentVar.first;
      int cnfVar = dependentVar.second;
      bdd_ptr varBdd = bdd_new_var_with_index(m_ddm, index);
      blif_solve_log_bdd(DEBUG, "cnfVar " << cnfVar << " represents bdd of index " << index, m_ddm, varBdd);
      bdd_free(m_ddm, varBdd);
    }

    for (auto tseytinVar: m_tseytinVars)
    {
      blif_solve_log_bdd(DEBUG, "cnfVar " << tseytinVar.second << " represents bdd ", m_ddm, tseytinVar.first);
    }
  }


  // ----------------------- Function -------------------------
  // ** Intro to dumpCnf
  //     writes the set of clauses for a given func
  // ** Parameters
  //   func
  //     the bdd function for which the clauses need to be written
  //   outputStream
  //     the output stream into which the clauses need to be written
  //   cnfDumpCache
  //     a cache object containing previously written cnf vars
  // ** Output
  //     a struct called DumpCnfResult, with two integers:
  //       tseytinVar: The var name of the tseytin cnf var 
  //                     for this bdd.
  //       numClauses: The number of clauses added for this bdd.
  //                   For bdd's that have already been added,
  //                     this number is zero.
  struct DumpCnfResult {
    int tseytinVar;
    int numClauses;
    DumpCnfResult(int tv, int nc): tseytinVar(tv), numClauses(nc) { }
  };
  DumpCnfResult dumpCnf(DdManager * manager,
              bdd_ptr func,
              std::ostream & outputStream,
              CnfDumpCache & cnfDumpCache,
              std::string const & debugIndent)
  {
    if (cnfDumpCache.isAlreadyWritten(func))
    {
      blif_solve_log(DEBUG, debugIndent << "already written");
      return DumpCnfResult(cnfDumpCache.getCnfVarForTseytinVar(func), 0);
    }

    //--------- Case 1-----------
    // base case: zero and one
    auto const one = DD_ONE(manager);
    auto const zero = Cudd_Not(one);
    if (one == func || zero == func)
    {
      int result = cnfDumpCache.getCnfVarForTseytinVar(func);
      // cnfVar(zero) = cnfVar(one) * -1
      int clause = result * (zero == func ? -1 : 1);
      blif_solve_log(DEBUG, debugIndent << "adding 1 clause '" << clause << " 0' for func " << (zero == func ? "zero" : "one"))
      outputStream << clause << " 0\n";
      return DumpCnfResult(result, 1);
    }
    

    

    // ---------- Case 3 ----------
    // recursive case: IfThenElse(v, t, e)
    
    auto funcRegular = Cudd_Regular(func);
    auto vidx = funcRegular->index;
    auto r = cnfDumpCache.getCnfVarForTseytinVar(func);
    bool isVExistentiallyQuantified = !cnfDumpCache.isIndependentVar(vidx);

    // recursive calls on t and e
    auto tfunc = cuddT(funcRegular); if (func != funcRegular) tfunc = Cudd_Not(tfunc);
    auto efunc = cuddE(funcRegular); if (func != funcRegular) efunc = Cudd_Not(efunc);
    blif_solve_log_bdd(DEBUG, debugIndent << "dumping left branch ", manager, tfunc);
    auto tdcr = dumpCnf(manager, tfunc, outputStream, cnfDumpCache, debugIndent + "  ");
    blif_solve_log_bdd(DEBUG, debugIndent << "dumping right branch ", manager, efunc);
    auto edcr = dumpCnf(manager, efunc, outputStream, cnfDumpCache, debugIndent + "  ");
    auto t = tdcr.tseytinVar;
    auto e = edcr.tseytinVar;




    // ---------- Case 3.1 --------
    // Special case: if v is to be existentially quantified out,
    // then r is (t or e)
    if (isVExistentiallyQuantified)
    {
      // (r <-> (t or e)) = (r -> (t or e)) and (!r -> (!t and !e))    // !t and !e <- naughty pronounciations :D
      //                  = (!r or t or e) and (r or (!t and !e))
      //                  = (!r or t or e) and (r or !t) and (r or !e)
      outputStream << -r << " " <<  t << " " <<  e << " 0\n"
                   <<  r << " " << -t              << " 0\n"
                   <<  r << " "              << -e << " 0\n";
      blif_solve_log_bdd(DEBUG, 
                         debugIndent << "adding 3 clauses on tseytin vars " << r << " " << t << " " << e 
                                     << " because var with bdd index " << vidx
                                     << " is to be existentially quantified in bdd ",
                         manager,
                         func);
      int numClauses = tdcr.numClauses + edcr.numClauses + 3;
      int tseytinVar = r;
      return DumpCnfResult(tseytinVar, numClauses);
    }

    // else: 
    // ------------- Case 3.2 -------------
    // 
    // create a new var r and write clauses to set it up as IfThenElse(v, t, e)
    // r <-> IfThenElse(v, t, e)
    // == r <-> (v -> t) and (!v -> e)
    // == (r -> (v -> t) and (!v -> e)) and (!r -> !((v -> t) and (!v -> e)))
    // == (r -> (v -> t)) and (r -> (!v -> e)) 
    //                    and (!r -> (!(v -> t) or !(!v -> e)))
    // == (!r or !v or t) and (!r or v or e)
    //                    and (r or (v and !t) or (!v and !e)).................. (1)
    //
    // r or (v and !t) or (!v and !e)
    // == r or ((v or !v) and (v or !e) and (!t or !v) and (!t or !e))
    // == r or (              (v or !e) and (!t or !v) and (!t or !e))
    // == (r or v or !e) and (r or !v or !t) and (r or !t or !e)
    // == (r or v or !e) and (r or !v or !t)   [since the first two clauses 
    //                                          together imply the third clause]
    //                                      ................................... (2)
    //
    // combining (1) and (2), we get
    // r <-> IfThenElse(v, t, e)
    // == (!r or !v or t) and (!r or v or e) and (r or v or !e) and (r or !v or !t)
    auto v = cnfDumpCache.getCnfVarForBddVar(vidx);
    outputStream << -r << " " << -v << " " <<  t << " 0\n"
                 << -r << " " <<  v << " " <<  e << " 0\n"
                 <<  r << " " <<  v << " " << -e << " 0\n"
                 <<  r << " " << -v << " " << -t << " 0\n";

    blif_solve_log_bdd(DEBUG,
                       debugIndent << "adding 4 clauses on tseytin vars " 
                                   << r << " " << v << " " << t << " " << e
                                   << " for bdd ",
                       manager,
                       func);
    int numClauses = tdcr.numClauses + edcr.numClauses + 4;
    int tseytinVar = r;

    return DumpCnfResult(tseytinVar, numClauses);
  }

} // end anonymous namespace


namespace blif_solve {

  void dumpCnfForModelCounting(DdManager * manager,
                               bdd_ptr_set const & allVars,
                               bdd_ptr_set const & upperLimit,
                               bdd_ptr_set const & lowerLimit,
                               std::string const & outputPath)
  {
    CnfDumpCache cnfDumpCache(manager, allVars);
    std::string const clauseFile = outputPath + ".clauses.txt";
    std::ofstream clauseOut(clauseFile);
    int numClauses = 0;

    
    
    // process upperLimit functions
    // and write their tseytin vars into individual clauses
    for (auto ulit = upperLimit.cbegin(); ulit != upperLimit.cend(); ++ulit)
    {
      blif_solve_log_bdd(DEBUG, "dumping cnf for upper limit func", manager, *ulit);
      auto dcr = dumpCnf(manager, *ulit, clauseOut, cnfDumpCache, "");
      numClauses += dcr.numClauses;
      clauseOut << dcr.tseytinVar << " 0\n";
      ++numClauses;
    }

    
    
    // process lowerLimit functions
    if (!lowerLimit.empty())
    {
      std::vector<int> llTseytins;
      llTseytins.reserve(lowerLimit.size());
      for (auto llit = lowerLimit.cbegin(); llit != lowerLimit.cend(); ++llit)
      {
        blif_solve_log_bdd(DEBUG, "dumping cnf for func", manager, *llit);
        auto dcr = dumpCnf(manager, *llit, clauseOut, cnfDumpCache, "");
        numClauses += dcr.numClauses;
        llTseytins.push_back(dcr.tseytinVar);
      }
      // write the negation of all lower limit tseytins into a single clause
      for (auto llt: llTseytins)
        clauseOut << -llt << " ";
      clauseOut << "0\n";
      ++numClauses;
    }

    // done processing clauses
    clauseOut.close();



    // write independent vars into the final output
    std::ofstream finalOut(outputPath);
    auto independentCnfVars = cnfDumpCache.getAllIndependentCnfVars();
    finalOut << "c ind ";
    for (auto icv: independentCnfVars)
      finalOut << icv << " ";
    finalOut << "0\n";


    // write number of variables and number of clauses
    finalOut << "p cnf " << cnfDumpCache.getNumVars() << " " << numClauses << "\n";


    // append the clauses into the final file
    std::ifstream clauseIn(clauseFile);
    std::string line;
    while(getline(clauseIn, line))
      finalOut << line << "\n";
    clauseIn.close();
    finalOut.close();

    if (blif_solve::getVerbosity() == blif_solve::DEBUG)
      cnfDumpCache.debugAllCnfVars();

  }

} // end namespace blif_solve
