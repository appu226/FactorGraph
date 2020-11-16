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


#include <dd.h>
#include <cnf_dump.h>
#include <optional.h>
#include <lru_cache.h>
#include <max_heap.h>
#include <clo.hpp>

#include <memory>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <factor_graph.h>
#include <bdd_partition.h>

#include "bdd_factory.h"
#include "testApproxMerge.h"
#include "testVarScoreQuantification.h"

void testCuddBddAndAbstractMulti(DdManager * manager);
void testCnfDump(DdManager * manager);
void testIsConnectedComponent(DdManager * manager);
void testCuddBddCountMintermsMulti(DdManager * manager);
void testOptional();
void testLruCache();
void testDisjointSet(DdManager * manager);
void testMaxHeap();
void testClo();
void testVarScoreQuantificationAlgo(DdManager * manager);

DdNode * makeFunc(DdManager * manager, int const numVars, int const funcAsIntger);

int main()
{
  try {
    DdManager * manager = Cudd_Init(0, 0, 256, 262144, 0);
    common_error(manager, "test/main.cpp : Could not initialize DdManager\n");

    
    testCuddBddCountMintermsMulti(manager);
    testCuddBddAndAbstractMulti(manager);
    testCnfDump(manager);
    testIsConnectedComponent(manager);
    testOptional();
    testLruCache();
    testDisjointSet(manager);
    testMaxHeap();
    testApproxMerge(manager);
    testClo();
    testVarScoreQuantificationUtils(manager);
    testVarScoreQuantificationAlgo(manager);

    std::cout << "SUCCESS" << std::endl;

    return 0;
  }
  catch (std::exception const & e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return -1;
  }
}



void testVarScoreQuantificationAlgo(DdManager * manager)
{
  int const numVars = 4;
  int const numTests = 5000;
  int const numFuncsPerTest = 6;
  int const largestSupportSet = 6;
  std::vector<DdNode *> vars;
  for (int vi = 0; vi < numVars; ++vi)
    vars.push_back(bdd_new_var_with_index(manager, vi));
  int const totalMinTerms = 1 << numVars;
  int const totalFuncs = 1 << totalMinTerms;
  for (int itest = 0; itest < numTests; ++itest)
  {
    std::set<DdNode *> funcs;
    for (int ifunc = 0; ifunc < numFuncsPerTest; ++ifunc)
    {
      int const funcAsInteger = rand() % totalFuncs;
      auto f = makeFunc(manager, numVars, funcAsInteger);
      funcs.insert(f);
    }
    
    const int numVarsToBeQuantified = rand() % numVars;
    DdNode * cube = bdd_one(manager);
    for (int iqv = 0; iqv < numVarsToBeQuantified; ++iqv)
    {
      auto temp = cube;
      auto varIndex = rand() % numVars;
      cube = bdd_cube_union(manager, cube, bdd_new_var_with_index(manager, varIndex));
      bdd_free(manager, temp);
    }

    auto manualResult = bdd_and_exists_multi(manager, funcs, cube, 100*100);

    std::vector<bdd_ptr> fvec(funcs.cbegin(), funcs.cend());
    auto varScoreResultVec = var_score::VarScoreQuantification::varScoreQuantification(fvec, cube, largestSupportSet, manager);
    std::set<bdd_ptr> varScoreResultSet(varScoreResultVec.cbegin(), varScoreResultVec.cend());
    auto varScoreResult = bdd_and_multi(manager, varScoreResultSet, 100*1000);


    assert(varScoreResult == manualResult);

    for (auto f: funcs)
      bdd_free(manager, f);
    bdd_free(manager, cube);
    bdd_free(manager, manualResult);
    for (auto f: varScoreResultVec)
      bdd_free(manager, f);
    bdd_free(manager, varScoreResult);
  }
  for (auto v: vars)
    bdd_free(manager, v);
}




void testClo()
{
  auto alpha = blif_solve::CommandLineOption<std::string>::make("--alpha", "Alpha");
  auto beta = blif_solve::CommandLineOption<int>::make("--beta", "Beta");
  int argc = 5;
  char const * const argv[5] = {"skip", "--alpha", "3", "--beta", "4"};
  std::vector<std::shared_ptr<blif_solve::CloInterface> > cloVec {alpha, beta};
  blif_solve::parse(cloVec, argc, argv);
  assert(alpha->value.has_value());
  assert(alpha->value.value() == "3");
  assert(beta->value.has_value());
  assert(beta->value.value() == 4);
 
}


