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


#include <dd/bdd_factory.h>

#include "testVarScoreQuantification.h"

#include <cassert>


void testVarScoreQuantificationUtils(DdManager * ddm)
{
  using dd::BddWrapper;
  // prepare variables
  std::vector<BddWrapper> v;
  for (int i = 0; i < 5; ++i)
    v.emplace_back(bdd_new_var_with_index(ddm, i), ddm);
  
  // prepare factors
  std::vector<BddWrapper> F;
  F.emplace_back(v[0] + v[1]*v[2] + v[1]*(-v[3]) + (-v[0]) * (-v[1]));
  F.emplace_back(v[1] + v[0]*v[2]);
  F.emplace_back(v[3] + v[4]);
  F.emplace_back(v[1] + v[2]);

  // setup VarScoreQuantification object
  BddWrapper Q = v[0];
  for (size_t i = 1; i < v.size(); ++i)
    Q = Q * v[i];
  int maxBddSize = 0;
  var_score::VarScoreQuantification vsq(F, Q, ddm);

  // test neighboringFactors
  for (size_t vidx = 0; vidx < v.size(); ++vidx)
  {
    std::set<BddWrapper> actual = vsq.neighboringFactors(v[vidx]);
    for (size_t fidx = 0; fidx < F.size(); ++fidx)
    {
      auto commonSupport = F[fidx].support().cubeIntersection(v[vidx]);
      bool isNeighbor = !bdd_is_one(ddm, commonSupport.getUncountedBdd());
      if (isNeighbor)
        assert(1 == actual.count(F[fidx]));
      else
        assert(0 == actual.count(F[fidx]));
    }
  }


  // test smallestTwoNeighbors
  for (size_t vidx = 0; vidx < v.size(); ++vidx)
  {
    auto allNeigh = vsq.neighboringFactors(v[vidx]);
    if (allNeigh.size() < 2)
      continue;
    auto smallestTwoNeigh = vsq.smallestTwoNeighbors(v[vidx]);
    int size1 = bdd_size(smallestTwoNeigh.first.getUncountedBdd());
    int size2 = bdd_size(smallestTwoNeigh.second.getUncountedBdd());
    for (auto n: allNeigh)
    {
      int nsize = bdd_size(n.getUncountedBdd());
      assert(nsize >= size1 || nsize == size2);
      assert(nsize >= size2 || nsize == size1);
    }
  }

  // test varWithLowestScore
  auto varWithLowestScore = vsq.varWithLowestScore();
  int lowestScore = -1;
  std::vector<int> scores;
  for (size_t vidx = 0; vidx < v.size(); ++vidx)
  {
    auto theVar = v[vidx];
    int score = 0;
    auto neighbors = vsq.neighboringFactors(theVar);
    for (auto n: neighbors)
      score += bdd_size(n.getUncountedBdd());
    scores.push_back(score);
    if (theVar == varWithLowestScore)
      lowestScore = score;
  }
  assert(lowestScore != -1);
  for (auto score: scores)
    assert(lowestScore <= score);

  // test findVarWithOnlyOneFactor
  assert(vsq.findVarWithOnlyOneFactor() == std::optional<BddWrapper>(v[4]));
  vsq.removeFactor(F[2]);
  assert(vsq.findVarWithOnlyOneFactor() == std::optional<BddWrapper>(v[3]));
  vsq.removeVar(v[4]);
  assert(vsq.findVarWithOnlyOneFactor() == std::optional<BddWrapper>(v[3]));
  vsq.addFactor(v[3]);
  assert(!vsq.findVarWithOnlyOneFactor().has_value());
}

