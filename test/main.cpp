#include <dd.h>

#include <memory>
#include <vector>
#include <cstdlib>

void testCuddBddAndAbstractMulti(DdManager * manager);
DdNode * makeFunc(DdManager * manager, int const numVars, int const funcAsIntger);

int main()
{
  DdManager * manager = Cudd_Init(0, 0, 256, 262144, 0);
  common_error(manager, "test/main.cpp : Could not initialize DdManager\n");
  
  testCuddBddAndAbstractMulti(manager);

  return 0;
}


void testCuddBddAndAbstractMulti(DdManager * manager)
{
  int const numVars = 3;
  int const numTests = 5000;
  int const numFuncsPerTest = 10;
  std::vector<DdNode *> vars;
  for (int vi = 0; vi < numVars; ++vi)
    vars.push_back(bdd_new_var_with_index(manager, vi));
  int const totalMinTerms = (1 << (numVars - 1));
  int const totalFuncs = (1 << (totalMinTerms - 1));
  for (int itest = 0; itest < numTests; ++itest)
  {
    std::set<DdNode *> funcs;
    for (int ifunc = 0; ifunc < numFuncsPerTest; ++ifunc)
    {
      int const funcAsInteger = rand() % totalFuncs;
      funcs.insert(makeFunc(manager, numVars, funcAsInteger));
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
    auto autoResult = bdd_and_exists_multi(manager, funcs, cube);
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
  int const totalMinTerms = 1 << (numVars - 1);
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