void testMaxHeap()
{
  typedef parakram::MaxHeap<std::string, int> MH;
  std::vector<std::pair<std::string, int> > values;
  values.push_back(std::make_pair("five", 5));
  values.push_back(std::make_pair("two", 2));
  values.push_back(std::make_pair("one", 1));
  values.push_back(std::make_pair("four", 4));
  values.push_back(std::make_pair("four", 4));
  values.push_back(std::make_pair("three", 3));
  MH max_heap(values);
  assert(max_heap.top() == "five");
  max_heap.pop();
  assert(max_heap.top() == "four");
  auto seven = max_heap.insert("seven", 3);
  assert(max_heap.top() == "four");
  max_heap.updatePriority(seven, 7);
  assert(max_heap.top() == "seven");
  auto five = max_heap.insert("five", 8);
  assert(max_heap.top() == "five");
  max_heap.updatePriority(five, 5);
  assert(max_heap.top() == "seven");
  max_heap.pop();
  assert(max_heap.top() == "five");
  max_heap.pop();
  assert(max_heap.top() == "four");
  max_heap.pop();
  assert(max_heap.top() == "four");
  max_heap.pop();
  assert(max_heap.top() == "three");
  max_heap.pop();
  assert(max_heap.top() == "two");
  assert(max_heap.size() == 2);
  max_heap.pop();
  assert(max_heap.top() == "one");
  max_heap.pop();
  assert(max_heap.size() == 0);

  max_heap.insert("one", 1);
  seven = max_heap.insert("seven", 7);
  assert(max_heap.top() == "seven");
  max_heap.updatePriority(seven, 0);
  assert(max_heap.top() == "one");
  max_heap.updatePriority(seven, 7);
  assert(max_heap.top() == "seven");
  max_heap.updatePriority(seven, 0);
  assert(max_heap.top() == "one");
  max_heap.pop();
  assert(max_heap.top() == "seven");
  max_heap.updatePriority(seven, 7);
  assert(max_heap.top() == "seven");
  max_heap.updatePriority(seven, 0);
  assert(max_heap.top() == "seven");
}



struct DestructorCounter {
  static int count;
  ~DestructorCounter() {
    ++count;
  }
};

int DestructorCounter::count = 0;


void testDisjointSet(DdManager * manager)
{
  using namespace test;

  std::vector<BddWrapper> v;
  for (int i = 0; i < 10; ++i)
    v.push_back(BddWrapper(bdd_new_var_with_index(manager, i), manager));
  
  //  f3  f4 -- f5 -- f6    f0
  //    \  |         
  // f1-- f2    f7 -- f8 -- f9
  //
  // f0 = v9
  // f1 = v0 . v1
  // f2 = v1.v2 + v1.v3
  // f3 = v2 + !v3
  // f4 = !v1 + v2.v4
  // f5 = v4 + !v5
  // f6 = v5
  //
  // f7 = v6 + v7
  // f8 = v7 . !v8
  // f9 = v8

  BddWrapper f0 = v[9];
  BddWrapper f1 = v[0] * v[1];
  BddWrapper f2 = v[1] * v[2] + v[1] * v[3];
  BddWrapper f3 = v[2] + (-v[3]);
  BddWrapper f4 = (-v[1]) + v[2] * v[4];
  BddWrapper f5 = v[4] + (-v[5]);
  BddWrapper f6 = v[5];
  BddWrapper f7 = v[6] + v[7];
  BddWrapper f8 = v[7] + (-v[8]);
  BddWrapper f9 = v[8];

  std::vector<bdd_ptr> factors{!f0, !f1, !f2, !f3, !f4, !f5, !f6, !f7, !f8, !f9};
  auto partitions = bddPartition(manager, factors);

  assert(partitions.size() == 3);
  std::vector<bdd_ptr> p1, p3, p6;
  for (auto partition: partitions)
  {
    if (partition.size() == 1)
      p1 = partition;
    else if (partition.size() == 3)
      p3 = partition;
    else if (partition.size() == 6)
      p6 = partition;
  }

  // expected sets
  std::set<bdd_ptr> p1x{!f0};
  std::set<bdd_ptr> p3x{!f7, !f8, !f9};
  std::set<bdd_ptr> p6x{!f1, !f2, !f3, !f4, !f5, !f6};

  assert(std::set<bdd_ptr>(p1.begin(), p1.end()) == p1x);
  assert(std::set<bdd_ptr>(p3.begin(), p3.end()) == p3x);
  assert(std::set<bdd_ptr>(p6.begin(), p6.end()) == p6x);

}


