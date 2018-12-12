#include <stdexcept>
#include "cuddAndAbsMulti.h"
#include <util.h>
#include <cuddInt.h>
#include <algorithm>


// **************************************
// *** Internal function declarations ***
// **************************************



// *** Function *****
// Recursive and-abstract implementation
DdNode * cuddBddAndAbstractMultiRecur(
    DdManager * manager,
    std::set<DdNode*> const & f,
    DdNode * cube);



// ***** Function *****
// Recursive "and" implementation
DdNode * cuddBddAndMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & f);











//****************************************
// *** Public api function definitions ***
//****************************************





/**
  @brief Takes the AND of a vector of BDDs and simultaneously abstracts 
  the variables in cube.

  @details The variables are existentially abstracted.
  Cudd_bddAndAbstractMulti implements the semiring matrix multiplication
  algorithm for the boolean semiring.

  @return a pointer to the result is successful; NULL otherwise.

  @sideeffect None

  @see Cudd_addMatrixMultiply Cudd_addTriangle Cudd_bddAnd

*/
DdNode * Cudd_bddAndAbstractMulti(
    DdManager *manager, 
    std::set<DdNode*> const & f, 
    DdNode *cube)
{
  DdNode * res;
  do {
    manager->reordered = 0;
    res = cuddBddAndAbstractMultiRecur(manager, f, cube);
  } while(manager->reordered == 1);
  if (manager->errorCode == CUDD_TIMEOUT_EXPIRED && manager->timeoutHandler) {
    manager->timeoutHandler(manager, manager->tohArg);
  }
  return (res);
} // end of Cudd_bddAndAbstractMulti





/**
  @brief Computes the conjunction of a set f of BDDs.

  @return a pointer to the resulting %BDD if successful; NULL if the
  intermediate result blows up.

  @sideeffect None

  @see Cudd_bddIte Cudd_addApply Cudd_bddAndAbstract Cudd_bddIntersect
  Cudd_bddOr Cudd_bddNand Cudd_bddNor Cudd_bddXor Cudd_bddXnor

*/
DdNode * Cudd_bddAndMulti(
    DdManager * dd,
    std::set<DdNode*> const & f,
    DdNode * cube)
{
  DdNode * res;
  do {
    dd->reordered = 0;
    res = cuddBddAndMultiRecur(dd, f);
  } while (dd->reordered == 1);
  if (dd->errorCode == CUDD_TIMEOUT_EXPIRED && dd->timeoutHandler)
  {
    dd->timeoutHandler(dd, dd->tohArg);
  }
  return res;
}





// *****************************************
// *** Internal api function definitions ***
// *****************************************



