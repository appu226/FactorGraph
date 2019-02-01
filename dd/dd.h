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

typedef struct DdNode * add_ptr;
typedef struct DdNode * bdd_ptr;
typedef std::set<bdd_ptr> bdd_ptr_set;


namespace dd_constants
{
  int const Clip_Up = 1;
  int const Clip_Down = 0;
}



void common_error(void * R, const char * s);

bdd_ptr  bdd_zero (DdManager *);
bdd_ptr  bdd_cube_union (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_and (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_or (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_one (DdManager *);
bdd_ptr  bdd_forsome (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_forall (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_cube_intersection (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_cube_diff (DdManager *, bdd_ptr, bdd_ptr);
bdd_ptr  bdd_dup (bdd_ptr);
bdd_ptr  bdd_support (DdManager *, bdd_ptr);
bdd_ptr  bdd_new_var_with_index (DdManager *, int);
bdd_ptr  bdd_vector_support (DdManager *, bdd_ptr*, int);
bdd_ptr  bdd_cofactor (DdManager *, bdd_ptr, bdd_ptr);
void     bdd_and_accumulate (DdManager *, bdd_ptr *, bdd_ptr);
int      bdd_get_lowest_index (DdManager *, bdd_ptr);
void     bdd_free (DdManager *, bdd_ptr);
bdd_ptr  bdd_not  (DdManager *, bdd_ptr);
int      bdd_is_one (DdManager *, bdd_ptr);
int      bdd_is_zero (DdManager *, bdd_ptr);
void     bdd_or_accumulate (DdManager *, bdd_ptr *, bdd_ptr);
int      bdd_size (DdManager *, bdd_ptr);
void     bdd_print_minterms(DdManager *dd, bdd_ptr f);
bdd_ptr  bdd_xnor(DdManager *dd, bdd_ptr f1, bdd_ptr f2);
bdd_ptr  bdd_and_multi(DdManager *dd, bdd_ptr_set const & funcs);
bdd_ptr  bdd_and_exists_multi(DdManager *dd, bdd_ptr_set const & funcs, bdd_ptr var_cube);
bdd_ptr  bdd_clipping_and_multi(DdManager *dd, bdd_ptr_set const & funcs, int max_depth, int direction);
bdd_ptr  bdd_clipping_and_exists_multi(DdManager *d, bdd_ptr_set const & funcs, bdd_ptr var_cube, int max_depth, int direction);
long double bdd_count_minterm(DdManager * dd, bdd_ptr f, int numVars);