void testLruCache()
{
  using namespace parakram;
  LruCache<int, std::string> lc(3);
  lc.insert(1, "a");
  auto o = lc.tryGet(1);
  assert(o.isPresent());
  assert(o.get() == "a");
  o = lc.tryGet(2);
  assert(!o.isPresent());

  lc.insert(2, "b");
  lc.insert(3, "c");

  assert(lc.tryGet(1).get() == "a");
  assert(lc.tryGet(2).get() == "b");
  assert(lc.tryGet(3).get() == "c");

  auto ctos = [](char c){ return std::string(&c, &c + 1); };

  for (int i = 0; i < 26; ++i)
  {
    lc.insert(i + 1, ctos('a' + i));
    if (i > 1) assert(lc.tryGet(i - 1).get() == ctos('a' + i - 2));
    if (i > 0) assert(lc.tryGet(i).get() == ctos('a' + i - 1));
    assert(lc.tryGet(i + 1).get() == ctos('a' + i));
  }
}

void testOptional() {
  using namespace parakram;
  Optional<int> oi;
  assert(!oi.isPresent());
  oi.reset(55);
  assert(oi.isPresent());
  assert(oi.get() == 55);
  oi.reset();
  assert(!oi.isPresent());

  assert(DestructorCounter::count == 0);

  DestructorCounter dc;
  
  Optional<DestructorCounter> odc(dc);
  assert(DestructorCounter::count == 0);
  assert(odc.isPresent());
  
  odc.reset();
  assert(DestructorCounter::count == 1);
  assert(!odc.isPresent());

  odc.reset();
  assert(DestructorCounter::count == 1);
  assert(!odc.isPresent());

  odc.reset(dc);
  assert(DestructorCounter::count == 1);
  assert(odc.isPresent());

  odc.reset(dc);
  assert(DestructorCounter::count == 2);
  assert(odc.isPresent());

  if (odc.isPresent())
  {
    Optional<DestructorCounter> odc2;
  }
  assert(DestructorCounter::count == 2);

  if (odc.isPresent())
  {
    Optional<DestructorCounter> odc2(odc);
  }
  assert(DestructorCounter::count == 3);

  oi.reset();
  Optional<int> oi2(oi);
  assert(!oi2.isPresent());
  oi.reset(5);
  Optional<int> oi3(oi);
  assert(oi3.isPresent());
  assert(oi3.get() == 5);

  oi.reset();
  oi3 = oi;
  assert(!oi3.isPresent());
  
  oi.reset(50);
  oi2 = oi;
  assert(oi2.isPresent());
  assert(oi2.get() == 50);
}

void testCnfDump(DdManager * manager)
{
  using test::BddWrapper;
  BddWrapper x(bdd_new_var_with_index(manager, 1), manager);
  BddWrapper y(bdd_new_var_with_index(manager, 2), manager);
  BddWrapper z(bdd_new_var_with_index(manager, 3), manager);

  bdd_ptr_set upperLimit;
  auto overApprox1 = x*y + x*-y*z;
  auto overApprox2 = y*-z + -y*z;
  upperLimit.insert(overApprox1.getUncountedBdd());
  upperLimit.insert(overApprox2.getUncountedBdd());

  bdd_ptr_set lowerLimit;
  auto underApprox1 = x*y*z;
  auto underApprox2 = overApprox2;
  lowerLimit.insert(underApprox1.getUncountedBdd());
  lowerLimit.insert(underApprox2.getUncountedBdd());

  bdd_ptr_set allVars;
  allVars.insert(y.getUncountedBdd());
  allVars.insert(z.getUncountedBdd());
  allVars.insert(x.getUncountedBdd());

  blif_solve::dumpCnfForModelCounting(manager,
                                      allVars,
                                      upperLimit,
                                      lowerLimit,
                                      "temp/testCnfDump.dimacs");

}




