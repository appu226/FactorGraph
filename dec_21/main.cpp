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

#include <factor_graph/fgpp.h>

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
    void processMuc(const std::vector<std::vector<int> >& muc) override;
};




// function declarations
CommandLineOptions parseClo(int argc, char const * const * const argv);
int main(int argc, char const * const * const argv);
std::shared_ptr<DdManager> ddm_init();
fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd);
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string & inputFilePath);
std::shared_ptr<Master> createMustMaster(const dd::Qdimacs& qdimacs);
bdd_ptr getFactorGraphResult(DdManager* ddm, const fgpp::FactorGraph& fg, const dd::QdimacsToBdd& qdimacsToBdd);
bdd_ptr getExactResult(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd);





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

  auto numIterations = fg->converge();                                  // converge factor graph
  blif_solve_log(INFO, "Factor graph converged after " 
      << numIterations << " iterations in "
      << blif_solve::duration(start) << " secs");
  

  auto factorGraphResult = getFactorGraphResult(ddm.get(), *fg, *bdds);
  auto exactResult = getExactResult(ddm.get(), *bdds);
  blif_solve_log(INFO, "factor graph result is " 
                      << (factorGraphResult == exactResult 
                          ? "exact" 
                          : "strictly over-approximate"));

  blif_solve_log_bdd(INFO, "factor graph:", ddm.get(), factorGraphResult);
  blif_solve_log_bdd(INFO, "exact:", ddm.get(), exactResult);

  auto mustMaster = createMustMaster(*qdimacs);
  mustMaster->enumerate();

  // cleanup
  if (factorGraphResult) bdd_free(ddm.get(), factorGraphResult);
  if (exactResult) bdd_free(ddm.get(), exactResult);

  blif_solve_log(INFO, "Done");
  return 0;
}





// parse qdimacs file
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string& inputFilePath)
{
  std::ifstream fin(inputFilePath);
  return dd::Qdimacs::parseQdimacs(fin);
}







// create factor graph
fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& bdds)
{
  std::vector<dd::BddWrapper> bddWrapperVec;
  for (const auto& factorKv: bdds.clauses)
    bddWrapperVec.push_back(dd::BddWrapper(bdd_dup(factorKv.second), ddm));
  return fgpp::FactorGraph::createFactorGraph(bddWrapperVec);
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
  auto isQuantifiedVariable = [&](int v)-> bool { return quantifiedVariableSet.count(v >=0 ? v : -v) > 0; };
  auto numMustVariables = qdimacs.numVariables; // numMustVariable mast mast, numMustVarible mast :)

  // create output clauses by:
  //   - filtering out clauses with no quantified variables
  //   - keeping only the quantified variables
  // create map from non-quantified literals to positions of clauses in the 'outputClauses' vector
  std::vector<std::vector<int> > outputClauses;
  typedef std::set<int> LiteralSet;
  std::set<LiteralSet> outputClauseSet;   // remember where each outputClause is stored
  std::map<int, std::set<size_t> > nonQuantifiedLiteralToOutputClausePosMap;
  for (const auto & clause: qdimacs.clauses)
  {
    LiteralSet quantifiedLiterals, nonQuantifiedLiterals;
    for (const auto literal: clause)
    {
      if (isQuantifiedVariable(literal)) quantifiedLiterals.insert(literal);
      else nonQuantifiedLiterals.insert(literal);
    }
    if (quantifiedLiterals.empty()) // skip if no quantified variables
      continue;
    size_t outputClausePos;  // find the position of this clause in 'outputClauses'
    if (outputClauseSet.count(quantifiedLiterals) == 0)
    {
      // if it's not already in 'outputClauses', then add it to the end
      outputClausePos = outputClauses.size();
      outputClauses.push_back(std::vector<int>(quantifiedLiterals.cbegin(), quantifiedLiterals.cend()));
      outputClauseSet.insert(quantifiedLiterals);
    }
    else
    {
      // if it's already present, add a new fake variable to the clause to make it unique
      ++numMustVariables;
      std::vector<int> nextOutputClause(quantifiedLiterals.cbegin(), quantifiedLiterals.cend());
      nextOutputClause.push_back(numMustVariables);
      outputClausePos = outputClauses.size();
      outputClauses.push_back(nextOutputClause);

      // also add a new clause that's the negative of the fake variable
      outputClauses.push_back(std::vector<int>(1, -numMustVariables));

      // no need to add these to the outputClauseSet, because they have a unique variable
    }
    for (const auto literal: nonQuantifiedLiterals)
      nonQuantifiedLiteralToOutputClausePosMap[literal].insert(outputClausePos);
  }

  // create the MUST Master using outputClauses
  auto result = std::make_shared<Master>(numMustVariables, outputClauses, "remus");
  // result->verbose = ??;
  result->depthMUS = 6;
  result->dim_reduction = 0.9;
  result->validate_mus_c = true;
  result->satSolver->shrink_alg = "default";
  result->get_implies = true;
  result->criticals_rotation = false;
  auto mucCallback = std::make_shared<DummyMucCallback>();
  result->setMucCallback(mucCallback);


  // find pairs of output clauses with opposite signs of a non-quantified variable
  // pass these pairs into the solver to indicate inconsistent sets of clauses
  std::set<std::pair<int, int> > inconsistentPairs;
  for (const auto & qvXcids: nonQuantifiedLiteralToOutputClausePosMap)
  {
    int qv = qvXcids.first;
    if (qv < 0) continue;  // skip half due to symmetry
    const auto& cids = qvXcids.second;
    const auto& opp_cids = nonQuantifiedLiteralToOutputClausePosMap[-qv];
    const auto printOutputClause = [&](int id) -> std::string {
      std::stringstream result;
      result << "{ ";
      for (const auto lit: outputClauses[id]) {
        result << lit << ", ";
      }
      result << "}";
      return result.str();
    };
    for (int cid: cids)
      for (int opp_cid: opp_cids)
      {
        int minId = (cid < opp_cid ? cid : opp_cid);
        int maxId = (cid > opp_cid ? cid : opp_cid);
        if (minId == maxId || inconsistentPairs.count(std::make_pair(minId, maxId)) > 0)
          continue;
        blif_solve_log(INFO, "marking inconsistent: " << printOutputClause(minId) << " " << printOutputClause(maxId)
          << " because of " << qv);
        result->explorer->mark_inconsistent_pair(minId, maxId);
        inconsistentPairs.insert(std::make_pair(minId, maxId));
      }

  }

  return result;
}




void DummyMucCallback::processMuc(const std::vector<std::vector<int> >& muc)
{
  std::cout << "Callback found an MUC:\n";
  for(const auto & clause: muc) {
    for (int v: clause)
      std::cout << v << " ";
    std::cout << "\n";
  }
}





bdd_ptr getFactorGraphResult(DdManager* ddm, const fgpp::FactorGraph& fg, const dd::QdimacsToBdd& q2b)
{
  auto qvars = dd::BddWrapper(q2b.quantifications[0]->quantifiedVariables, ddm);
  auto resultVec = fg.getIncomingMessages(qvars);
  dd::BddWrapper result(bdd_one(ddm), ddm);
  for (const auto & message: resultVec)
    result = result * message;
  return result.getCountedBdd();
}





bdd_ptr getExactResult(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd)
{
  bdd_ptr f = bdd_one(ddm);
  for(const auto & kv: qdimacsToBdd.clauses)
    bdd_and_accumulate(ddm, &f, kv.second);
  bdd_ptr result = bdd_forsome(ddm, f, qdimacsToBdd.quantifications[0]->quantifiedVariables);
  bdd_free(ddm, f);
  return result;
}