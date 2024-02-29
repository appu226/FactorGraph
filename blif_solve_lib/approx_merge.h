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

#pragma once

#include <dd/dd.h>
#include <dd/bdd_factory.h>

#include <vector>
#include <memory>
#include <map>
#include <set>

namespace blif_solve
{

  class MergeHints {
    
    public:
      typedef dd::BddWrapper BddWrapper;
      typedef std::pair<BddWrapper, BddWrapper> FactorPair;
      MergeHints(DdManager* manager): m_manager(manager) { }
      void addWeight(bdd_ptr func1, bdd_ptr func2, double weight);
      double getWeight(bdd_ptr func1, bdd_ptr func2) const;
      void merge(bdd_ptr func1, bdd_ptr func2, bdd_ptr newFunc);

    private:
      typedef std::map<FactorPair, double> WeightMap;
      WeightMap m_weights;
      DdManager * m_manager;

  };

  struct MergeResults
  {
    typedef std::shared_ptr<std::vector<bdd_ptr> > FactorVec;
    typedef std::shared_ptr<std::vector<std::string> > NameVec;
    FactorVec factors;
    FactorVec variables;
    NameVec factorNames;
    NameVec variableNames;
  };

  MergeResults 
    merge(DdManager * manager,
          const std::vector<bdd_ptr> & factors, 
          const std::vector<bdd_ptr> & variables, 
          int largestSupportSet,
          int largestBddSize,
          const MergeHints& mergeHints,
          const std::set<bdd_ptr>& quantifiedVariables,
          const std::vector<std::string> & factorNames,
          const std::vector<std::string> & variableNames);

  MergeResults
    varElimMerge(DdManager * manager,
                 const std::vector<bdd_ptr> & factors, 
                 const std::vector<bdd_ptr> & variables, 
                 int largestSupportSet,
                 int largestBddSize,
                 const MergeHints& mergeHints,
                 const std::set<bdd_ptr>& quantifiedVariables,
                 const std::vector<std::string> & factorNames,
                 const std::vector<std::string> & variableNames);

} // end namespace blif_solve
