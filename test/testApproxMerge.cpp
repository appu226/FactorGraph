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

#include "testApproxMerge.h"

#include <dd/bdd_factory.h>

#include <random>
#include <algorithm>
#include <assert.h>

namespace {

  using dd::BddWrapper;

  BddWrapper makeFactor(DdManager * manager,
                        std::default_random_engine & dre,
                        std::bernoulli_distribution & distVarPositiveInFactor,
                        std::vector<BddWrapper> & variables)
  {
    BddWrapper factor(bdd_one(manager), manager);
    for (auto var: variables)
    {
      if (distVarPositiveInFactor(dre))
        factor = factor * var;
      else
        factor = factor * -var;
    }
    return factor;
  }

  BddWrapper makeFunction(DdManager * manager,
                          std::default_random_engine & dre,
                          std::bernoulli_distribution & distVarIncludedInFunction,
                          std::bernoulli_distribution & distVarPositiveInFactor,
                          int numFactors,
                          std::vector<BddWrapper> & variables,
                          const BddWrapper & baseline)
  {
    std::vector<BddWrapper> variablesInFunc;
    for (auto variable: variables)
      if (distVarIncludedInFunction(dre))
        variablesInFunc.push_back(variable);
    BddWrapper func(bdd_zero(manager), manager);
    for (int i = 0; i < numFactors; ++i)
      func = func + makeFactor(manager, dre, distVarPositiveInFactor, variablesInFunc);
    auto fs = func.support();
    auto bls = baseline.support();
    auto nonSupport = bls.cubeDiff(fs);
    auto projectedBaseline = baseline.existentialQuantification(nonSupport);
    func = func + projectedBaseline;
    return func;
  }

  template<typename OutElem, typename InElem, typename Op>
    std::vector<OutElem>
    mapVector(const std::vector<InElem> & input,
              Op op)
    {
      std::vector<OutElem> output;
      output.reserve(input.size());
      std::transform(input.cbegin(), input.cend(), std::back_inserter(output), op);
      return output;
    }
} // end anonymous namespace

void testApproxMerge(DdManager * manager)
{
  using dd::BddWrapper;
  const int seed = 20190516;
  const int NumFuncs = 90;
  const int NumVars = 50;
  const int AvgVarsPerFunc = 10;
  const int NumFactorsPerFunc = 30;
  const double ProbVarPositiveInFactor = .5;
  const double ProbVarIncludedInBaseline = .9;
  const int LargestSupportSet = 20;
  const int LargestBddSize = 1000*1000*1000;
  const blif_solve::MergeHints hints(manager);

  // get all the variables
  std::vector<BddWrapper> variables;
  for (int i = 0; i < NumVars; ++i)
    variables.emplace_back(bdd_new_var_with_index(manager, i), manager);

  // create a baseline bdd to ensure that final conjunction is not false
  std::default_random_engine dre(seed);
  std::bernoulli_distribution distVarIncludedInBaseline(ProbVarIncludedInBaseline);
  std::bernoulli_distribution distVarPositiveInFactor(ProbVarPositiveInFactor);
  BddWrapper one(bdd_one(manager), manager);
  BddWrapper zero(bdd_zero(manager), manager);
  BddWrapper baseline = 
    makeFunction(manager, 
                 dre,
                 distVarIncludedInBaseline, 
                 distVarPositiveInFactor, 
                 1, variables, zero);

  // create functions
  std::vector<BddWrapper> functions;
  functions.reserve(NumFuncs);
  std::bernoulli_distribution distVarIncludedInFunction(static_cast<double>(AvgVarsPerFunc) / NumVars);
  for (int i = 0; i < NumFuncs; ++i)
    functions.push_back(makeFunction(manager, 
                                     dre,
                                     distVarIncludedInFunction,
                                     distVarPositiveInFactor,
                                     NumFactorsPerFunc,
                                     variables,
                                     baseline));

  auto wrapperToBdd = [&](const BddWrapper & bddWrapper) { return bddWrapper.getUncountedBdd(); };
  auto functionBdds = mapVector<bdd_ptr>(functions, wrapperToBdd);
  auto variableBdds = mapVector<bdd_ptr>(variables, wrapperToBdd);
  std::vector<std::string> emptyNameVec;
  auto mergeResults = blif_solve::merge(manager, functionBdds, variableBdds, LargestSupportSet, LargestBddSize, hints, std::set<bdd_ptr>(), emptyNameVec, emptyNameVec);

  auto bddToWrapper = [&](const bdd_ptr & in) { return BddWrapper(in, manager); };
  auto mergedFunctions = mapVector<BddWrapper>(*mergeResults.factors, bddToWrapper);
  auto mergedVariables = mapVector<BddWrapper>(*mergeResults.variables, bddToWrapper);

  BddWrapper expectedFullFunction = one;
  for (auto function: functions)
    expectedFullFunction = expectedFullFunction * function;
  BddWrapper mergedFullFunction(bdd_one(manager), manager);
  for (auto function: mergedFunctions)
    mergedFullFunction = mergedFullFunction * function;
  assert(expectedFullFunction.getUncountedBdd() == mergedFullFunction.getUncountedBdd());
  assert(expectedFullFunction.getUncountedBdd() != zero.getUncountedBdd());


  for (int i = 0; i < mergedVariables.size(); ++i)
    for (int j = i + 1; j < mergedVariables.size(); ++j)
    {
      auto vi = mergedVariables[i];
      auto vj = mergedVariables[j];
      auto intersection = vi.cubeIntersection(vj);
      assert(intersection.getUncountedBdd() == one.getUncountedBdd());
    }

  BddWrapper expectedFullVar = one;
  for (auto variable: variables) expectedFullVar = expectedFullVar * variable;
  BddWrapper actualFullVar = one;
  for (auto variable: mergedVariables) actualFullVar = actualFullVar * variable;
  assert(expectedFullVar.getUncountedBdd() == actualFullVar.getUncountedBdd());

  auto fullMergeResults = blif_solve::merge(manager, functionBdds, variableBdds, NumVars + 1, LargestBddSize, hints, std::set<bdd_ptr>(), emptyNameVec, emptyNameVec);
  auto fullMergeFunctions = mapVector<BddWrapper>(*fullMergeResults.factors, bddToWrapper);
  auto fullMergeVariables = mapVector<BddWrapper>(*fullMergeResults.variables, bddToWrapper);
  assert(fullMergeFunctions.size() == 1);
  assert(fullMergeFunctions[0].getUncountedBdd() == mergedFullFunction.getUncountedBdd());
  assert(fullMergeVariables.size() == 1);
  assert(fullMergeVariables[0].getUncountedBdd() == actualFullVar.getUncountedBdd());

}
