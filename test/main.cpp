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

#include <memory>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <factor_graph.h>

#include "bdd_factory.h"

void testCuddBddAndAbstractMulti(DdManager * manager);
void testCnfDump(DdManager * manager);
void testIsConnectedComponent(DdManager * manager);
void testCuddBddCountMintermsMulti(DdManager * manager);

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

    std::cout << "SUCCESS" << std::endl;

    return 0;
  }
  catch (std::exception const & e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return -1;
  }
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

    auto autoConjunction = bdd_and_multi(manager, funcs);
    auto autoResult = bdd_and_exists_multi(manager, funcs, cube);
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

    const auto computedAnswer = bdd_count_minterm_multi(manager, funcs, numVars);
    //std::cout << "number of solutions is " << computedAnswer << std::endl; // DeleteMe

    bdd_ptr conj = bdd_and_multi(manager, funcs);
    const auto expectedAnswer = bdd_count_minterm(manager, conj, numVars);
    bdd_free(manager, conj);
    for (auto f: funcs)
      bdd_free(manager, f);


    const auto absoluteTolerance = std::abs(expectedAnswer * .5) + std::abs(computedAnswer * .5);
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


