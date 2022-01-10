/*

Copyright 2021 Parakram Majumdar

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


#include <blif_solve_lib/clo.hpp>
#include <blif_solve_lib/log.h>

#include <dd/qdimacs.h>
#include <dd/qdimacs_to_bdd.h>

#include <factor_graph/factor_graph.h>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include <cudd.h>
#include <cuddInt.h>

#include <mustool/core/Master.h>


// struct to contain parse command line options
struct CommandLineOptions {
  int largestSupportSet;
  int maxMucSize;
  std::string inputFile;
};



struct DummyMucCallback: public MucCallback
{
    void processMuc(const std::vector<std::vector<int> >& muc) override
    {
      std::cout << "Callback found an MUC:\n";
      for(const auto & clause: muc) {
        for (int v: clause)
          std::cout << v << " ";
        std::cout << "\n";
      }
    }
};




// function declarations
CommandLineOptions parseClo(int argc, char const * const * const argv);
int main(int argc, char const * const * const argv);
std::shared_ptr<DdManager> ddm_init();
factor_graph * createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd);
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string & inputFilePath);
std::shared_ptr<Master> createMustMaster(const dd::Qdimacs& qdimacs);






// main
int main(int argc, char const * const * const argv)
{

  // parse command line options
  auto clo = parseClo(argc, argv);
  blif_solve_log(DEBUG, "Command line options parsed.");


  auto start = blif_solve::now();
  auto qdimacs = parseQdimacs(clo.inputFile);                           // parse input file
  auto ddm = ddm_init();                                                // init cudd
  auto bdds = dd::QdimacsToBdd::createFromQdimacs(ddm.get(), *qdimacs); // create bdds
  auto fg = createFactorGraph(ddm.get(), *bdds);                        // create factor graph
  blif_solve_log(INFO, "create factor graph from qdimacs file with "
      << qdimacs->numVariables << " variables and "
      << qdimacs->clauses.size() << " clauses in "
      << blif_solve::duration(start) << " sec");

  auto numIterations = factor_graph_converge(fg);                      // converge factor graph
  blif_solve_log(INFO, "Factor graph converged after " 
      << numIterations << " iterations in "
      << blif_solve::duration(start) << " secs");
  

  auto mustMaster = createMustMaster(*qdimacs);
  mustMaster->enumerate();

  // cleanup
  if (fg) factor_graph_delete(fg);
  return 0;
}





// parse qdimacs file
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string& inputFilePath)
{
  std::ifstream fin(inputFilePath);
  return dd::Qdimacs::parseQdimacs(fin);
}







// create factor graph
factor_graph* createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& bdds)
{
  std::vector<bdd_ptr> allFactors;
  allFactors.reserve(bdds.clauses.size());
  for (const auto & kv: bdds.clauses) allFactors.push_back(kv.second);
  return factor_graph_new(ddm, &allFactors.front(), allFactors.size());
}




// parse command line options
CommandLineOptions parseClo(int argc, char const * const * const argv)
{
  // set up the various options
  using blif_solve::CommandLineOption;
  auto largestSupportSet =
    std::make_shared<CommandLineOption<int> >(
        "--largestSupportSet",
        "largest allowed support set size while clumping cnf factors",
        false,
        50);
  auto maxMucSize =
    std::make_shared<CommandLineOption<int> >(
        "--maxMucSize",
        "max clauses allowed in an MUC",
        false,
        10);
  auto inputFile =
    std::make_shared<CommandLineOption<std::string> >(
        "--inputFile",
        "Input qdimacs file with exactly one quantifier which is existential",
        true);
  auto verbosity =
    std::make_shared<CommandLineOption<std::string> >(
        "--verbosity",
        "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)",
        false,
        std::string("ERROR"));
  
  
  // parse the command line
  blif_solve::parse(
      {  largestSupportSet, maxMucSize, inputFile, verbosity },
      argc,
      argv);

  
  // set log verbosity
  blif_solve::setVerbosity(blif_solve::parseVerbosity(*(verbosity->value)));

  // return the rest of the options
  return CommandLineOptions{
    *(largestSupportSet->value),
    *(maxMucSize->value),
    *(inputFile->value)
  };
}





// initialize cudd with some defaults
std::shared_ptr<DdManager> ddm_init()
{
  //UNIQUE_SLOTS = 256
  //CACHE_SLOTS = 262144
  std::shared_ptr<DdManager> m(Cudd_Init(0, 0, 256, 262144, 0));
  if (!m) throw std::runtime_error("Could not initialize cudd");
  return m;
}






// initialize must
std::shared_ptr<Master> createMustMaster(const dd::Qdimacs& qdimacs)
{
  // check that we have exactly one quantifier which happens to be existential
  assert(qdimacs.quantifiers.size() == 1);
  assert(qdimacs.quantifiers.front().quantifierType == dd::Quantifier::Exists);
  const auto & quantifiedVariablesVec = qdimacs.quantifiers.front().variables;
  std::set<int> quantifiedVariableSet(quantifiedVariablesVec.cbegin(), quantifiedVariablesVec.cend());
  auto isQuantifiedVariable = [&](int v)-> bool { return quantifiedVariableSet.count(v) + quantifiedVariableSet.count(-v) > 0; };

  // filter out clauses with at least one quantified variable
  // remove all the quantified variables
  std::vector<std::vector<int> > outputClauses;
  std::set<std::set<int> > uniqueOutputClauses;
  for (const auto & inputClause : qdimacs.clauses)
  {
    std::vector<int> outputClause;
    std::copy_if(inputClause.cbegin(), inputClause.cend(), std::back_inserter(outputClause), isQuantifiedVariable);
    std::set<int> uniqueOutputClause(outputClause.cbegin(), outputClause.cend());
    if (!outputClause.empty() && !uniqueOutputClauses.count(uniqueOutputClause))
    {
      uniqueOutputClauses.insert(uniqueOutputClause);
      outputClauses.push_back(outputClause);
    }
  }
  auto result = std::make_shared<Master>(qdimacs.numVariables, outputClauses, "remus");
  // result->verbose = ??;
  result->depthMUS = 6;
  result->dim_reduction = 0.9;
  result->validate_mus_c = true;
  result->satSolver->shrink_alg = "default";
  result->get_implies = true;
  result->criticals_rotation = false;
  auto mucCallback = std::make_shared<DummyMucCallback>();
  result->setMucCallback(mucCallback);
  return result;
}