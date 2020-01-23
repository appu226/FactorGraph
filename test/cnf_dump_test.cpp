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




// std includes
#include <iostream>
#include <random>
#include <string>
#include <cmath>


// dd includes
#include <dd.h>


// blif_solve_lib includes
#include <command_line_options.h>
#include <log.h>
#include <cnf_dump.h>


namespace {
  class RandomBddGenerator {
    public:
      RandomBddGenerator(
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

      ~RandomBddGenerator()
      {
        for (auto var: m_vars)
          bdd_free(m_manager, var);
      }

      bdd_ptr generateClause(const std::vector<int>& supportSet)
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

      bdd_ptr generateFunc(int numClauses = 0, std::vector<int> supportSet = std::vector<int>())
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

      bdd_ptr_set generateFactors(int numFactors)
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

      bdd_ptr_set getIndependentVars(int numVarsToQuantify) const
      {
        bdd_ptr_set result;
        for (int i = numVarsToQuantify; i < m_vars.size(); ++i)
          result.insert(m_vars[i]);
        return result;
      }

      bdd_ptr getVarsToQuantify(int numVarsToQuantify) const
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



    private:
      DdManager * m_manager;
      std::vector<bdd_ptr> m_vars;
      std::mt19937 m_randomEngine;
      std::bernoulli_distribution m_distVarInClause;
      std::bernoulli_distribution m_distNegateVarInClause;
      std::bernoulli_distribution m_distVarInSupportSet;
      std::uniform_int_distribution<int> m_distNumClausesInFunction;
  };


