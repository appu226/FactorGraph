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



#include <stdexcept>
#include "cuddAndAbsMulti.h"
#include <util.h>
#include <cuddInt.h>
#include <algorithm>
#include <float.h>


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



// ***** Function *****
// Recursive "clipping and" implementation
// for more than two input bdds
DdNode * cuddBddClippingAndMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & f,
    int distance,
    int direction);



// ***** Function *****
// Recursive clipping and-abstract implementation
// for more than two input bdds
DdNode * cuddBddClippingAndAbstractMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & f,
    DdNode * cube,
    int distance,
    int direction);



// ***** Function *****
// Recursive model counting for more than two bdds
long double cuddBddCountMintermMultiAux(
    DdManager * manager,
    const std::set<DdNode *> & f,
    long double max);







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
    std::set<DdNode*> const & f)
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
} // end of Cudd_bddAndMulti





/**
  @brief Approximates the conjunction of a set f of BDDs

  @return a pointer to the resulting %BDD if successful; NULL if the
  intermediate result blows up.

  @sideeffect None

  @see Cudd_bddAnd

*/
DdNode *
Cudd_bddClippingAndMulti(
    DdManager * dd,
    std::set<DdNode *> const & f,
    int maxDepth,
    int direction)
{
  DdNode * res;
  do {
    dd->reordered = 0;
    res = cuddBddClippingAndMultiRecur(dd, f, maxDepth, direction);
  } while(1 == dd->reordered);
  if (CUDD_TIMEOUT_EXPIRED == dd->errorCode && dd->timeoutHandler) {
    dd->timeoutHandler(dd, dd->tohArg);
  }
  return res;
} // end of Cudd_bddClippingAndMulti





/**
  @brief Approximates the conjunction of a set f of BDDs and
  simultaneously abstracts the variables in cube.

  @details The variables are existentially abstracted.

  @return a pointer to the resulting %BDD if successful; NULL if the
  intermediate result blows up.

  @sideeffect None

  @see Cudd_bddAndAbstract Cudd_bddClippingAnd

*/
DdNode *
Cudd_bddClippingAndAbstractMulti(
    DdManager * dd,
    std::set<DdNode *> const & f,
    DdNode * cube,
    int maxDepth,
    int direction)
{
  DdNode * res;

  do 
  {
    dd->reordered = 0;
    res = cuddBddClippingAndAbstractMultiRecur(dd, f, cube, maxDepth, direction);
  } while (1 == dd->reordered);


  if (CUDD_TIMEOUT_EXPIRED == dd->errorCode && dd->timeoutHandler)
    dd->timeoutHandler(dd, dd->tohArg);

  return res;
} // end of Cudd_bddClippingAndAbstractMulti



/**
  @brief Returns the number of minterms of a set of %ADD or %BDD as a long double.

  @details On systems where double and long double are the same type,
  Cudd_CountMinterm() is preferable.  On systems where long double values
  have 15-bit exponents, this function avoids overflow for up to 16383
  variables.  It applies scaling to try to avoid overflow when the number of
  variables is larger than 16383, but smaller than 32764.

  @return The nimterm count if successful; +infinity if the number is known to
  be too large for representation as a long double;
  `(long double)CUDD_OUT_OF_MEM` otherwise. 

  @see Cudd_CountMinterm Cudd_EpdCountMinterm Cudd_ApaCountMinterm
*/
long double 
Cudd_LdblCountMintermMulti(
    DdManager * manager,
    const std::set<DdNode *> & funcs,
    int numVars)
{
  long double max = powl(2.0L, (long double) (numVars+LDBL_MIN_EXP));
  if (HUGE_VALL == max)
    throw new std::runtime_error("OOM while counting minterms");


  long double count = cuddBddCountMintermMultiAux(manager, funcs, max);

  if (count >= powl(2.0L, (long double)(LDBL_MAX_EXP + LDBL_MIN_EXP)))
    throw std::runtime_error("min term count is too large to be scaled back");
  else {
    count *= powl(2.0L, (long double)-LDBL_MIN_EXP);
    return count;
  }

}





// *****************************************
// *** Internal api function definitions ***
// *****************************************



