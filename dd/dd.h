#pragma once

#include <stdio.h>
#include <cudd.h>
#include <set>

typedef struct DdNode * add_ptr;
typedef struct DdNode * bdd_ptr;
typedef std::set<bdd_ptr> bdd_ptr_set;

void common_error(void * R, const char * s);

extern bdd_ptr  bdd_zero (DdManager *);
extern bdd_ptr  bdd_cube_union (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_and (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_or (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_one (DdManager *);
extern bdd_ptr  bdd_forsome (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_forall (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_cube_intersection (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_cube_diff (DdManager *, bdd_ptr, bdd_ptr);
extern bdd_ptr  bdd_dup (bdd_ptr);
extern bdd_ptr  bdd_support (DdManager *, bdd_ptr);
extern bdd_ptr  bdd_new_var_with_index (DdManager *, int);
extern bdd_ptr  bdd_vector_support (DdManager *, bdd_ptr*, int);
extern bdd_ptr  bdd_cofactor (DdManager *, bdd_ptr, bdd_ptr);
extern void     bdd_and_accumulate (DdManager *, bdd_ptr *, bdd_ptr);
extern int      bdd_get_lowest_index (DdManager *, bdd_ptr);
extern void     bdd_free (DdManager *, bdd_ptr);
extern bdd_ptr  bdd_not  (DdManager *, bdd_ptr);
extern int      bdd_is_one (DdManager *, bdd_ptr);
extern int      bdd_is_zero (DdManager *, bdd_ptr);
extern void     bdd_or_accumulate (DdManager *, bdd_ptr *, bdd_ptr);
extern int      bdd_size (DdManager *, bdd_ptr);
extern void     bdd_print_minterms(DdManager *dd, bdd_ptr f);
extern bdd_ptr  bdd_xnor(DdManager *dd, bdd_ptr f1, bdd_ptr f2);
extern bdd_ptr  bdd_and_exists_multi(DdManager *dd, bdd_ptr_set const & funcs, bdd_ptr var_cube);
