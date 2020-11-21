/*

Copyright 2020 Parakram Majumdar

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



#include <log.h>
#include <command_line_options.h>
#include <blif_factors.h>
#include <srt.h>

#include <memory>


#include "var_score_quantification.h"


//////////////////
// declarations //
//////////////////

// structure and functions to parse and store command line options
struct VarScoreCommandLineOptions
{
  std::string verbosity;
  std::string blif;
  int maxBddSize;
  var_score::ApproximationMethod::CPtr approximationMethod;
  bool mustCountNumSolutions;
};
template<typename TValue>
std::shared_ptr<blif_solve::CommandLineOptionValue<TValue> > 
  addCommandLineOption(std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > & commandLineOptions,
                       const std::string & name,
                       const std::string & help,
                       const TValue & defaultValue);
VarScoreCommandLineOptions parseClo(int argc, char const * const * const argv);

// main
int main(int argc, char const * const * const argv);




///////////////////
// main function //
///////////////////
int main(int argc, char const * const * const argv)
{
  auto start = blif_solve::now();
  auto clo = parseClo(argc, argv); // parse the command line options
  auto srt = std::make_shared<SRT>(); // initialize BDD
  
  // parse the blif file
  blif_solve::BlifFactors bf(clo.blif, 0, srt->ddm);
  bf.createBdds();
  auto subProblems = bf.partitionFactors();
  blif_solve_log(INFO, "Parsed " << subProblems.size() << " sub problems in " << blif_solve::duration(start) << " sec");
  start = blif_solve::now();

  // solve each sub problem
  std::vector<bdd_ptr> resultVec;
  for (auto sp: subProblems)
  {
    auto rv = var_score::VarScoreQuantification::varScoreQuantification(*(sp->getFactors()), sp->getPiVars(), sp->getDdManager(), clo.maxBddSize, clo.approximationMethod);
    blif_solve_log(INFO, "Computed sub result");
    resultVec.insert(resultVec.end(), rv.cbegin(), rv.cend());
  }
  blif_solve_log(INFO, "Finished in " << blif_solve::duration(start) << " sec");
  

  // count number of solutions
  if (clo.mustCountNumSolutions)
  {
    start = blif_solve::now();
    auto numVars = bf.getNonPiVars()->size();
    auto conjoinedSolution = bdd_one(srt->ddm);
    for (auto result: resultVec)
      bdd_and_accumulate(srt->ddm, &conjoinedSolution, result);
    blif_solve_log(INFO, "Computed conjoined solution in " << blif_solve::duration(start) << " sec");
    start = blif_solve::now();
    auto numSolutions = bdd_count_minterm(srt->ddm, conjoinedSolution, numVars);
    blif_solve_log(INFO, "Counted " << numSolutions << " solutions in " << blif_solve::duration(start) << " sec");
  }

  // display final result
  if (blif_solve::getVerbosity() == blif_solve::DEBUG)
  {
    bdd_ptr finalResult = bdd_one(srt->ddm);
    for (auto r: resultVec)
      bdd_and_accumulate(srt->ddm, &finalResult, r);
    blif_solve_log_bdd(DEBUG, "finalResult", srt->ddm, finalResult);
    bdd_free(srt->ddm, finalResult);
  }

  // free results
  for (auto r: resultVec)
    bdd_free(srt->ddm, r);
  return 0;
}









///////////////////////////////////////////////////////
// functions to parse and store command line options //
///////////////////////////////////////////////////////
template<typename TValue>
std::shared_ptr<blif_solve::CommandLineOptionValue<TValue> >
  addCommandLineOption(std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > & commandLineOptions,
                       const std::string & name,
                       const std::string & help,
                       const TValue & defaultValue)
{
  auto result = blif_solve::CommandLineOptionValue<TValue>::create(name, help, defaultValue);
  commandLineOptions.push_back(result);
  return result;
}

//------------------------------------
VarScoreCommandLineOptions
  parseClo(int argc, char const * const * const argv)
{
  VarScoreCommandLineOptions result;
  std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > clo;
  auto verbosity = addCommandLineOption<std::string>(clo, "--verbosity", "verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)", "WARNING");
  auto blif = addCommandLineOption<std::string>(clo, "--blif", "blif file name", "");
  auto maxBddSize = addCommandLineOption<int>(clo, "--maxBddSize", "max bdd size allowed for exact computation", 100*1000*1000);
  auto approximationMethod = addCommandLineOption<std::string>(clo, "--approximationMethod", "approximation method (exact / early_quantification / factor_graph)", "exact");
  auto factorGraphMergeSize = addCommandLineOption<int>(clo, "--factorGraphMergeSize", "largest support set allowed the factor graph during merging", 1);
  auto mustCountNumSolutions = addCommandLineOption<bool>(clo, "--mustCountNumSolutions", "count and print the number of satisfying states", false);


  parseCommandLineOptions(argc - 1, argv + 1, clo);
  blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity->getValue()));
  if (blif->getValue() == "")
  {
    blif_solve_log(ERROR, "Expecting value for command line argument --blif");
    blif_solve::printHelp(clo);
    exit(1);
  }
  blif_solve_log(DEBUG, "blif file: " + blif->getValue());
  blif_solve_log(DEBUG, "verbosity: " + verbosity->getValue());
  blif_solve_log(DEBUG, "largest bdd size: " << maxBddSize->getValue());
  blif_solve_log(DEBUG, "approximation method: " << approximationMethod->getValue());
  blif_solve_log(DEBUG, "factor graph merge size: " << factorGraphMergeSize->getValue());
  blif_solve_log(DEBUG, "must count num solutions: " << mustCountNumSolutions->getValue());
  result.verbosity = verbosity->getValue();
  result.blif = blif->getValue();
  result.maxBddSize = maxBddSize->getValue();
  result.mustCountNumSolutions = mustCountNumSolutions->getValue();
  std::string am = approximationMethod->getValue();
  for (auto amit = am.begin(); amit != am.end(); ++amit)
    *amit = std::tolower(*amit);
  if (am == "exact")
      result.approximationMethod = var_score::ApproximationMethod::createExact();
  else if (am == "early_quantification")
    result.approximationMethod = var_score::ApproximationMethod::createEarlyQuantification();
  else if (am == "factor_graph")
    result.approximationMethod = var_score::ApproximationMethod::createFactorGraph(factorGraphMergeSize->getValue());
  else
    throw std::runtime_error("Could not recognise approximation method '" + approximationMethod->getValue() + "'. See --help.");
  return result;
}









