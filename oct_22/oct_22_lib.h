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

#pragma once

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

namespace oct_22 {
    struct CommandLineOptions {
        int largestSupportSet;
        int largestBddSize;
        std::string inputFile;
        bool computeExactUsingBdd;
        std::optional<std::string> outputFile;
        bool runMusTool;
        bool runFg;
        bool mustMinimalizeAssignments;
        std::optional<std::string> musResultFile() const;
    };


    typedef std::set<int> Clause;
    typedef std::set<int> Assignments;

    struct Oct22MucCallback: public MucCallback
    {
      typedef std::set<std::vector<int> > Cnf;
      typedef std::shared_ptr<Cnf> CnfPtr;
      typedef std::map<Clause, Assignments> ClauseToAssignmentMap;
      typedef std::map<Clause, int>  ClauseToMarkerVariableMap;
      typedef std::map<int, std::set<int> > AssignmentToClauseIndicesMap;
      typedef std::map<int, std::set<size_t> > AssignmentToMarkerPositionsMap;
      typedef Minisat::vec<Minisat::Lit> Assumptions;
      Oct22MucCallback(
        const CnfPtr& factorGraphCnf, 
        int numMustVariables, 
        bool mustMinimalizeAssignments,
        std::optional<std::string> const& musResultFile);
    
      void processMuc(const std::vector<std::vector<int> >& muc) override;
      void addClause(int markerVariable, const Clause& clause, int clauseIndex, const Assignments& assignments);
      void addFakeClause(int fakeVariable, int clauseIndex);
    
      void setMustMaster(const std::shared_ptr<Master>& mustMaster) {
        m_mustMaster = mustMaster;
      }

      const AssignmentToClauseIndicesMap& getAssignmentToClauseIndicesMap() const { return m_assignmentToClauseIndicesMap; }
      Assignments minimalizeAssignments(const Assignments& inputAssignments);
      Assumptions createAssumptionsWithAllMarkersFalse() const;
      void disableAllMissingAssignments(Assumptions& assumptions, const Assignments& assignments);
      
      private:
      CnfPtr m_factorGraphCnf;
      int m_numMustVariables;
      bool m_mustMinimalizeAssignments;
      ClauseToAssignmentMap m_clauseToAssignmentMap;
      AssignmentToClauseIndicesMap m_assignmentToClauseIndicesMap;
      AssignmentToMarkerPositionsMap m_assignmentToMarkerPositionsMap;
      Assumptions m_assumptionsWithAllMarkersFalse;
      Minisat::Solver m_factorGraphResultSolver;
      std::weak_ptr<Master> m_mustMaster;
      std::clock_t m_explorationStartTime;
      Minisat::Solver m_minimalizationSolver;
      std::optional<std::ofstream> m_musResultFile;
    };

    // function declarations
    CommandLineOptions parseClo(int argc, char const * const * const argv);

    std::shared_ptr<DdManager> ddm_init();
    fgpp::FactorGraph::Ptr createFactorGraph(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd, int largestSupportSet, int largestBddSize);
    std::shared_ptr<dd::Qdimacs> parseQdimacs(const std::string & inputFilePath);
    std::shared_ptr<Master> createMustMaster(const dd::Qdimacs& qdimacs,
                                             const Oct22MucCallback::CnfPtr& factorGraphCnf,
                                             bool mustMinimalizeAssignments,
                                             std::optional<std::string> const& musResultFile);
    std::vector<dd::BddWrapper> getFactorGraphResults(DdManager* ddm, const fgpp::FactorGraph& fg, const dd::QdimacsToBdd& qdimacsToBdd);
    dd::BddWrapper getExactResult(DdManager* ddm, const dd::QdimacsToBdd& qdimacsToBdd);
    Oct22MucCallback::CnfPtr convertToCnf(DdManager* ddm, 
                                          int numVariables, 
                                          const std::vector<dd::BddWrapper> & funcs);
    void writeResult(const Oct22MucCallback::Cnf& cnf,
                     const dd::Qdimacs& qdimacs,
                     const std::string& outputFile);
    bdd_ptr computeExact(const dd::QdimacsToBdd& qdimacsToBdd);
    bdd_ptr cnfToBdd(const dd::QdimacsToBdd& qdimacsToBdd, const Oct22MucCallback::Cnf& fgMustResult);


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

    void test(DdManager* manager);


} // end namespace oct_22