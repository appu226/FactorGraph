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

#include "bdd_partition.h"
#include "disjoint_set.h"

#include <unordered_map>

std::vector<std::vector<bdd_ptr>> bddPartition(DdManager * manager, std::vector<bdd_ptr> const & inputBdds)
{
  typedef parakram::DisjointSet<bdd_ptr> FactorDs;
  std::vector<std::vector<bdd_ptr>> result;
  std::vector<FactorDs::Ptr> factorSets; // singleton sets of the factors
  std::unordered_map<int, std::vector<int>> varToNeighborMap; // adjacency list for variable nodes
  bdd_ptr one = bdd_one(manager);

  // loop across factors
  // to populate the singleton sets and adjacency lists
  for (auto factor: inputBdds)
  {
    // make a singleton set with the factor
    int id = factorSets.size();
    factorSets.push_back(std::make_shared<FactorDs>(id, factor));

    // loop across all the vars in the factor
    auto vars = bdd_support(manager, factor);
    while(vars != one)
    {
      // reduce the set of vars by one
      auto nextVarIndex = bdd_get_lowest_index(manager, vars);
      auto nextVar = bdd_new_var_with_index(manager, nextVarIndex);
      auto reducedVars = bdd_cube_diff(manager, vars, nextVar);
      bdd_free(manager, nextVar);
      bdd_free(manager, vars);
      vars = reducedVars;

      // mark the current function as a neighbor of this var
      varToNeighborMap[nextVarIndex].push_back(id);
    } // end loop across vars in the factor
    bdd_free(manager, vars);
  } // end loop across factors
  bdd_free(manager, one);


  // loop across all variables to compute union of adjacent factor sets
  for (auto varAndNeighbors: varToNeighborMap)
  {
    auto var = varAndNeighbors.first;
    auto const & neighbors = varAndNeighbors.second;
    auto firstFactorSet = factorSets[neighbors.front()];
    for (auto neighborId: neighbors)
    {
      auto nextNeighborSet = factorSets[neighborId];
      firstFactorSet->computeUnion(*nextNeighborSet);
    }
  }

  // collect elements in partitions
  std::vector<std::vector<bdd_ptr> > tempResult;
  tempResult.resize(factorSets.size());
  for (auto factorSet: factorSets)
  {
    auto parent = factorSet->find();
    auto idx = parent->id();
    tempResult[idx].push_back(factorSet->element());
  }

  // get rid of empty partitions
  for (auto r: tempResult)
    if (!r.empty()) result.push_back(r);

  return result;
} // end bddPartition
