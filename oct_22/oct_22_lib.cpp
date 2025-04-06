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

#include "oct_22_lib.h"
#include "approx_var_elim.h"
#include <mustool/core/Master.h>
#include <mustool/mcsmus/minisat/core/mcsmus_Solver.h>
#include <unordered_set>
#include <unordered_map>

namespace {
  void disableClauseHelper(
    std::vector<int>& clauseIndicesToBeDisabled,
    const std::set<int>& clauseIndicesToSkip,
    std::vector<std::set<int> const *>::const_iterator clauseIndicesPerAssignmentBeginIt,
    std::vector<std::set<int> const *>::const_iterator clauseIndicesPerAssignmentEndIt,
    Explorer* explorer,
    int& disableCount
  ) {
    if (clauseIndicesPerAssignmentBeginIt == clauseIndicesPerAssignmentEndIt)
    {
      explorer->mark_inconsistent_set(clauseIndicesToBeDisabled);
      ++disableCount;
      return;
    }
    const auto& clauseIndicesForNextLiteral = **clauseIndicesPerAssignmentBeginIt;
    ++clauseIndicesPerAssignmentBeginIt;
    std::set<int> newClauseIndicesToSkip = clauseIndicesToSkip;
    newClauseIndicesToSkip.insert(clauseIndicesForNextLiteral.cbegin(), clauseIndicesForNextLiteral.cend());
    bool areAllClausesSkipped = true;
    for (auto ci: clauseIndicesForNextLiteral)
    {
      if (clauseIndicesToSkip.count(ci) == 0) // NOT to be skipped
      {
        areAllClausesSkipped = false;
        clauseIndicesToBeDisabled.push_back(ci);
        disableClauseHelper(
          clauseIndicesToBeDisabled, 
          newClauseIndicesToSkip, 
          clauseIndicesPerAssignmentBeginIt, 
          clauseIndicesPerAssignmentEndIt,
          explorer,
          disableCount);
        clauseIndicesToBeDisabled.pop_back();
      }
    }
    if (areAllClausesSkipped)
    {
      disableClauseHelper(
        clauseIndicesToBeDisabled, 
        clauseIndicesToSkip, 
        clauseIndicesPerAssignmentBeginIt, 
        clauseIndicesPerAssignmentEndIt, 
        explorer,
        disableCount);
    }
  }

  void disableClause(
    const std::vector<std::set<int> const *> & clauseIndicesPerAssignment,
    Explorer* explorer,
    int& disableCount
  ) {
    std::vector<int> clauseIndicesToBeDisabled;
    std::set<int> clauseIndicesToSkip;
    disableClauseHelper(
      clauseIndicesToBeDisabled, 
      clauseIndicesToSkip, 
      clauseIndicesPerAssignment.cbegin(),
      clauseIndicesPerAssignment.cend(),
      explorer,
      disableCount);
  }
}


namespace oct_22 {

  std::optional<std::string> CommandLineOptions::musResultFile() const
  {
    if (outputFile.has_value())
      return outputFile.value() + ".mus";
    else
      return std::nullopt;
  }

  Oct22MucCallback::Oct22MucCallback(
    const CnfPtr& factorGraphCnf, 
    int numMustVariables,
    bool mustMinimalizeAssignments,
    std::optional<std::string> const& musResultFile)
  {
    m_numMustVariables = numMustVariables;
    m_mustMinimalizeAssignments = mustMinimalizeAssignments;
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
    m_explorationStartTime = blif_solve::now();
    for (int i = 0; i <= numMustVariables; ++i)
      m_minimalizationSolver.newVar();

    if (musResultFile.has_value())
    {
      m_musResultFile.emplace(musResultFile.value());
      if (!m_musResultFile->is_open())
      {
        blif_solve_log(ERROR, "Could not open file " << musResultFile.value() << " for writing");
        throw std::runtime_error("Could not open file " + musResultFile.value() + " for writing");
      }
    }
  }
  
