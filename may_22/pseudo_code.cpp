#include <map>
#include <set>
#include <stdexcept>
#include <optional>

std::runtime_error NYI() { return std::runtime_error("Not yet implemented"); }

struct FactorGraph {};
struct Factor {};
struct Weights {};
struct Bdd {};
struct MustSolver {};
using Variable = int;
using Literal = int;
using Clause = std::set<Literal>;
using ClauseSet = std::set<Clause>;
using VariableSet = std::set<Variable>;
using BddSet = std::set<Bdd>;
using ClauseMap = std::map<Clause, Clause>;

FactorGraph createFactorGraph(const ClauseSet& phi)         { throw NYI(); }
Weights createDefaultWeights(const FactorGraph& fg, 
                             int lambda)                    { throw NYI(); }

// merge nodes in a factor graph fg
//  based on weights w, 
//  except
//    - if largest support set goes above lambda
//    - never merge X and non-X variables
FactorGraph mergeNodes(const FactorGraph& fg,
                       const Weights& w,
                       const VariableSet& X,
                       int lambda)                          { throw NYI(); }
FactorGraph addFactor(const FactorGraph& fg,
                      const Bdd& factor)                    { throw NYI(); }

// run the factor graph algorithm
BddSet convergeFactorGraph(const FactorGraph& fg,
                           const VariableSet& X)            { throw NYI(); }

// compute the union of two bdd sets
// optionally:
//   for any two elems a, b
//     such that a => b
//     throw away b
BddSet bddSetUnion(const BddSet& left,
                   const BddSet& right)                     { throw NYI(); }
BddSet bddSetInsert(const BddSet& bddSet,
                    const Bdd& newBdd)                      { throw NYI(); }

int    bddSupportSetSize(const Bdd& bdd)                    { throw NYI(); }
Bdd bddNot(const Bdd& f)                                    { throw NYI(); }

// remove clauses which do not have any X variables
// this ensures that the call to filterLiterals never
//   results in empty clauses
ClauseSet filterClauses(const ClauseSet& C,
                        const VariableSet& X)               { throw NYI(); }

// remove non-X variables from all input clauses
// if any clauses become duplicate,
//    add a fake unique variable to one of them
//    to make it unique
// output variables:
//   - orig: mapping from output clauses to input clauses
//   - assignment: mapping from output clauses to set of literals
//        on non-X variables that would have converted
//        the corresponding orig clause to the output clause
ClauseSet filterLiterals(ClauseMap& orig,
                         ClauseMap& assignment,
                         const ClauseSet& phi,
                         const VariableSet& X)              { throw NYI(); }

// union all the assignments of all sets in muc
// create a bdd representing the full assignment
Bdd getAssignmentFromMuc(const ClauseSet& muc,
                         const ClauseMap& assignment)       { throw NYI(); }

// check whether too much time has passed and therefore the algo should be terminated
bool mustTerminate()                                        { throw NYI(); }


MustSolver createMustSolver(const ClauseSet& C)             { throw NYI(); }

// tell the mustSolver to never explore a set
//   with both these clauses
void disableExplorationOfClausePair(const MustSolver& mustSolver,
                                    const Clause& clause1,
                                    const Clause& clause2)  { throw NYI(); }

// disable exploration of all clause pairs with inconsistent assignments
void disableInconsistentAssignments(const MustSolver& mustSolver,
                                    const ClauseMap& assignments)
{
  std::map<Literal, ClauseSet> literalToClauseMap;
  for (const auto & asit: assignments) {
    const auto & clause = asit.first;
    const auto & assignments = asit.second;
    for (auto literal: assignments) {
      literalToClauseMap[literal].insert(clause);
      auto oppositeLiteral = -literal;
      const auto& conflictingClauseSet = literalToClauseMap[oppositeLiteral];
      for (const auto & conflictingClause: conflictingClauseSet) {
        disableExplorationOfClausePair(mustSolver, clause, conflictingClause);
      }
    }
  }
}

// tell mustSolver to generate the next MUC
std::optional<ClauseSet> findNextMuc(MustSolver& mustSolver){ throw NYI(); }

// tell mustSolver to use the factorGraphResult to guide how to reduce seeds
//   by prioritizing the removal of those clauses 
//     whose intersection with the factor graph result is the small
bool setMustReductionGuide(const MustSolver& mustSolver,
                           const BddSet& factorGraphResult) { throw NYI(); }
Weights updateWeights(const ClauseSet& C,
                      const Weights& w,
                      const ClauseSet& muc)                 { throw NYI(); }


BddSet may_22(const ClauseSet& C, const VariableSet& X, int lambda) {
  auto factorGraph = createFactorGraph( C );
  auto weights = createDefaultWeights(factorGraph, lambda);
  auto phi = filterClauses(C, X);
  ClauseMap orig, assignment;
  auto psi = filterLiterals(orig, assignment, phi, X);
  auto mustSolver = createMustSolver(psi);
  disableInconsistentAssignments(mustSolver, assignment);
  BddSet factorGraphResult;
  do {
    auto mergedFactorGraph = mergeNodes(factorGraph, weights, X, lambda);
    factorGraphResult = bddSetUnion(factorGraphResult, 
                                    convergeFactorGraph(factorGraph, X));
    setMustReductionGuide(mustSolver, factorGraphResult);
    auto optMuc = findNextMuc(mustSolver);
    if (!optMuc) break;
    auto mucAssignment = getAssignmentFromMuc(*optMuc, assignment);
    auto newFactor = bddNot(mucAssignment);
    factorGraphResult = bddSetInsert(factorGraphResult, newFactor);
    if (bddSupportSetSize(mucAssignment) <= lambda) {
      factorGraph = addFactor(factorGraph, newFactor);
    }
    weights = updateWeights(C, weights, *optMuc);
  } while(!mustTerminate());
  return factorGraphResult;
}



int main()                                                  { throw NYI(); }
