/*

Copyright 2024 Parakram Majumdar

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

#include "approx_merge.h"

#include <dd/disjoint_set.h>
#include <dd/max_heap.h>

#include <list>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <unordered_map>

namespace {

    using namespace parakram;
    using namespace blif_solve;

    struct VemFuncNode;
    struct VemVarNode;
    typedef MaxHeap<VemVarNode *, double> VemHeap;
    typedef typename VemHeap::DataCellCptr HeapEntry;

    struct VemFuncNode
    {
        std::list<VemVarNode *> neigh;
        bdd_ptr func;
    };

    struct VemVarNode
    {
        typedef std::list<VemVarNode *>::iterator NeighLinkToMe;
        std::vector<std::pair<VemFuncNode *, NeighLinkToMe> > neigh;
        bdd_ptr var;
        HeapEntry heapEntry;
    };



} // end anonymous namespace




namespace blif_solve {

    MergeResults
    varElimMerge(DdManager * manager,
                 const std::vector<bdd_ptr> & factors, 
                 const std::vector<bdd_ptr> & variables, 
                 int largestSupportSet,
                 int largestBddSize,
                 const MergeHints& mergeHints,
                 const std::set<bdd_ptr>& quantifiedVariables,
                 const std::vector<std::string> & factorNames,
                 const std::vector<std::string> & variableNames)
    {
        throw std::runtime_error("blif_solve::varElimMerge not yet implemented.");
    }

} // end namespace blif_solve