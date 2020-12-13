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


#include <bdd_factory.h>

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
  std::vector<BddWrapper> f;
  f.emplace_back(v[0] + v[1]*v[2] + v[1]*(-v[3]) + (-v[0]) * (-v[1]));
  f.emplace_back(v[1] + v[0]*v[2]);
  f.emplace_back(v[3] + v[4]);
  f.emplace_back(v[1] + v[2]);
  std::vector<bdd_ptr> F;
  for (size_t i = 0; i < f.size(); ++i)
    F.push_back(f[i].getUncountedBdd());


  // setup VarScoreQuantification object
  BddWrapper qbw = v[0];
  for (size_t i = 1; i < v.size(); ++i)
    qbw = qbw * v[i];
  bdd_ptr Q = qbw.getUncountedBdd();
  int maxBddSize = 0;
  var_score::VarScoreQuantification vsq(F, Q, ddm);

  // test neighboringFactors
  for (size_t vidx = 0; vidx < v.size(); ++vidx)
  {
    std::set<bdd_ptr> actual = vsq.neighboringFactors(v[vidx].getUncountedBdd());
    for (size_t fidx = 0; fidx < f.size(); ++fidx)
    {
      auto commonSupport = f[fidx].support().cubeIntersection(v[vidx]);
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
    auto allNeigh = vsq.neighboringFactors(v[vidx].getUncountedBdd());
    if (allNeigh.size() < 2)
      continue;
    auto smallestTwoNeigh = vsq.smallestTwoNeighbors(v[vidx].getUncountedBdd());
    int size1 = bdd_size(smallestTwoNeigh.first);
    int size2 = bdd_size(smallestTwoNeigh.second);
    for (auto n: allNeigh)
    {
      int nsize = bdd_size(n);
      assert(nsize >= size1 || nsize == size2);
      assert(nsize >= size2 || nsize == size1);
    }
  }

  // test varWithLowestScore
  bdd_ptr varWithLowestScore = vsq.varWithLowestScore();
  int lowestScore = -1;
  std::vector<int> scores;
  for (size_t vidx = 0; vidx < v.size(); ++vidx)
  {
    bdd_ptr theVar = v[vidx].getUncountedBdd();
    int score = 0;
    auto neighbors = vsq.neighboringFactors(theVar);
    for (auto n: neighbors)
      score += bdd_size(n);
    scores.push_back(score);
    if (theVar == varWithLowestScore)
      lowestScore = score;
  }
  assert(lowestScore != -1);
  for (auto score: scores)
    assert(lowestScore <= score);

  // test findVarWithOnlyOneFactor
  assert(vsq.findVarWithOnlyOneFactor() == v[4].getUncountedBdd());
  vsq.removeFactor(F[2]);
  assert(vsq.findVarWithOnlyOneFactor() == v[3].getUncountedBdd());
  vsq.removeVar(v[4].getUncountedBdd());
  assert(vsq.findVarWithOnlyOneFactor() == v[3].getUncountedBdd());
  vsq.addFactor(v[3].getUncountedBdd());
  assert(vsq.findVarWithOnlyOneFactor() == NULL);


}

