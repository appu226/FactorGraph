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

#pragma once

#include <stdio.h>
#include <cudd.h>
#include <set>

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

DdNode * Cudd_bddAndAbstractMulti(DdManager *manager, const std::set<DdNode *> & f, DdNode *cube, int multiCacheCapacity);
DdNode * Cudd_bddAndMulti(DdManager *manager, const std::set<DdNode *> & f, int multiCacheCapacity);
DdNode * Cudd_bddClippingAndMulti(DdManager *manager, 
                                  const std::set<DdNode *> & f, 
                                  int maxDepth, 
                                  int direction);
DdNode * Cudd_bddClippingAndAbstractMulti(DdManager *manager, 
                                          const std::set<DdNode *> & f, 
                                          DdNode *cube, 
                                          int maxDepth, 
                                          int direction);
long double Cudd_LdblCountMintermMulti(DdManager * dd, const std::set<DdNode *> & funcs, int numVars, int multiCacheCapacity);
