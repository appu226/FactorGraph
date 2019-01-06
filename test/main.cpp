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
#include <stdexcept>

#include "bdd_factory.h"

void testCuddBddAndAbstractMulti(DdManager * manager);
void testCnfDump(DdManager * manager);

DdNode * makeFunc(DdManager * manager, int const numVars, int const funcAsIntger);

int main()
{
  try {
    DdManager * manager = Cudd_Init(0, 0, 256, 262144, 0);
    common_error(manager, "test/main.cpp : Could not initialize DdManager\n");

    testCuddBddAndAbstractMulti(manager);
    testCnfDump(manager);

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

  blif_solve::dumpCnfForModelCounting(manager,
                                      upperLimit,
                                      lowerLimit,
                                      "temp/testCnfDump_headers.dimacs",
                                      "temp/testCnfDump_clauses.dimacs");

}


void testCuddBddAndAbstractMulti(DdManager * manager)
{
  int const numVars = 3;
  int const numTests = 5000;
  int const numFuncsPerTest = 3;
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
      //std::cout << "creating function for " << funcAsInteger << std::endl;
      //bdd_print_minterms(manager, f);
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
    //std::cout << "quantifying cube" << std::endl;
    //bdd_print_minterms(manager, cube);

    auto manualConjunction = bdd_one(manager);
    for (auto f: funcs)
      bdd_and_accumulate(manager, &manualConjunction, f);
    //std::cout << "Computed manual conjunction" << std::endl;
    //bdd_print_minterms(manager, manualConjunction);
    auto manualResult = bdd_forsome(manager, manualConjunction, cube);
    //std::cout << "Manually quantified result" << std::endl;
    //bdd_print_minterms(manager, manualResult);

    auto autoResult = bdd_and_exists_multi(manager, funcs, cube);
    //std::cout << "Automatically quantified result" << std::endl;
    //bdd_print_minterms(manager, autoResult);
    auto answerMatches = bdd_xnor(manager, autoResult, manualResult);
    auto one = bdd_one(manager);
    if (answerMatches != one)
      throw std::runtime_error("answer does not match");

    for (auto f: funcs)
      bdd_free(manager, f);
    bdd_free(manager, cube);
    bdd_free(manager, manualConjunction);
    bdd_free(manager, manualResult);
    bdd_free(manager, autoResult);
    bdd_free(manager, answerMatches);
    bdd_free(manager, one);
  }
  return;
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