void testIsConnectedComponent(DdManager * manager)
{
  using test::BddWrapper;
  BddWrapper w(bdd_new_var_with_index(manager, 1), manager);
  BddWrapper x(bdd_new_var_with_index(manager, 2), manager);
  BddWrapper y(bdd_new_var_with_index(manager, 3), manager);
  BddWrapper z(bdd_new_var_with_index(manager, 4), manager);


  auto fxy = x * y;
  auto fyz = -y + y * z;
  auto fwx = w + -x;

  std::vector<bdd_ptr> funcs;
  funcs.push_back(fwx.getUncountedBdd());
  funcs.push_back(fyz.getUncountedBdd());
  funcs.push_back(fyz.getUncountedBdd()); // add again, to make a cycle
  
  auto fg_disconnected = factor_graph_new(manager, &funcs.front(), funcs.size());
  if (factor_graph_is_single_connected_component(fg_disconnected))
    throw std::runtime_error("factor graph was expected to be disconnected");
  factor_graph_delete(fg_disconnected);

  funcs.push_back(fxy.getUncountedBdd());
  auto fg_connected = factor_graph_new(manager, &funcs.front(), funcs.size());
  if (!factor_graph_is_single_connected_component(fg_connected))
    throw std::runtime_error("factor graph was expected to be connected");
  factor_graph_delete(fg_connected);
}


void testCuddBddAndAbstractMulti(DdManager * manager)
{
  int const numVars = 3;
  int const numTests = 5000;
  int const numFuncsPerTest = 3;
  int const maxDepth = 2;
  std::vector<DdNode *> vars;
  for (int vi = 0; vi < numVars; ++vi)
    vars.push_back(bdd_new_var_with_index(manager, vi));
  int const totalMinTerms = 1 << numVars;
  int const totalFuncs = 1 << totalMinTerms;
  for (int itest = 0; itest < numTests; ++itest)
  {
    std::set<DdNode *> funcs;
    for (int ifunc = 0; ifunc < numFuncsPerTest; ++ifunc)
    {
      int const funcAsInteger = rand() % totalFuncs;
      auto f = makeFunc(manager, numVars, funcAsInteger);
      funcs.insert(f);
    }
    
    const int numVarsToBeQuantified = rand() % numVars;
    DdNode * cube = bdd_one(manager);
    for (int iqv = 0; iqv < numVarsToBeQuantified; ++iqv)
    {
      auto temp = cube;
      auto varIndex = rand() % numVars;
      cube = bdd_cube_union(manager, cube, bdd_new_var_with_index(manager, varIndex));
      bdd_free(manager, temp);
    }

    auto manualConjunction = bdd_one(manager);
    for (auto f: funcs)
      bdd_and_accumulate(manager, &manualConjunction, f);
    auto manualResult = bdd_forsome(manager, manualConjunction, cube);

    auto autoConjunction = bdd_and_multi(manager, funcs, 100*1000);
    auto autoResult = bdd_and_exists_multi(manager, funcs, cube, 100*1000);
    auto conjunctionClipUp = bdd_clipping_and_multi(manager, funcs, maxDepth, dd_constants::Clip_Up);
    auto conjunctionClipDown = bdd_clipping_and_multi(manager, funcs, maxDepth, dd_constants::Clip_Down);
    auto resultClipUp = bdd_clipping_and_exists_multi(manager, funcs, cube, maxDepth, dd_constants::Clip_Up);
    auto resultClipDown = bdd_clipping_and_exists_multi(manager, funcs, cube, maxDepth, dd_constants::Clip_Down);


    if (manualResult != autoResult)
      throw std::runtime_error("bdd_and_exists_multi did not give expected result");
    if (autoConjunction != manualConjunction)
      throw std::runtime_error("bdd_and_multi did not give expected result");
    if (!Cudd_bddLeq(manager, autoConjunction, conjunctionClipUp))
      throw std::runtime_error("bdd_clipping_and_multi did not give over approximation");
    if (!Cudd_bddLeq(manager, conjunctionClipDown, autoConjunction))
      throw std::runtime_error("bdd_clipping_and_multi did not give under approximation");
    if (!Cudd_bddLeq(manager, autoResult, resultClipUp))
      throw std::runtime_error("bdd_clipping_and_exists_multi did not give over approximation");
    if (!Cudd_bddLeq(manager, resultClipDown, autoResult))
      throw std::runtime_error("bdd_clipping_and_exists_multi did not give under approximation");


    for (auto f: funcs)
      bdd_free(manager, f);
    bdd_free(manager, cube);
    bdd_free(manager, manualConjunction);
    bdd_free(manager, manualResult);
    bdd_free(manager, autoConjunction);
    bdd_free(manager, autoResult);
    bdd_free(manager, conjunctionClipUp);
    bdd_free(manager, conjunctionClipDown);
    bdd_free(manager, resultClipUp);
    bdd_free(manager, resultClipDown);
  }
  return;
} // end testCuddBddAndAbstractMulti

