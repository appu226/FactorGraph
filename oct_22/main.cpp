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


#include <blif_solve_lib/approx_merge.h>
#include <blif_solve_lib/clo.hpp>
#include <blif_solve_lib/cnf_dump.h>
#include <blif_solve_lib/log.h>

#include <dd/qdimacs_to_bdd.h>

#include <factor_graph/fgpp.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cuddInt.h>

#include <mustool/core/Master.h>
#include <mustool/mcsmus/minisat/core/mcsmus_Solver.h>


// struct to contain parse command line options
struct CommandLineOptions {
  int largestSupportSet;
  std::string inputFile;
  bool computeExactUsingBdd;
  std::optional<std::string> outputFile;
  bool runMusTool;
};

struct Oct22MucCallback: public MucCallback
{
  typedef std::set<std::vector<int> > Cnf;
  typedef std::shared_ptr<Cnf> CnfPtr;
  typedef std::set<int> Clause;
  typedef std::set<int> Assignments;
  typedef std::map<Clause, Assignments> ClauseToAssignmentMap;
  typedef std::map<int, std::set<int> > AssignmentToClauseIndicesMap;

  Oct22MucCallback(const CnfPtr& factorGraphCnf);

  void processMuc(const std::vector<std::vector<int> >& muc) override;
  void addClause(const Clause& clause, int clauseIndex, const Assignments& assignments);
  void addFakeClause(int fakeVariable, int clauseIndex);

  void setMustMaster(const std::shared_ptr<Master>& mustMaster) {
    m_mustMaster = mustMaster;
  }
  
  private:
  CnfPtr m_factorGraphCnf;
  ClauseToAssignmentMap m_clauseToAssignmentMap;
  AssignmentToClauseIndicesMap m_assignmentToClauseIndicesMap;
  Minisat::Solver m_factorGraphResultSolver;
  std::weak_ptr<Master> m_mustMaster;
  std::set<std::set<int> > m_conflictSetCache;
};




// function declarations
CommandLineOptions parseClo(int argc, char const * const * const argv);
int main(int argc, char const * const * const argv);
std::shared_ptr<DdManager> ddm_init();
fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd, int largestSupportSet);
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string & inputFilePath);
std::shared_ptr<Master> createMustMaster(const dd::Qdimacs& qdimacs,
                                         const Oct22MucCallback::CnfPtr& factorGraphCnf);
std::vector<dd::BddWrapper> getFactorGraphResults(DdManager* ddm, const fgpp::FactorGraph& fg, const dd::QdimacsToBdd& qdimacsToBdd);
dd::BddWrapper getExactResult(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd);
Oct22MucCallback::CnfPtr convertToCnf(DdManager* ddm, 
                                      int numVariables, 
                                      const std::vector<dd::BddWrapper> & funcs);
void writeResult(const Oct22MucCallback::Cnf& cnf,
                 const dd::Qdimacs& qdimacs,
                 const std::string& outputFile);



template<typename TVecOfVecIt, typename TVec, typename TFunc>
void forAllCartesian(TVecOfVecIt start, TVecOfVecIt end, TVec& elements, TFunc func)
{
  if (start == end)
      func(elements);
  else {
    const auto & vec = **start;
    for (auto elem: vec)
    {
      elements.push_back(elem);
      forAllCartesian(start + 1, end, elements, func);
      elements.pop_back();
    }
  }
}


template<typename TSet>
std::string setToString(const TSet& set)
{
  std::stringstream ss;
  for (const auto & elem: set)
      ss << elem << " ";
  return ss.str();
}