// *** Function *****
// Recursive and-abstract implementation
DdNode * cuddBddAndAbstractMultiRecur(
    DdManager * manager,
    std::set<DdNode*> const & fSet2,
    DdNode * cube)
{

  statLine(manager);
  auto one = DD_ONE(manager);
  auto zero = Cudd_Not(one);
  DdNode * r = NULL;

  auto fSet = fSet2;
  fSet.erase(one);

  // Terminal cases.
  // if any of the funcs is zero, return zero
  if (fSet.count(zero) == 1) return zero;

  // if any of the funcs is the not of any other func, return zero
  for (auto f: fSet)
    if (fSet.count(Cudd_Not(f)) > 0)
      return zero;

  // if all of the funcs are one, return one
  if (fSet.empty()) return one;

  // if there is only one element, no more need for conjunction
  if (fSet.size() == 1)
    return cuddBddExistAbstractRecur(manager, *fSet.cbegin(), cube);

  // if cube is empty, return the conjunction
  if (cube == one)
    return cuddBddAndMultiRecur(manager, fSet);

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
    auto remainingCube = cuddT(cube);
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
  auto zero = Cudd_Not(one);

  // Terminal cases
  std::set<DdNode *> fSet = fset;
  fSet.erase(one);
  if (fSet.empty())
    return one;
  if (fSet.size() == 1)
    return *fSet.cbegin();
  if (fSet.size() == 2)
  {
    auto first = fSet.cbegin();
    auto second = fSet.cbegin();
    ++second;
    if (*first == Cudd_Not(*second))
      return zero;
  } 
  if (fSet.count(zero) > 0)
    return zero;

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
  } else
  {
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
} // end cuddBddAndMultiRecur




/**
  @brief Implements the recursive step of Cudd_bddClippingAndMulti

  @details Takes the conjunction of a set of BDDs.

  @return a pointer to the result is successful; NULL otherwise.

  @sideeffect None

  @see cuddBddClippingAnd

*/
DdNode *
cuddBddClippingAndMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & f,
    int distance,
    int direction)
{
  statLine(manager);
  auto one = DD_ONE(manager);
  auto zero = Cudd_Not(one);


  // Terminal cases
  if (f.find(zero) != f.end())
    return zero;
  for (auto fn: f)
    if (f.find(Cudd_Not(fn)) != f.end())
      return zero;
  std::set<DdNode *> f2;
  for (auto fn: f)
    if (fn != one)
      f2.insert(fn);
  if (f2.size() == 1)
    return *f2.begin();
  else if (f2.size() == 0)
    return one;

  if (distance == 0) {
    auto min = *f2.cbegin();
    for (auto fit = f2.cbegin(); NULL != min && fit != f2.cend(); ++fit)
    {
      if (Cudd_bddLeq(manager, min, *fit))
        min = min;
      else if (Cudd_bddLeq(manager, *fit, min))
        min = *fit;
      else
        min = NULL;
    }
    if (NULL != min)
      return min;
    return Cudd_NotCond(one, (direction == 0));
  }

  // at this point, none of the functions are constant
  --distance;

  checkWhetherToGiveUp(manager);

  // here we can skip the use of cuddI,
  // because the operands are known to be non-constant
  int minIndex = Cudd_Regular(*f2.cbegin())->index;
  int minTop = manager->perm[minIndex];
  for (auto felem: f2)
  {
    int index = Cudd_Regular(felem)->index;
    int top = manager->perm[index];
    if (top < minTop)
    {
      minTop = top;
      minIndex = index;
    }
  }

  std::set<DdNode *> ft, fe;
  for (auto felem: f2)
  {
    auto Felem = Cudd_Regular(felem);
    DdNode *th, *el;
    if (Felem->index == minIndex)
    {
      if (Cudd_IsComplement(felem))
      {
        th = Cudd_Not(cuddT(Felem));
        el = Cudd_Not(cuddE(Felem));
      } 
      else
      {
        th = cuddT(Felem);
        el = cuddE(Felem);
      }
    }
    else
    {
      th = felem;
      el = felem;
    }
    ft.insert(th);
    fe.insert(el);
  }

  auto t = cuddBddClippingAndMultiRecur(manager, ft, distance, direction);
  if (NULL == t) return NULL;
  cuddRef(t);
  auto e = cuddBddClippingAndMultiRecur(manager, fe, distance, direction);
  if (NULL == e)
  {
    Cudd_RecursiveDeref(manager, t);
    return NULL;
  }
  cuddRef(e);

  DdNode * result;
  if (t == e)
    result = t;
  else
  {
    if (Cudd_IsComplement(t))
    {
      result = cuddUniqueInter(manager, minIndex, Cudd_Not(t), Cudd_Not(e));
      if (NULL == result)
      {
        Cudd_RecursiveDeref(manager, t);
        Cudd_RecursiveDeref(manager, e);
        return NULL;
      }
      result = Cudd_Not(result);
    }
    else
    {
      result = cuddUniqueInter(manager, minIndex, t, e);
      if (NULL == result)
      {
        Cudd_RecursiveDeref(manager, t);
        Cudd_RecursiveDeref(manager, e);
        return NULL;
      }
    }
  }

  cuddDeref(e);
  cuddDeref(t);
  return result;

} // end of cuddBddClippingAndMultiRecur