void testCuddBddCountMintermsMulti(DdManager * manager)
{
  const int numVars = 3;
  const int numTests = 5000;
  const int numFuncsPerTest = 3;
  const long double relativeTolerance = 1e-10;
  std::vector<DdNode *> vars;
  for (int vi = 0; vi < numVars; ++vi)
    vars.push_back(bdd_new_var_with_index(manager, vi));
  int const totalMinTerms = 1 << numVars;
  int const totalFuncs = 1 << totalMinTerms;
  for (int itest = 0; itest < numTests; ++itest)
  {
    std::set<DdNode *> funcs;
    for (int ifunc = 0; ifunc < numFuncsPerTest; ++ifunc)
    {
      int const funcAsInteger = rand() % totalFuncs;
      auto f = makeFunc(manager, numVars, funcAsInteger);
      funcs.insert(f);
    }

    const auto computedAnswer = bdd_count_minterm_multi(manager, funcs, numVars, 100*1000);
    //std::cout << "number of solutions is " << computedAnswer << std::endl; // DeleteMe

    bdd_ptr conj = bdd_and_multi(manager, funcs, 100*1000);
    const auto expectedAnswer = bdd_count_minterm(manager, conj, numVars);
    bdd_free(manager, conj);
    for (auto f: funcs)
      bdd_free(manager, f);


    const auto absoluteTolerance = (std::abs(expectedAnswer * .5) + std::abs(computedAnswer * .5)) * relativeTolerance;
    const auto absoluteDiff = std::abs(expectedAnswer - computedAnswer);
    if (absoluteDiff > absoluteTolerance)
    {
      for(const auto f: funcs)
      {
        std::cout << "--\n";
        bdd_print_minterms(manager, f);
      }
      std::stringstream ss;
      ss << "bdd_count_minterm_multi gave an answer (" << computedAnswer
         << ") different from the expected answer (" << expectedAnswer
         << ") where relative tolerance is " << relativeTolerance;
      throw std::runtime_error(ss.str());
    }
  }
    
 
}

DdNode * makeFunc(DdManager * manager, int const numVars, int const funcAsInteger)
{
  int const totalMinTerms = 1 << numVars;
  auto func = bdd_zero(manager);
  for (int iMinTerm = 0; iMinTerm < totalMinTerms; ++iMinTerm)
  {
    if (funcAsInteger & (1 << iMinTerm))
    {
      auto minterm = bdd_one(manager);
      for (int ivar = 0; ivar < numVars; ++ivar)
      {
        auto var = bdd_new_var_with_index(manager, ivar);
        bool mustNegate = !(iMinTerm & (1 << ivar));
        if (mustNegate)
        {
          auto notVar = bdd_not(manager, var);
          bdd_and_accumulate(manager, &minterm, notVar);
          bdd_free(manager, notVar);
        } else
        {
          bdd_and_accumulate(manager, &minterm, var);
        }
        bdd_free(manager, var);
      }
      bdd_or_accumulate(manager, &func, minterm);
      bdd_free(manager, minterm);
    }
  }
  return func;
}