  void Oct22MucCallback::processMuc(const std::vector<std::vector<int> >& muc)
  {
    blif_solve_log(INFO, "MUS exploration finished in " << blif_solve::duration(m_explorationStartTime) << " sec");
    auto processStart = blif_solve::now();
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
    Assignments assignments;
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
    if (m_mustMinimalizeAssignments)
      assignments = minimalizeAssignments(assignments);
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
      if (m_musResultFile.has_value())
      {
        (*m_musResultFile) << setToString(negAssign) << " 0" << std::endl;
      }
      blif_solve_log(INFO, "Adding clause " << setToString(negAssign) << " to solution");
      m_factorGraphCnf->insert(negAssign);
    }
    else
    {
      const auto & conflicts = m_factorGraphResultSolver.conflict;
      int numDisabled = 0;
      std::vector<std::set<int> const *> conflictClauses;
      for (int i = 0; i < conflicts.size(); ++i)
      {
        auto conflictLit = conflicts[i];
        int var = Minisat::var(conflictLit) * (Minisat::sign(conflictLit) ? -1 : 1);
        auto acimit = m_assignmentToClauseIndicesMap.find(var);
        assert(acimit != m_assignmentToClauseIndicesMap.cend());
        conflictClauses.push_back(&acimit->second);
      }

      disableClause(conflictClauses, master->explorer, numDisabled);
      // auto disabler = [&master, &numDisabled](const std::vector<int> & clauseIndices) { 
      //   master->explorer->mark_inconsistent_set(clauseIndices);
      //   ++numDisabled;
      // };
      // std::vector<int> inconsistentIndices;
      // forAllCartesian(conflictClauses.cbegin(), conflictClauses.cend(), inconsistentIndices, disabler);
      blif_solve_log(INFO, "Disabled " << numDisabled << " sets from must solver.");
    }
    blif_solve_log(INFO, "MUC processing finished in " << blif_solve::duration(processStart) << " sec");
    m_explorationStartTime = blif_solve::now();
  }
  
  void 
    Oct22MucCallback::addClause(
      int markerVariable,
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
    {
      m_assignmentToClauseIndicesMap[assignment].insert(clauseIndex);
      m_assignmentToMarkerPositionsMap[assignment].insert(m_assumptionsWithAllMarkersFalse.size());
    }
    m_assumptionsWithAllMarkersFalse.push(Minisat::mkLit(std::abs(markerVariable), false));
    
    auto solverMarkerVar = m_minimalizationSolver.newVar();
    assert(solverMarkerVar == markerVariable);
    Assumptions minimalizationSolverClause;
    minimalizationSolverClause.capacity(clause.size());
    for (auto lit: clause)
      minimalizationSolverClause.push(Minisat::mkLit(std::abs(lit), lit > 0));
    m_minimalizationSolver.addClause(minimalizationSolverClause);
  }
  
  
  
  void Oct22MucCallback::addFakeClause(
    int fakeVariable,
    int clauseIndex
  ) {
    Clause fakeClause;
    fakeClause.insert(fakeVariable);
    m_clauseToAssignmentMap.insert(std::make_pair(fakeClause, Assignments()));
  }


  Oct22MucCallback::Assumptions Oct22MucCallback::createAssumptionsWithAllMarkersFalse() const
  {
    return m_assumptionsWithAllMarkersFalse;
  }

  void Oct22MucCallback::disableAllMissingAssignments(Assumptions& assumptions, const Assignments& assignments)
  {
    for(int lit = -(m_numMustVariables - 1); lit < m_numMustVariables; ++lit)
    {
      if (lit == 0 || assignments.count(lit) > 0)
        continue;
      auto mpIt = m_assignmentToMarkerPositionsMap.find(lit);
      if (mpIt != m_assignmentToMarkerPositionsMap.end())
      {
        for (const auto mp: mpIt->second)
        {
          auto oldLit = assumptions[mp];
          assumptions[mp] = Minisat::mkLit(Minisat::var(oldLit), true);
        }
      }
    }
  }


  Assignments Oct22MucCallback::minimalizeAssignments(const Assignments& inputAssignments)
  {
    // initialize solver

    // initialize assumptions with all marker variables set to true
    Assumptions assumptions = createAssumptionsWithAllMarkersFalse();
    disableAllMissingAssignments(assumptions, inputAssignments);

    Assignments result = inputAssignments;
    bool continueMinimalization = true; // assume that the assignments contain an MUC
    while(continueMinimalization)
    {
      continueMinimalization = false;
      for (auto ir = result.cbegin(), nextir = result.cbegin(); !continueMinimalization && ir != result.cend(); ir = nextir)
      {
        ++nextir;
        auto assignmentToCheck = *ir;
        auto nextAssumptions = assumptions;
        auto a2mvit = m_assignmentToMarkerPositionsMap.find(assignmentToCheck);
        if (a2mvit != m_assignmentToMarkerPositionsMap.end())
        {
          for (auto markerPos: a2mvit->second)
          {
            auto oldLit = nextAssumptions[static_cast<int>(markerPos)];
            nextAssumptions[static_cast<int>(markerPos)] = Minisat::mkLit(Minisat::var(oldLit), true);
          }
          bool isSat = m_minimalizationSolver.solve(nextAssumptions);
          if (!isSat)
            continueMinimalization = true;
        }
        else
          continueMinimalization = true;
        if (continueMinimalization)
        {
          assumptions = nextAssumptions;
          result.erase(ir);
        }
      }
    }
    return result;
  }



  bdd_ptr cnfToBdd(const dd::QdimacsToBdd& qdimacsToBdd, const Oct22MucCallback::Cnf& cnf)
  {
    auto ddm = qdimacsToBdd.ddManager;
    auto one = dd::BddWrapper(bdd_one(ddm), ddm);
    auto zero = dd::BddWrapper(bdd_zero(ddm), ddm);
    auto result = one;
    auto tseytinVarCube = one;
    for (const auto & cnfClause: cnf)
    {
      auto resultClause = zero;
      for (auto cnfLiteral: cnfClause)
      {
        resultClause = resultClause + qdimacsToBdd.getBdd(cnfLiteral);
        auto cnfVar = (cnfLiteral < 0 ? -cnfLiteral : cnfLiteral);
        if (cnfVar > qdimacsToBdd.numVariables)
          tseytinVarCube = tseytinVarCube.cubeUnion(qdimacsToBdd.getBdd(cnfVar));
      }
      result = result * resultClause;
    }
    if (tseytinVarCube != one)
        result = result.existentialQuantification(tseytinVarCube);
    return result.getCountedBdd();
  }
  
  
  // parse qdimacs file
  std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string& inputFilePath)
  {
    std::ifstream fin(inputFilePath);
    return dd::Qdimacs::parseQdimacs(fin);
  }
  
  
  
  
  
  
  
  // create factor graph
  fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& bdds, int largestSupportSet, int largestBddSize)
  {
    auto start = blif_solve::now();
    std::vector<bdd_ptr> factors, variables;
    std::vector<std::string> factorNames, variableNames;
    std::set<std::pair<int, dd::BddWrapper> > variableWrapperSet;
    std::set<dd::BddWrapper> quantifiedVariableWrapperSet;
    std::set<bdd_ptr> quantifiedVariableSet;
  
    // get factors and variables
    for (const auto & kv: bdds.clauses)
    {
        factors.push_back(kv.second);
        std::stringstream factorNameSs;
        for (auto v: kv.first)
        {
            auto av = v > 0 ? v : -v;
            variableWrapperSet.insert(std::make_pair(av, bdds.getBdd(av)));
            factorNameSs << v << " OR ";
        }
        factorNames.push_back(factorNameSs.str());
        if (factorNames.back().size() >= 3)
            factorNames.back() = factorNames.back().substr(0, factorNames.back().size() - 3);
    }
    for (const auto &v: variableWrapperSet) 
    {
        variables.push_back(v.second.getUncountedBdd());
        variableNames.push_back(std::to_string(v.first));
    }
    
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
    auto mergeResults = blif_solve::merge(ddm, factors, variables, largestSupportSet, largestBddSize, blif_solve::MergeHints(ddm), quantifiedVariableSet, factorNames, variableNames);
    blif_solve_log(INFO, "Merged to " 
                         << mergeResults.factors->size() << " factors and "
                         << mergeResults.variables->size() << "variables in "
                         << blif_solve::duration(start) << " sec");
#ifdef DEBUG_MERGE
    blif_solve_log(DEBUG, "====================== Merged factor nodes ==================== ");
    for (const auto & fn: *mergeResults.factorNames)
      blif_solve_log(DEBUG, fn);
    blif_solve_log(DEBUG, "===================== Merged variable nodes =================== ");
    for (const auto & vn: *mergeResults.variableNames)
      blif_solve_log(DEBUG, vn);
    blif_solve_log(DEBUG, "=============================================================== ");
#endif
    
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
    auto largestBddSize =
      std::make_shared<CommandLineOption<int> >(
        "--largestBddSize",
        "largest allowed bdd size while clumping cnf factors",
        false,
        1000*1000*1000
      );
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
    auto runFg =
      std::make_shared<CommandLineOption<bool> >(
          "--runFg",
          "Whether to run factor graph (default true)",
          false,
          std::optional<bool>(true)
      );
    auto minimalizeAssignments =
      std::make_shared<CommandLineOption<bool> >(
        "--minimalizeAssignments",
        "Whether to minimalize assignments found by must",
        false,
        std::optional<bool>(true)
      );
    
    // parse the command line
    blif_solve::parse(
        {  largestSupportSet, largestBddSize, inputFile, verbosity, 
           computeExactUsingBdd, outputFile, runMusTool, runFg,
           minimalizeAssignments },
        argc,
        argv);
  
    
    // set log verbosity
    blif_solve::setVerbosity(blif_solve::parseVerbosity(*(verbosity->value)));
  
    // return the rest of the options
    return CommandLineOptions{
      *(largestSupportSet->value),
      *(largestBddSize->value),
      *(inputFile->value),
      *(computeExactUsingBdd->value),
      outputFile->value,
      *(runMusTool->value),
      *(runFg->value),
      *(minimalizeAssignments->value)
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
    const Oct22MucCallback::CnfPtr& factorGraphCnf,
    bool mustMinimalizeAssignments,
    std::optional<std::string> const& musResultFile)
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
    auto mucCallback = std::make_shared<Oct22MucCallback>(
      factorGraphCnf, 
      numMustVariables,
      mustMinimalizeAssignments,
      musResultFile);
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
      
      // add a new fake variable to the clause to make it unique
      ++numMustVariables;
      nextOutputClause = quantifiedLiterals;
      nextOutputClause.insert(numMustVariables);
      outputClausePos = outputClauses.size();
      mucCallback->addClause(numMustVariables, nextOutputClause, outputClausePos, reversedNonQuantifiedLiterals);
      outputClauses.push_back(std::vector<int>(nextOutputClause.cbegin(), nextOutputClause.cend()));

      // also add a new clause that's the negative of the fake variable
      mucCallback->addFakeClause(-numMustVariables, outputClauses.size());
      outputClauses.push_back(std::vector<int>(1, -numMustVariables));
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
    result->exit_if_satisfiable = false;
  
  
    // find pairs of output clauses with opposite signs of a non-quantified variable
    // pass these pairs into the solver to indicate inconsistent sets of clauses
    std::set<std::pair<int, int> > inconsistentPairs;
    const auto& nonQuantifiedLiteralToOutputClausePosMap = mucCallback->getAssignmentToClauseIndicesMap();
    for (const auto & qvXcids: nonQuantifiedLiteralToOutputClausePosMap)
    {
      int qv = qvXcids.first;
      if (qv < 0) continue;  // skip half due to symmetry
      const auto& cids = qvXcids.second;
      auto opp_cids_it = nonQuantifiedLiteralToOutputClausePosMap.find(-qv);
      if (opp_cids_it != nonQuantifiedLiteralToOutputClausePosMap.cend())
      {
        const auto& opp_cids = opp_cids_it->second;
        const auto printOutputClause = [&](int id) -> std::string {
          std::stringstream ss;
          ss << "{ ";
          for (const auto lit: outputClauses[id]) {
            ss << lit << ", ";
          }
          ss << "}";
          return ss.str();
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
  
    }
  
    return result;
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


  Oct22MucCallback::CnfPtr
      approxVarElim(
        const dd::Qdimacs& qdimacs
      )
  {
    // convert to factor graph data structure
    auto ave = ApproxVarElim::parseQdimacs(qdimacs);
    if (qdimacs.quantifiers.size() != 1 || qdimacs.quantifiers.front().quantifierType != dd::Quantifier::Exists)
    {
      throw std::runtime_error("Only qdimacs with exactly one existential quantifier is supported for now.");
    }

    // approximately remove all variables
    auto const& qvars = qdimacs.quantifiers.front().variables;
    for (int x: qvars)
    {
      if (x == 0)
        continue;
      ave->approximatelyEliminateVar(x);
    }
    
    // convert to cnf
    auto resultCnf = std::make_shared<Oct22MucCallback::Cnf>();
    for (auto const& clause: ave->getClauses())
    {
      std::vector<int> convertedClause;
      convertedClause.reserve(clause->vars.size());
      for (auto const& var: clause->vars)
        convertedClause.push_back(var.lock()->literal);
      resultCnf->insert(convertedClause);
    }
    return resultCnf;
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
  
  bdd_ptr computeExact(const dd::QdimacsToBdd& bdds)
  {
    auto startTime = blif_solve::now();
    blif_solve_log(INFO, "Computing exact result using Bdds");
    auto ddm = bdds.ddManager;
    dd::BddWrapper conjunction(bdd_one(ddm), ddm);


    std::unordered_map<int, std::unordered_set<bdd_ptr>> varToClauseMap;
    std::unordered_set<bdd_ptr> alreadyConjoinedClauses;
    for (const auto & clause: bdds.clauses)
    {
      for (auto lit : clause.first)
          varToClauseMap[std::abs(lit)].insert(clause.second);
    }
    if (bdds.quantifications.size() != 1 || bdds.quantifications[0]->quantifierType != dd::Quantifier::Exists)
      throw std::runtime_error("Expecting exactly one quantifier in qdimacs, which has to be Existential.");
    const auto & quantifiedVarIndices = bdds.quantifications[0]->quantifiedVarIndices;
    for (const auto varIndex: quantifiedVarIndices)
    {
      for (const auto quantifiedClause: varToClauseMap[varIndex])
      {
        if (alreadyConjoinedClauses.count(quantifiedClause) > 0)
          continue;
        alreadyConjoinedClauses.insert(quantifiedClause);
        blif_solve_log(DEBUG, "Conjoining clause");
        conjunction = conjunction * dd::BddWrapper(bdd_dup(quantifiedClause), ddm);
      }
      blif_solve_log(DEBUG, "Eliminating variable " << varIndex);
      conjunction = conjunction.existentialQuantification(dd::BddWrapper(bdd_new_var_with_index(ddm, varIndex), ddm));
    }

    blif_solve_log(INFO, "Computed exact result in " << blif_solve::duration(startTime) << " sec");
    return conjunction.getCountedBdd();
  }

  class VerbosityCapture {
    public:
      VerbosityCapture(blif_solve::Verbosity targetVerbosity)
        : m_oldVerbosity(blif_solve::getVerbosity())
      {
        blif_solve::setVerbosity(targetVerbosity);
      }

      ~VerbosityCapture()
      {
        blif_solve::setVerbosity(m_oldVerbosity);
      }
    private:
      blif_solve::Verbosity m_oldVerbosity;
  };

  void test(DdManager* manager)
  {
    VerbosityCapture vc(blif_solve::QUIET);
    dd::Qdimacs qdimacs{2, {{dd::Quantifier::Exists, {2}}}, {{-1, 2}, {1, -2}, {-1, -2}}};
    auto bdds = dd::QdimacsToBdd::createFromQdimacs(manager, qdimacs);
    auto fg = createFactorGraph(manager, *bdds, 1, 1);
    auto numIterations = fg->converge();
    auto factorGraphResults = getFactorGraphResults(manager, *fg, *bdds);
    auto factorGraphCnf = convertToCnf(manager, bdds->numVariables + (2 * bdds->clauses.size()), factorGraphResults);

    auto mustMaster = createMustMaster(qdimacs, factorGraphCnf, true, std::nullopt);
    mustMaster->enumerate();

    auto exactResult = computeExact(*bdds);
    auto fgMustResult = cnfToBdd(*bdds, *factorGraphCnf);
    assert(exactResult == fgMustResult);
    bdd_free(manager, fgMustResult);
    bdd_free(manager, exactResult);
  }


} // end namespace oct_22