  class CnfDumpTestClo
  {
    public:
      int numFactors;
      int numVars;
      int seed;
      double probVarInClause;
      int avgSupportSetSize;
      int minClausesInFunction;
      int maxClausesInFunction;
      int numClausesInConjunctionDriver;

  };
} // end anonymous namespace


int main(int argc, char const * const * const argv)
{
  using blif_solve::CommandLineOptionValue;
  
  
  auto numFactorsClo = CommandLineOptionValue<int>::create("--num_factors", "Number of factors (mandatory)", -1);
  auto numVarsClo = CommandLineOptionValue<int>::create("--num_vars", "Number of vars (mandatory)", -1);
  auto cnfClo = CommandLineOptionValue<std::string>::create("--cnf", "CNF output path (mandatory)", "");
  auto seedClo = CommandLineOptionValue<int>::create("--seed", "Seed for randomization", 20200123);
  auto probVarInClauseClo  = CommandLineOptionValue<double>::create("--prob_var_in_clause", "Probability for selecting a variable into a clause (default 0.8)",  0.8);
  auto avgSupportSetSizeClo  = CommandLineOptionValue<int>::create("--avg_support_set_size", "Average size of factor support sets (default num_vars *.8)", -1);
  auto minClausesInFunctionClo = CommandLineOptionValue<int>::create("--min_clauses_in_function", "Min number of clauses in function (default is 2^(num_vars-3), bounded by int max)", -1);
  auto maxClausesInFunctionClo = CommandLineOptionValue<int>::create("--max_clauses_in_function", "Max number of clauses in function (default is 2^(num_vars-2), bounded by int max)", -1);
  auto verbosityClo = CommandLineOptionValue<std::string>::create("--verbosity", "QUIET/ERROR/WARN/INFO/DEBUG (default INFO)", "INFO");
  auto numVarsToQuantifyClo = CommandLineOptionValue<int>::create("--num_vars_to_quantify", "Number of vars to existentially quantify away (default num_vars / 2)", -1);


  std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > options{ numFactorsClo, numVarsClo, cnfClo, seedClo, 
                                                                         probVarInClauseClo, avgSupportSetSizeClo, 
                                                                         minClausesInFunctionClo, maxClausesInFunctionClo, 
                                                                         verbosityClo, numVarsToQuantifyClo };
  blif_solve::parseCommandLineOptions(argc - 1, argv + 1, options);
  
  
  int numFactors = numFactorsClo->getValue();
  int numVars = numVarsClo->getValue();
  std::string cnfFilePath = cnfClo->getValue();
  if (numFactors < 0 || numVars < 0 || cnfFilePath == "")
  {
    std::cout << "Missing mandatory parameters num_factors/num_vars/cnf" << std::endl;
    blif_solve::printHelp(options);
    return -1;
  }
  int seed = seedClo->getValue();
  double probVarInClause = probVarInClauseClo->getValue();
  int avgSupportSetSize = avgSupportSetSizeClo->getValue();
  if (avgSupportSetSize < 0)
    avgSupportSetSize = std::max(static_cast<int>(numVars * 0.8), 1);
  int minClausesInFunction = minClausesInFunctionClo->getValue();
  if (minClausesInFunction < 0) 
    minClausesInFunction = static_cast<int>(std::pow(2.0, std::min(31, std::max(0, numVars - 3))));
  int maxClausesInFunction = maxClausesInFunctionClo->getValue();
  if (maxClausesInFunction < 0) 
    maxClausesInFunction = static_cast<int>(std::pow(2.0, std::min(31, std::max(0, numVars - 2))));
  int numVarsToQuantify = numVarsToQuantifyClo->getValue();
  if (numVarsToQuantify < 0) numVarsToQuantify = numVars / 2;
  
  
  blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosityClo->getValue()));
  blif_solve_log(INFO, "numFactors = " << numFactors);
  blif_solve_log(INFO, "numVars = " << numVars);
  blif_solve_log(INFO, "seed = " << seed);
  blif_solve_log(INFO, "probVarInClause = " << probVarInClause);
  blif_solve_log(INFO, "avgSupportSetSize = " << avgSupportSetSize);
  blif_solve_log(INFO, "minClausesInFunction = " << minClausesInFunction);
  blif_solve_log(INFO, "maxClausesInFunction = " << maxClausesInFunction);
  blif_solve_log(INFO, "numVarsToQuantify = " << numVarsToQuantify);
  blif_solve_log(INFO, "cnfFilePath = " << cnfFilePath);


  blif_solve_log(INFO, "generating factors");
  DdManager * manager = Cudd_Init(0, 0, 256, 262144, 0);
  RandomBddGenerator bddGen(manager, numVars, seed, probVarInClause, 
                            avgSupportSetSize, minClausesInFunction,
                            maxClausesInFunction);
  auto factors = bddGen.generateFactors(numFactors);
  auto independentVars = bddGen.getIndependentVars(numVarsToQuantify);


  blif_solve_log(INFO, "dumping cnf");
  blif_solve::dumpCnfForModelCounting(manager, independentVars, factors, bdd_ptr_set(), cnfFilePath);
  blif_solve_log(INFO, "running ganak");
  std::system(("scripts/ganak.sh ${PWD}/" + cnfFilePath + " 2> /dev/null | grep -A 1 '# solutions' | tail -n 1 ").c_str());


  blif_solve_log(INFO, "computing bdd result");
  bdd_ptr conjunction = bdd_one(manager);
  bdd_ptr varsToQuantify = bddGen.getVarsToQuantify(numVarsToQuantify);
  for (auto factor: factors)
  {
    bdd_ptr temp = bdd_and(manager, conjunction, factor);
    bdd_free(manager, conjunction);
    conjunction = temp;
  }
  bdd_ptr result = bdd_forsome(manager, conjunction, varsToQuantify);
  blif_solve_log_bdd(DEBUG, "result is ", manager, result);
  std::cout << bdd_count_minterm(manager, result, numVars - numVarsToQuantify) << std::endl;


  bdd_free(manager, result);
  bdd_free(manager, conjunction);
  bdd_free(manager, varsToQuantify);
  for (auto factor: factors)
    bdd_free(manager, factor);

  blif_solve_log(INFO, "DONE");
  return 0;
}