/**
  @brief Approximates the AND of a set of BDDs and simultaneously abstracts the
  variables in cube.

  @details The variables are existentially abstracted.

  @return a pointer to the result is successful; NULL otherwise.

  @sideeffect None

  @see Cudd_bddClippingAndAbstract

*/
DdNode *
cuddBddClippingAndAbstractMultiRecur(
    DdManager * manager,
    std::set<DdNode *> const & f,
    DdNode * cube,
    int distance,
    int direction)
{

  statLine(manager);
  auto const one = DD_ONE(manager);
  auto const zero = Cudd_Not(one);

  // Terminal cases
  std::set<DdNode *> f2; // filter out ones
  for (auto felem: f)
  {
    // if any elem is zero return zero
    if (zero == felem) return zero;
    // if any elem is the not of any other lem return zero
    for (auto felem2: f) if (felem == Cudd_Not(felem2)) return zero;
    // filter away ones
    if (one != felem) f2.insert(felem);
  }
  // if no elements then return true
  if (f2.size() == 0) return one;
  // if nothing more to abstract, just compute and
  if (cube == one) return cuddBddClippingAndMultiRecur(manager, f2, distance, direction);
  // if only one element, compute abstraction
  if (f2.size() == 1) return cuddBddExistAbstractRecur(manager, *f2.cbegin(), cube);
  // if distance 0 then just return true or false depending on direction
  if (0 == distance) return Cudd_NotCond(one, (0 == direction));

  // At this point, f2 does not have any constants
  --distance;

  checkWhetherToGiveUp(manager);

  // Here we can skip the use of cuddI, because f2 does not
  // have any constants
  
  // find the topmost variable among all functions
  int minIndex = Cudd_Regular(*f2.cbegin())->index;
  int minTop = manager->perm[minIndex];
  for (auto felem: f2)
  {
    int index = Cudd_Regular(felem)->index;
    int top = manager->perm[index];
    if (top < minTop)
    {
      minIndex = index;
      minTop = top;
    }
  }
  // find the top variable of the abstraction cube
  int topCube = manager->perm[cube->index];

  // if none of the funcs have the top cube variable
  // then we don't need to quantify on this variable
  if (topCube < minTop)
    return cuddBddClippingAndAbstractMultiRecur(
        manager, f2, cuddT(cube), 
        distance, direction);

  // collect then-s and else-s
  std::set<DdNode *> ft, fe;
  for (auto felem: f2)
  {
    int index = Cudd_Regular(felem)->index;
    int top = manager->perm[index];
    if (top == minTop)
    {
      // if top variable needs to be extracted
      auto ftelem = cuddT(Cudd_Regular(felem));
      auto feelem = cuddE(Cudd_Regular(felem));
      if (Cudd_IsComplement(felem))
      {
        ftelem = Cudd_Not(ftelem);
        feelem = Cudd_Not(feelem);
      }
      ft.insert(ftelem);
      fe.insert(feelem);
    }
    else
    {
      // if function is independent of current variable
      ft.insert(felem);
      fe.insert(felem);
    }
  }

  // compute the 'then' part of the result
  auto nextCube = (topCube == minTop) ? cuddT(cube) : cube;
  auto t = cuddBddClippingAndAbstractMultiRecur(manager, ft, nextCube, distance, direction);
  if (NULL == t) return NULL;
  
  // Special case: 
  //     1 OR anything = 1.
  // Hence, no need to compute the else branch if t is 1.
  if (t == one && topCube == minTop)
    return one;

  cuddRef(t);

  // compute the 'else' part of the result
  auto e = cuddBddClippingAndAbstractMultiRecur(manager, fe, nextCube, distance, direction);
  if (NULL == e)
  {
    Cudd_RecursiveDeref(manager, t);
    return NULL;
  }
  cuddRef(e);
  

  if (topCube == minTop)
  {
    // need to abstract
    // so compute the OR of t and e
    std::set<DdNode *> teSet;
    teSet.insert(Cudd_Not(t));
    teSet.insert(Cudd_Not(e));
    DdNode * result = cuddBddClippingAndMultiRecur(
        manager, teSet,
        distance, (direction == 0));
    if (NULL == result)
    {
      Cudd_RecursiveDeref(manager, t);
      Cudd_RecursiveDeref(manager, e);
      return NULL;
    }
    result = Cudd_Not(result);
    cuddRef(result);
    Cudd_RecursiveDeref(manager, t);
    Cudd_RecursiveDeref(manager, e);
    cuddDeref(result);
    return result;
  }
  else if (t == e)
  {
    auto result = t;
    cuddDeref(t);
    cuddDeref(e);
    return result;
  }
  else
  {
    // nothing to abstract, return if-then-else(minIndex, t, e)
    DdNode * result;
    if (Cudd_IsComplement(t))
    {
      result = cuddUniqueInter(manager, minIndex, Cudd_Not(t), Cudd_Not(e));
      if (NULL == result)
      {
        Cudd_RecursiveDeref(manager, t);
        Cudd_RecursiveDeref(manager, e);
        return NULL;
      }
      result = Cudd_Not(result);
    }
    else
    {
      result = cuddUniqueInter(manager, minIndex, t, e);
      if (NULL == result)
      {
        Cudd_RecursiveDeref(manager, t);
        Cudd_RecursiveDeref(manager, e);
        return NULL;
      }
    }
    cuddDeref(t);
    cuddDeref(e);
    return result;
  }
} // end of cuddBddClippingAndAbsMultiRecur