// *** Function *****
// Recursive and-abstract implementation
DdNode * cuddBddAndAbstractMultiRecur(
    DdManager * manager,
    std::set<DdNode*> const & fSet,
    DdNode * cube)
{

  statLine(manager);
  auto one = DD_ONE(manager);
  auto zero = Cudd_Not(one);
  DdNode * r = NULL;

  // Terminal cases.
  // if any of the funcs is zero, return zero
  if (fSet.count(zero) == 1) return zero;

  // if any of the funcs is the not of any other func, return zero
  for (auto f: fSet)
    if (fSet.count(Cudd_Not(f)) > 0)
      return zero;

  // if all of the funcs are one, return one
  if (fSet.empty()) return one;
  if (fSet.size() == 1 && *fSet.cbegin() == one) return one;

  // if there is only one element, no more need for conjunction
  if (fSet.size() == 1)
    return cuddBddExistAbstractRecur(manager, *fSet.cbegin(), cube);

  // find the top variable of the set of functions
  int top = manager->perm[Cudd_Regular(*fSet.cbegin())->index];
  for (auto f: fSet)
    top = ddMin(top, manager->perm[Cudd_Regular(f)->index]);

  // find the top variables of the quantified variables
  int topcube = manager->perm[cube->index];

  // skip the quantified variables until there is something to quantify
  while (topcube < top) {
    cube = cuddT(cube);
    if (cube == one) { // if there is nothing to quantify, return the conjunction
      return (cuddBddAndMultiRecur(manager, fSet));
    }
    topcube = manager->perm[cube->index];
  }

  // collect the 'then's and 'else's
  std::set<DdNode *> tv, ev;
  int index = 0;
  for (auto f: fSet)
  {
    auto F = Cudd_Regular(f);
    auto topf = manager->perm[F->index];
    if (topf == top) {
      index = F->index;
      auto t = cuddT(F);
      auto e = cuddE(F);
      if (Cudd_IsComplement(f)) {
        t = Cudd_Not(t);
        e = Cudd_Not(e);
      }
      tv.insert(t);
      ev.insert(e);
    } else {
      tv.insert(f);
      ev.insert(f);
    }
  }

  // need to quantify the topmost variable
  if (topcube == top) {
    auto *remainingCube = cuddT(cube);
    auto t = cuddBddAndAbstractMultiRecur(manager, tv, remainingCube);
    if (t == NULL) return NULL;
    // Special case: 1 or anything = 1. Hence, no need to compute
    // the else branch if t is 1. Likewise t + t * anything = t.
    // Notice that t == fe implies that fe does not depend on the
    // variables in the Cube.
    if (t == one || std::find(ev.cbegin(), ev.cend(), t) != ev.cend()) {
      return t;
    }

    cuddRef(t);
    // Special case: t + !t * anything == t + anything
    ev.erase(Cudd_Not(t));
    auto e = cuddBddAndAbstractMultiRecur(manager, ev, remainingCube);
    if (NULL == e)
    {
      Cudd_IterDerefBdd(manager, t);
      return NULL;
    }
    if (t == e)
    {
      r = t;
      cuddDeref(t);
    } else {
      cuddRef(e);
      r = cuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
      if (NULL == r) {
        Cudd_IterDerefBdd(manager, t);
        Cudd_IterDerefBdd(manager, e);
        return NULL;
      }
      r = Cudd_Not(r);
      cuddRef(r);
      Cudd_DelayedDerefBdd(manager, t);
      Cudd_DelayedDerefBdd(manager, e);
      cuddDeref(r);
    }
  } // end of case where you need to quantify
  else
  { // no need to quantify
    auto t = cuddBddAndAbstractMultiRecur(manager, tv, cube);
    if (NULL == t) return NULL;
    cuddRef(t);
    auto e = cuddBddAndAbstractMultiRecur(manager, ev, cube);
    if (NULL == e)
    {
      Cudd_IterDerefBdd(manager, t);
      return NULL;
    }

    if (t == e)
    {
      r = t;
      cuddDeref(t);
    } else {
      cuddRef(e);
      if (Cudd_IsComplement(t)) {
        r = cuddUniqueInter(manager,
                            (int) index,
                            Cudd_Not(t), 
                            Cudd_Not(e));
        if (NULL == r) {
          Cudd_IterDerefBdd(manager, t);
          Cudd_IterDerefBdd(manager, e);
          return NULL;
        }
        r = Cudd_Not(r);
      } else {
        r = cuddUniqueInter(manager, (int)index, t, e);
        if (r == NULL) {
          Cudd_IterDerefBdd(manager, t);
          Cudd_IterDerefBdd(manager, e);
          return NULL;
        }
      }
      cuddDeref(e);
      cuddDeref(t);
    }
  }

  return r;
} // end of cuddBddAndAbstractMultiRecur






// ***** Function *****
// Recursive "and" implementation
DdNode * cuddBddAndMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & fset)
{
  statLine(manager);
  auto one = DD_ONE(manager);

  // Terminal cases
  std::set<DdNode *> fSet = fset;
  fSet.erase(one);
  if (fSet.empty())
    return one;
  else if (fSet.size() == 1)
    return *fSet.cbegin();
  else if (fSet.size() == 2)
  {
    auto first = fSet.cbegin();
    auto second = fSet.cbegin();
    ++second;
    if (*first == Cudd_Not(*second))
      return Cudd_Not(one);
  }

  auto index = Cudd_Regular(*fSet.cbegin())->index;
  auto top = manager->perm[index];
  for (auto f: fSet)
  {
    auto F = Cudd_Regular(f);
    auto ftop = manager->perm[F->index];
    if (ftop < top)
    {
      top = ftop;
      index = F->index;
    }
  }

  std::set<DdNode *> tv, ev;
  for (auto f: fSet)
  {
    auto F = Cudd_Regular(f);
    if (index == F->index)
    {
      tv.insert(Cudd_IsComplement(f) ? Cudd_Not(cuddT(F)) : cuddT(F));
      ev.insert(Cudd_IsComplement(f) ? Cudd_Not(cuddE(F)) : cuddE(F));
    } else {
      tv.insert(f);
      ev.insert(f);
    }
  }

  auto t = cuddBddAndMultiRecur(manager, tv);
  if (NULL == t) return NULL;
  cuddRef(t);

  auto e = cuddBddAndMultiRecur(manager, ev);
  if (NULL == e)
  {
    Cudd_IterDerefBdd(manager, t);
    return NULL;
  }
  cuddRef(e);


  DdNode * r;

  if (t == e)
  {
    r = t;
  } else {
    if (Cudd_IsComplement(t))
    {
      r = cuddUniqueInter(manager, (int) index, Cudd_Not(t), Cudd_Not(e));
      if (NULL == r)
      {
        Cudd_IterDerefBdd(manager, t);
        Cudd_IterDerefBdd(manager, e);
        return NULL;
      }
      r = Cudd_Not(r);
    } else
    {
      r = cuddUniqueInter(manager, (int) index, t, e);
      if (NULL == r)
      {
        Cudd_IterDerefBdd(manager, t);
        Cudd_IterDerefBdd(manager, e);
        return NULL;
      }
    }
  }
  cuddDeref(e);
  cuddDeref(t);
  return r;




  throw std::runtime_error("asfa");
}