// main
int main(int argc, char const * const * const argv)
{

  // parse command line options
  auto clo = parseClo(argc, argv);
  blif_solve_log(DEBUG, "Command line options parsed.");


  auto start = blif_solve::now();
  auto qdimacs = parseQdimacs(clo.inputFile);                           // parse input file
  blif_solve_log(INFO, "Parsed qdimacs file in with " 
                        << qdimacs->clauses.size() << " clauses and " 
                        << qdimacs->numVariables << " variables in "
                        << blif_solve::duration(start) << " sec");

  start = blif_solve::now();
  auto ddm = ddm_init();                                                // init cudd
  auto bdds = dd::QdimacsToBdd::createFromQdimacs(ddm.get(), *qdimacs); // create bdds
  blif_solve_log(INFO, "Created bdds in " << blif_solve::duration(start) << " sec");

  auto fg = createFactorGraph(ddm.get(), *bdds, clo.largestSupportSet); // merge factors and create factor graph

  start = blif_solve::now();
  auto numIterations = fg->converge();                                  // converge factor graph
  blif_solve_log(INFO, "Factor graph converged after " 
      << numIterations << " iterations in "
      << blif_solve::duration(start) << " secs");

  start = blif_solve::now();                                            // factor graph result to CNF
  auto factorGraphResults = getFactorGraphResults(ddm.get(), *fg, *bdds);
  auto factorGraphCnf = convertToCnf(ddm.get(), bdds->numVariables + (2 * bdds->clauses.size()), factorGraphResults);
  blif_solve_log(INFO, "Factor graph result converted to cnf in "
      << blif_solve::duration(start) << " secs");

  if (clo.runMusTool)                                                     // run mustool
  {
    start = blif_solve::now();
    auto mustMaster = createMustMaster(*qdimacs, factorGraphCnf);
    mustMaster->enumerate();
    blif_solve_log(INFO, "Must exploration finished in " << blif_solve::duration(start) << " sec");

    if (clo.outputFile.has_value())                                       // write results
      writeResult(*factorGraphCnf, *qdimacs, clo.outputFile.value());
    blif_solve_log(INFO, "Done");
  }
  return 0;
}





// parse qdimacs file
std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string& inputFilePath)
{
  std::ifstream fin(inputFilePath);
  return dd::Qdimacs::parseQdimacs(fin);
}







