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
#include <string>
#include <cmath>


// dd includes
#include <dd/dd.h>


// blif_solve_lib includes
#include <blif_solve_lib/command_line_options.h>
#include <blif_solve_lib/log.h>
#include <blif_solve_lib/cnf_dump.h>


// test includes
#include "random_bdd_generator.h"


namespace {

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
  auto clippingDepthClo = CommandLineOptionValue<int>::create("--clipping_depth", "Depth for clipping approximation (mandatory)", -1);
  auto clippingDirectionClo = CommandLineOptionValue<int>::create("--clipping_direction", "Clipping direction 0/1 (deault 1)", 1);
  auto seedClo = CommandLineOptionValue<int>::create("--seed", "Seed for randomization", 20200123);
  auto probVarInClauseClo  = CommandLineOptionValue<double>::create("--prob_var_in_clause", "Probability for selecting a variable into a clause (default 0.8)",  0.8);
  auto avgSupportSetSizeClo  = CommandLineOptionValue<int>::create("--avg_support_set_size", "Average size of factor support sets (default num_vars *.8)", -1);
  auto minClausesInFunctionClo = CommandLineOptionValue<int>::create("--min_clauses_in_function", "Min number of clauses in function (default is 2^(num_vars-3), bounded by int max)", -1);
  auto maxClausesInFunctionClo = CommandLineOptionValue<int>::create("--max_clauses_in_function", "Max number of clauses in function (default is 2^(num_vars-2), bounded by int max)", -1);
  auto verbosityClo = CommandLineOptionValue<std::string>::create("--verbosity", "QUIET/ERROR/WARN/INFO/DEBUG (default INFO)", "INFO");
  auto numVarsToQuantifyClo = CommandLineOptionValue<int>::create("--num_vars_to_quantify", "Number of vars to existentially quantify away (default num_vars / 2)", -1);


  std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > options{ numFactorsClo, numVarsClo, 
                                                                         clippingDepthClo, clippingDirectionClo,
                                                                         seedClo, probVarInClauseClo, avgSupportSetSizeClo, 
                                                                         minClausesInFunctionClo, maxClausesInFunctionClo, 
                                                                         verbosityClo, numVarsToQuantifyClo };
  blif_solve::parseCommandLineOptions(argc - 1, argv + 1, options);
  
  
  int numFactors = numFactorsClo->getValue();
  int numVars = numVarsClo->getValue();
  int clippingDepth = clippingDepthClo->getValue();
  if (numFactors < 0 || numVars < 0 || clippingDepth < 0)
  {
    std::cout << "Missing mandatory parameters num_factors/num_vars/clipping_depth" << std::endl;
    blif_solve::printHelp(options);
    return -1;
  }
  int clippingDirection = clippingDirectionClo->getValue();
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
  blif_solve_log(INFO, "clippingDepth = " << clippingDepth);
  blif_solve_log(INFO, "clippingDirection = " << clippingDirection);
  blif_solve_log(INFO, "seed = " << seed);
  blif_solve_log(INFO, "probVarInClause = " << probVarInClause);
  blif_solve_log(INFO, "avgSupportSetSize = " << avgSupportSetSize);
  blif_solve_log(INFO, "minClausesInFunction = " << minClausesInFunction);
  blif_solve_log(INFO, "maxClausesInFunction = " << maxClausesInFunction);
  blif_solve_log(INFO, "numVarsToQuantify = " << numVarsToQuantify);




  blif_solve_log(INFO, "generating factors");
  DdManager * manager = Cudd_Init(0, 0, 256, 262144, 0);
  test::RandomBddGenerator bddGen(manager, numVars, seed, probVarInClause, 
                                  avgSupportSetSize, minClausesInFunction,
                                  maxClausesInFunction);
  auto factors = bddGen.generateFactors(numFactors);
  auto independentVars = bddGen.getIndependentVars(numVarsToQuantify);
  bdd_ptr varsToQuantify = bddGen.getVarsToQuantify(numVarsToQuantify);



  blif_solve_log(INFO, "computing exact result");
  bdd_ptr conjunction = bdd_one(manager);
  for (auto factor: factors)
  {
    bdd_ptr temp = bdd_and(manager, conjunction, factor);
    bdd_free(manager, conjunction);
    conjunction = temp;
  }
  bdd_ptr result = bdd_forsome(manager, conjunction, varsToQuantify);
  bdd_free(manager, conjunction);
  blif_solve_log_bdd(DEBUG, "exact result is ", manager, result);
  std::cout << "Exact number of solutions = " << bdd_count_minterm(manager, result, numVars - numVarsToQuantify) << std::endl;
  bdd_free(manager, result);




  blif_solve_log(INFO, "computing clipped result");
  result = bdd_clipping_and_exists_multi(manager, factors, varsToQuantify, clippingDepth, clippingDirection);
  blif_solve_log_bdd(DEBUG, "clipped result is ", manager, result);
  std::cout << "Clipped number of solutions = " << bdd_count_minterm(manager, result, numVars - numVarsToQuantify) << std::endl;
  bdd_free(manager, result);



  bdd_free(manager, varsToQuantify);
  for (auto factor: factors)
    bdd_free(manager, factor);

  blif_solve_log(INFO, "DONE");
  return 0;
}
