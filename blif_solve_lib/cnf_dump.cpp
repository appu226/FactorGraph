#include "cnf_dump.h"

// cudd includes
#include <util.h>
#include <cuddInt.h>

// std includes
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <fstream>


namespace {

  // ----------------------- Class --------------------------
  // CnfDumpCache
  //   A cache containing the various variables that have
  //   been added to represent independent and tseytin vars
  //   
  class CnfDumpCache 
  {
    public:
      int getCnfVarForIndependentVar(int bdd_var_index);
      int getCnfVarForTseytinVar(bdd_ptr func);
      bool isAlreadyWritten(bdd_ptr func) const;
      std::vector<int> getAllIndependentCnfVars() const;
      int getNumVars() const;
      CnfDumpCache();

    private:
      int m_counter;
      std::map<int, int> m_independentVars;
      std::map<bdd_ptr, int> m_tseytinVars;

  }; // end class CnfDumpCache





  // ----------------------------------------------------
  // function definitions for CnfDumpCache
  CnfDumpCache::CnfDumpCache() :
    m_counter(0),
    m_independentVars(),
    m_tseytinVars()
  { }

  int CnfDumpCache::getCnfVarForIndependentVar(int bdd_var_index)
  {
    auto resultIt = m_independentVars.find(bdd_var_index);
    if (m_independentVars.end() == resultIt)
    {
      int newCnfVar = ++m_counter;
      m_independentVars[bdd_var_index] = newCnfVar;
      return newCnfVar;
    } 
    else
      return resultIt->second;
  }

  int CnfDumpCache::getCnfVarForTseytinVar(bdd_ptr func)
  {
    auto resultIt = m_tseytinVars.find(func);
    if (m_tseytinVars.end() == resultIt)
    {
      int newCnfVar = ++m_counter;
      m_tseytinVars[func] = newCnfVar;
      return newCnfVar;
    }
    else
      return resultIt->second;
  }

  bool CnfDumpCache::isAlreadyWritten(bdd_ptr func) const
  {
    return (m_tseytinVars.end() != m_tseytinVars.find(func));
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
              CnfDumpCache & cnfDumpCache)
  {
    if (cnfDumpCache.isAlreadyWritten(func))
      return DumpCnfResult(cnfDumpCache.getCnfVarForTseytinVar(func), 0);

    //--------- Case 1-----------
    // base case: zero and one
    auto const one = DD_ONE(manager);
    auto const zero = Cudd_Not(one);
    if (one == func || zero == func)
    {
      int result = cnfDumpCache.getCnfVarForTseytinVar(func);
      // cnfVar(zero) = cnfVar(one) * -1
      int clause = result * (zero == func ? -1 : 1);
      outputStream << clause << " 0\n";
      return DumpCnfResult(result, 1);
    }
    

    
   
    //--------- Case 2 ---------
    // if the func is negated
    //   call recursively on the non-negated func
    //   and return -1 * tseytinVar
    auto funcRegular = Cudd_Regular(func);
    if (func != funcRegular)
    {
      auto regularDcr = dumpCnf(manager, funcRegular, outputStream, cnfDumpCache);
      return DumpCnfResult(regularDcr.tseytinVar * -1, regularDcr.numClauses);
    }

    
    

    // ---------- Case 3 ----------
    // recursive case: IfThenElse(v, t, e)
    
    auto vidx = manager->perm[funcRegular->index];
    auto v = cnfDumpCache.getCnfVarForIndependentVar(vidx);

    // recursive calls on t and e
    auto tfunc = cuddT(funcRegular);
    auto efunc = cuddE(funcRegular);
    auto tdcr = dumpCnf(manager, tfunc, outputStream, cnfDumpCache);
    auto edcr = dumpCnf(manager, efunc, outputStream, cnfDumpCache);
    auto t = tdcr.tseytinVar;
    auto e = edcr.tseytinVar;

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
    auto r = cnfDumpCache.getCnfVarForTseytinVar(func);
    outputStream << -r << " " << -v << " " <<  t << " 0\n"
                 << -r << " " <<  v << " " <<  e << " 0\n"
                 <<  r << " " <<  v << " " << -e << " 0\n"
                 <<  r << " " << -v << " " << -t << " 0\n";

    int numClauses = tdcr.numClauses + edcr.numClauses + 4;
    int tseytinVar = r;

    return DumpCnfResult(tseytinVar, numClauses);
  }

} // end anonymous namespace


namespace blif_solve {

  void dumpCnfForModelCounting(DdManager * manager,
                               bdd_ptr_set const & upperLimit,
                               bdd_ptr_set const & lowerLimit,
                               std::string const & headerFile,
                               std::string const & clauseFile)
  {
    CnfDumpCache cnfDumpCache;
    std::ofstream clauseOut(clauseFile);
    int numClauses = 0;

    
    
    // process upperLimit functions
    // and write their tseytin vars into individual clauses
    for (auto ulit = upperLimit.cbegin(); ulit != upperLimit.cend(); ++ulit)
    {
      auto dcr = dumpCnf(manager, *ulit, clauseOut, cnfDumpCache);
      numClauses += dcr.numClauses;
      clauseOut << dcr.tseytinVar << " 0\n";
      ++numClauses;
    }

    
    
    // process lowerLimit functions
    std::vector<int> llTseytins;
    llTseytins.reserve(lowerLimit.size());
    for (auto llit = lowerLimit.cbegin(); llit != lowerLimit.cend(); ++llit)
    {
      auto dcr = dumpCnf(manager, *llit, clauseOut, cnfDumpCache);
      numClauses += dcr.numClauses;
      llTseytins.push_back(dcr.tseytinVar);
    }
    // write the negation of all lower limit tseytins into a single clause
    for (auto llt: llTseytins)
      clauseOut << -llt << " ";
    clauseOut << "0\n";
    ++numClauses;


    // done processing clauses
    clauseOut.close();



    // write independent vars into header file
    std::ofstream headerOut(headerFile);
    auto independentCnfVars = cnfDumpCache.getAllIndependentCnfVars();
    headerOut << "c ind ";
    for (auto icv: independentCnfVars)
      headerOut << icv << " ";
    headerOut << "0\n";


    // write number of variables and number of clauses
    headerOut << "p cnf " << cnfDumpCache.getNumVars() << " " << numClauses << "\n";


    // done processing headers
    headerOut.close();

  }

} // end namespace blif_solve