// create factor graph
fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& bdds, int largestSupportSet)
{
  auto start = blif_solve::now();
  std::vector<bdd_ptr> factors, variables;
  std::set<dd::BddWrapper> variableWrapperSet;
  std::set<dd::BddWrapper> quantifiedVariableWrapperSet;
  std::set<bdd_ptr> quantifiedVariableSet;

  // get factors and variables
  for (const auto & kv: bdds.clauses)
  {
      factors.push_back(kv.second);
      for (auto v: kv.first)
          variableWrapperSet.insert(bdds.getBdd(v > 0 ? v : -v));
  }
  for (const auto &v: variableWrapperSet) variables.push_back(v.getUncountedBdd());
  
  // get quantified variables
  assert(bdds.quantifications.size() == 1 && bdds.quantifications.front()->quantifierType == dd::Quantifier::Exists);
  dd::BddWrapper eqv(bdd_dup(bdds.quantifications.front()->quantifiedVariables), ddm);
  while(!eqv.isOne())
  {
    auto v = eqv.varWithLowestIndex();
    quantifiedVariableWrapperSet.insert(v);
    eqv = eqv.cubeDiff(v);
  }
  for(const auto &v: quantifiedVariableWrapperSet)
      quantifiedVariableSet.insert(v.getCountedBdd());
  
  // merge factors and variables
  auto mergeResults = blif_solve::merge(ddm, factors, variables, largestSupportSet, blif_solve::MergeHints(ddm), quantifiedVariableSet);
  blif_solve_log(INFO, "Merged to " 
                       << mergeResults.factors->size() << " factors and "
                       << mergeResults.variables->size() << "variables in "
                       << blif_solve::duration(start) << " sec");
  
  // create factor graph using merged factors
  start = blif_solve::now();
  std:vector<dd::BddWrapper> mergedFactorVec;
  for (auto p: *mergeResults.factors) mergedFactorVec.emplace_back(p, ddm);
  auto fg = fgpp::FactorGraph::createFactorGraph(mergedFactorVec);

  // group the variables in the factor graph
  for (auto mergedVariables: *mergeResults.variables)
      fg->groupVariables(dd::BddWrapper(mergedVariables, ddm));
  blif_solve_log(INFO, "Created factor graph in " << blif_solve::duration(start) << " sec");

  return fg;
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
  auto computeExactUsingBdd =
    std::make_shared<CommandLineOption<bool> >(
        "--computeExactUsingBdd",
        "Compute exact solution (default false)",
        false,
        false
    );
  auto outputFile =
    std::make_shared<CommandLineOption<std::string> >(
        "--outputFile",
        "Cnf file with result",
        false,
        std::optional<std::string>()
    );
  auto runMusTool =
    std::make_shared<CommandLineOption<bool> >(
        "--runMusTool",
        "Whether to run MUS tool (default true)",
        false,
        std::optional<bool>(true)
    );
  
  // parse the command line
  blif_solve::parse(
      {  largestSupportSet, inputFile, verbosity, computeExactUsingBdd, outputFile, runMusTool },
      argc,
      argv);

  
  // set log verbosity
  blif_solve::setVerbosity(blif_solve::parseVerbosity(*(verbosity->value)));

  // return the rest of the options
  return CommandLineOptions{
    *(largestSupportSet->value),
    *(inputFile->value),
    *(computeExactUsingBdd->value),
    outputFile->value,
    *(runMusTool->value)
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
std::shared_ptr<Master> createMustMaster(
  const dd::Qdimacs& qdimacs,
  const Oct22MucCallback::CnfPtr& factorGraphCnf)
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
  using Clause = Oct22MucCallback::Clause;
  std::vector<std::vector<int> > outputClauses;
  std::set<Clause> outputClauseSet;   // remember where each outputClause is stored
  std::map<int, std::set<size_t> > nonQuantifiedLiteralToOutputClausePosMap;
  auto mucCallback = std::make_shared<Oct22MucCallback>(factorGraphCnf);
  for (const auto & clause: qdimacs.clauses)
  {
    Clause quantifiedLiterals, reversedNonQuantifiedLiterals, nextOutputClause;
    for (const auto literal: clause)
    {
      if (isQuantifiedVariable(literal)) quantifiedLiterals.insert(literal);
      else reversedNonQuantifiedLiterals.insert(-literal);
    }
    if (quantifiedLiterals.empty()) // skip if no quantified variables
      continue;
    size_t outputClausePos;  // find the position of this clause in 'outputClauses'
    if (outputClauseSet.count(quantifiedLiterals) == 0)
    {
      // if it's not already in 'outputClauses', then add it to the end
      outputClausePos = outputClauses.size();
      mucCallback->addClause(quantifiedLiterals, outputClausePos, reversedNonQuantifiedLiterals);
      outputClauses.push_back(std::vector<int>(quantifiedLiterals.cbegin(), quantifiedLiterals.cend()));
      nextOutputClause = quantifiedLiterals;
      outputClauseSet.insert(quantifiedLiterals);
    }
    else
    {
      // if it's already present, add a new fake variable to the clause to make it unique
      ++numMustVariables;
      nextOutputClause = quantifiedLiterals;
      nextOutputClause.insert(numMustVariables);
      outputClausePos = outputClauses.size();
      mucCallback->addClause(nextOutputClause, outputClausePos, reversedNonQuantifiedLiterals);
      outputClauses.push_back(std::vector<int>(nextOutputClause.cbegin(), nextOutputClause.cend()));

      // also add a new clause that's the negative of the fake variable
      mucCallback->addFakeClause(-numMustVariables, outputClauses.size());
      outputClauses.push_back(std::vector<int>(1, -numMustVariables));

      // no need to add these to the outputClauseSet, because they have a unique variable
    }
    for (const auto literal: reversedNonQuantifiedLiterals)
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
  result->setMucCallback(mucCallback);
  mucCallback->setMustMaster(result);


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
        blif_solve_log(DEBUG, "marking inconsistent: " << printOutputClause(minId) << " " << printOutputClause(maxId)
          << " because of " << qv);
        result->explorer->mark_inconsistent_pair(minId, maxId);
        inconsistentPairs.insert(std::make_pair(minId, maxId));
      }

  }

  return result;
}