long double cuddBddCountMintermMultiAux(
    DdManager * manager,
    const std::set<DdNode *> & f,
    long double max)
{
  const auto one = DD_ONE(manager);
  const auto zero = Cudd_Not(one);

  // if any of the funcs is false
  // then there are zero solutions
  if (f.count(zero) > 0)
    return 0;

  // filter away true funcs
  // as they cannot affect the answer
  std::set<DdNode *> funcs;
  for (const auto func: f)
    if(func != one)
      funcs.insert(func);

  // no funcs left, so all funcs must have been true
  if (funcs.empty())
    return max;


  // find the earliest variable
  int minIndex = Cudd_Regular(*funcs.cbegin())->index;
  int minTop = manager->perm[minIndex];
  for (const auto func: funcs)
  {
    int index = Cudd_Regular(func)->index;
    int top = manager->perm[index];
    if (top < minTop)
    {
      minTop = top;
      minIndex = index;
    }
  }


  // process the "then" children
  std::set<DdNode *> children;
  for (const auto func: funcs)
  {
    int index = Cudd_Regular(func)->index;
    if (index == minIndex)
    {
      // This is one of the nodes
      // that is splitting on the earliest variable.
      // So we need to take the "then" child
      DdNode * t = cuddT(Cudd_Regular(func));
      if (Cudd_IsComplement(func))
        t = Cudd_Not(t);
      children.insert(t);
    } else
    {
      // This node will split later,
      // so just keep it as it is
      children.insert(func);
    }
  }
  const long double tCount = cuddBddCountMintermMultiAux(manager, children, max);


  // process the "else" children
  children.clear();
  for (const auto func: funcs)
  {
    int index = Cudd_Regular(func)->index;
    if (index == minIndex)
    {
      // split on this node
      DdNode * e = cuddE(Cudd_Regular(func));
      if (Cudd_IsComplement(func))
        e = Cudd_Not(e);
      children.insert(e);
    } else
    {
      // do not split on this node yet
      children.insert(func);
    }
  }
  const long double eCount = cuddBddCountMintermMultiAux(manager, children, max);

  return (tCount * .5) + (eCount * .5);


} // end of cuddBddCountMintermMultiAux