Oct22MucCallback::Oct22MucCallback(const CnfPtr& factorGraphCnf)
{
  m_factorGraphCnf = factorGraphCnf;

  int maxVar = 0;
  for (const auto & clause: *factorGraphCnf)
  {
    for(auto var: clause)
    {
      auto absvar = var > 0 ? var : -var;
      maxVar = maxVar > var ? maxVar : var;
    }
  }
  blif_solve_log(DEBUG, "Initializing sat solveer to " << maxVar << " variables");
  for (int i = 0; i <= maxVar; ++i)
    m_factorGraphResultSolver.newVar();
  for (const auto & clause: *factorGraphCnf)
  {
    Minisat::vec<Minisat::Lit> solverClause;
    solverClause.capacity(clause.size());
    for (auto var: clause)
    {
      solverClause.push(Minisat::mkLit(std::abs(var), var > 0));
    }
    blif_solve_log(DEBUG, "Adding clause " << setToString(solverClause) << " to sat solver");
    m_factorGraphResultSolver.addClause(solverClause);
  }
}

void Oct22MucCallback::processMuc(const std::vector<std::vector<int> >& muc)
{
  if (blif_solve::getVerbosity() >= blif_solve::DEBUG)
  {
    std::stringstream mucss;
    mucss << "Callback found an MUC:\n";
    for(const auto & clause: muc) {
      for (int v: clause)
        mucss << v << " ";
      mucss << "\n";
    }
    blif_solve_log(DEBUG, mucss.str());
  }
  auto master = m_mustMaster.lock();
  assert(master);
  std::set<int> assignments;
  for (const auto& mucClause: muc) {
    Clause clause(mucClause.cbegin(), mucClause.cend());
    auto camit = m_clauseToAssignmentMap.find(clause);
    assert(camit != m_clauseToAssignmentMap.end());
    auto mucAssignments = camit->second;
    for (auto mucAssignment: mucAssignments)
    {
      assert(assignments.count(-mucAssignment) == 0);
      assignments.insert(mucAssignment);
    }
  }
  Minisat::vec<Minisat::Lit> assumps;
  assumps.capacity(assignments.size());
  for (auto x: assignments)
    assumps.push(Minisat::mkLit(std::abs(x), x > 0));
  blif_solve_log(DEBUG, "Running sat solver with assumptions " << setToString(assumps));
  bool isSat = m_factorGraphResultSolver.solve(assumps);
  if (isSat)
  {
    Minisat::vec<Minisat::Lit> negAssump;
    negAssump.capacity(assignments.size());
    std::vector<int> negAssign;
    for (auto x: assignments)
    {
        negAssump.push(Minisat::mkLit(std::abs(x), x < 0));
        negAssign.push_back(-x);
    }
    
    blif_solve_log(DEBUG, "Adding clause " << setToString(negAssump) << " to sat solver");
    m_factorGraphResultSolver.addClause(negAssump);
    
    std::sort(negAssign.begin(), negAssign.end());
    blif_solve_log(INFO, "Adding clause " << setToString(negAssign) << " to solution");
    m_factorGraphCnf->insert(negAssign);
  }
  else
  {
    const auto & conflicts = m_factorGraphResultSolver.conflict;
    std::set<int> conflictSetCacheCheck;
    for (int i = 0; i < conflicts.size(); ++i)
        conflictSetCacheCheck.insert(conflicts[i].x);
    int numDisabled = 0;
    if (m_conflictSetCache.count(conflictSetCacheCheck) == 0)
    {
      m_conflictSetCache.insert(conflictSetCacheCheck);
      std::vector<std::set<int> const *> conflictClauses;
      for (int i = 0; i < conflicts.size(); ++i)
      {
        auto conflictLit = conflicts[i];
        int var = Minisat::var(conflictLit) * (Minisat::sign(conflictLit) ? 1 : -1);
        auto acimit = m_assignmentToClauseIndicesMap.find(var);
        assert(acimit != m_assignmentToClauseIndicesMap.cend());
        conflictClauses.push_back(&acimit->second);
      }
      auto disabler = [&master, &numDisabled](const std::vector<int> & clauseIndices) { 
        master->explorer->mark_inconsistent_set(clauseIndices);
        ++numDisabled;
      };
      std::vector<int> inconsistentIndices;
      forAllCartesian(conflictClauses.cbegin(), conflictClauses.cend(), inconsistentIndices, disabler);
    }
    blif_solve_log(INFO, "Disabled " << numDisabled << " sets from must solver.");
  }


 
}

void 
  Oct22MucCallback::addClause(
    const Clause& clause,
    int clauseIndex,
    const Assignments& assignments)
{
  if (blif_solve::getVerbosity() >= blif_solve::DEBUG)
  {
    std::stringstream dbgss;
    dbgss << "Inserting to m_clauseToAssignmentMap: ";
    for (auto l: clause)
      dbgss << l << ' ';
    blif_solve_log(DEBUG, dbgss.str());
  }
  m_clauseToAssignmentMap.insert(std::make_pair(clause, assignments));
  for (auto assignment: assignments)
    m_assignmentToClauseIndicesMap[assignment].insert(clauseIndex);
}



void Oct22MucCallback::addFakeClause(
  int fakeVariable,
  int clauseIndex
) {
  Clause fakeClause;
  fakeClause.insert(fakeVariable);
  m_clauseToAssignmentMap.insert(std::make_pair(fakeClause, Assignments()));
}



std::vector<dd::BddWrapper> getFactorGraphResults(DdManager* ddm, const fgpp::FactorGraph& fg, const dd::QdimacsToBdd& q2b)
{
  dd::BddWrapper allVars(bdd_one(ddm), ddm);
  auto qvars = dd::BddWrapper(bdd_dup(q2b.quantifications[0]->quantifiedVariables), ddm);
  for (int i = 1; i <= q2b.numVariables; ++i)
  {
    dd::BddWrapper nextVar(bdd_new_var_with_index(ddm, i), ddm);
    allVars = allVars.cubeUnion(nextVar);
  }
  auto nonQVars = allVars.cubeDiff(qvars);
  return fg.getIncomingMessages(nonQVars);
}





dd::BddWrapper getExactResult(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd)
{
  bdd_ptr f = bdd_one(ddm);
  for(const auto & kv: qdimacsToBdd.clauses)
    bdd_and_accumulate(ddm, &f, kv.second);
  bdd_ptr result = bdd_forsome(ddm, f, qdimacsToBdd.quantifications[0]->quantifiedVariables);
  bdd_free(ddm, f);
  return dd::BddWrapper(result, ddm);
}


Oct22MucCallback::CnfPtr
    convertToCnf(DdManager* ddm,
                 int numVariables,
                 const std::vector<dd::BddWrapper> & funcs)
{
  auto result = std::make_shared<std::set<std::vector<int> > >();
  std::set<bdd_ptr> funcSet;
  for (const auto & func: funcs)
      funcSet.insert(func.getUncountedBdd());
  blif_solve::dumpCnf(ddm, numVariables, funcSet, *result);
  return result;
}


void writeResult(const Oct22MucCallback::Cnf& cnf, 
                 const dd::Qdimacs& qdimacs,
                 const std::string& outputFile)
{
  auto start = blif_solve::now();
  // prepare output stream
  std::unique_ptr<std::ostream> outUPtr;
  std::ostream* outRPtr;
  if (outputFile == "stdout")
  {
    outRPtr = &std::cout;
  } else
  {
    outUPtr.reset(new std::ofstream(outputFile));
    outRPtr = outUPtr.get();
  }
  std::ostream& out = *outRPtr;

  // get independent variabels
  std::set<int> indVar;
  for (int i = 1; i <= qdimacs.numVariables; ++i)
      indVar.insert(i);
  for (const auto & quantifier: qdimacs.quantifiers)
      for (auto var: quantifier.variables)
          indVar.erase(var);
  out << "c ind ";
  for (auto var: indVar)
      out << var << ' ';
  out << "0\n";


  // get number of vars
  int maxVar = 0;
  for (const auto & clause: cnf)
      for (auto var: clause)
          maxVar = std::max(maxVar, std::abs(var));
  out << "p cnf " << maxVar << ' ' << cnf.size() << '\n';

  // print clauses
  for (const auto & clause: cnf)
  {
    for (auto var: clause)
        out << var << ' ';
    out << "0\n";
  }
  out << std::endl;
  blif_solve_log(INFO, "Wrote result to " << outputFile 
                       << " in " << blif_solve::duration(start) << " sec.");
}

