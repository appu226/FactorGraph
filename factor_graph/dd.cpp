#include "dd.h"
#include <stdlib.h>

void common_error(void * R, const char * s)
{
  if(R == NULL)
  {
    printf("%s\n", s);
    fflush(stdout);
    exit(1);
  }
}


/**Function********************************************************************

  Synopsis    [Computes f constrain c.]

  Description [Computes f constrain c (f @ c).
  Uses a canonical form: (f' @ c) = ( f @ c)'.  (Note: this is not true
  for c.)  List of special cases:
    <ul>
    <li> F @ 0 = 0
    <li> F @ 1 = F
    <li> 0 @ c = 0
    <li> 1 @ c = 1
    <li> F @ F = 1
    <li> F @ F'= 0
    </ul>
  Returns a pointer to the result if successful; a failure is
  generated otherwise.] 

  SideEffects []

  SeeAlso     [bdd_minimize bdd_simplify_assuming]

******************************************************************************/
bdd_ptr bdd_cofactor(DdManager * dd, bdd_ptr f, bdd_ptr g)
{
  DdNode *result;

  /* We use Cudd_bddConstrain instead of Cudd_Cofactor for generality. */
  result = Cudd_bddConstrain(dd, (DdNode *)f, (DdNode *)g);
  common_error(result, "bdd_cofactor: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
} /* end of bdd_cofactor */

/**Function********************************************************************

  Synopsis    [Finds the variables on which a set of BDDs depends.]

  Description [Finds the variables on which a set of BDDs depends.
  The set must contain BDDs.
  Returns a BDD consisting of the product of the variables if
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [bdd_support]

******************************************************************************/
bdd_ptr bdd_vector_support(DdManager *dd, bdd_ptr *fn, int n)
{
  DdNode *result;
  result = Cudd_VectorSupport(dd, (DdNode **)fn, n);
  common_error(result, "bdd_vectorSupport: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis           [Returns the BDD variable with index <code>index</code>.]

  Description        [Retrieves the BDD variable with index <code>index</code>
  if it already exists, or creates a new BDD variable. Returns a
  pointer to the variable if successful; a failure is generated
  otherwise. The returned value is referenced.]

  SideEffects        []

  SeeAlso            [bdd_new_var_at_level add_new_var_at_level]

******************************************************************************/
bdd_ptr bdd_new_var_with_index(DdManager * dd, int index)
{
  DdNode * result;
  
  result = Cudd_bddIthVar(dd, index);
  Cudd_Ref(result);
  common_error(result, "bdd_new_var_with_index: result = NULL");
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis    [Finds the variables on which an BDD depends on.]

  Description [Finds the variables on which an BDD depends on.
  Returns an BDD consisting of the product of the variables if
  successful; a failure is generated otherwise.]

  SideEffects []

  SeeAlso     [add_support]

*****************************************************************************/
bdd_ptr bdd_support(DdManager *dd, bdd_ptr fn)
{
  DdNode * result;
  
  result = Cudd_Support(dd, (DdNode *)fn);
  common_error(result, "bdd_support: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis           [Creates a copy of an BDD node.]

  Description        [Creates a copy of an BDD node.]

  SideEffects        [The reference count is increased by one unit.]

  SeeAlso            [bdd_ref bdd_free bdd_deref]

******************************************************************************/
bdd_ptr bdd_dup(bdd_ptr dd_node)
{
  Cudd_Ref(dd_node);
  return(dd_node);
}

/**Function********************************************************************

  Synopsis           [Reads the constant 0 BDD of the manager.]

  Description        [Reads the constant 0 BDD of the manager.]

  SideEffects        []

  SeeAlso            [bdd_one]
******************************************************************************/
bdd_ptr bdd_zero(DdManager * dd)
{
  DdNode * result = Cudd_ReadLogicZero(dd);

  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis    [Computes the union between two BDD cubes.]

  Description [Computes the union between two BDD cubes, i.e. the
  cube of BDD variables belonging to cube a OR to cube b. 
  Returns a pointer to the resulting cube; a failure is generated
  otherwise.]

  SideEffects []

  SeeAlso     [bdd_cube_intersection,bdd_and]

******************************************************************************/
bdd_ptr bdd_cube_union(DdManager * dd, bdd_ptr a, bdd_ptr b)
{
  bdd_ptr result;
  result = bdd_and(dd,a,b);
  common_error(result, "bdd_cube_union: result = NULL");
  return(result);
}

/**Function********************************************************************

  Synopsis    [Applies OR to the corresponding discriminants of f and g.]

  Description [Applies logical OR to the corresponding discriminants
  of f and g. f and g must be BDDs. Returns a pointer to the result if
  successful; a failure is generated otherwise.]

  SideEffects []

  SeeAlso     [bdd_and bdd_xor bdd_not]

******************************************************************************/
bdd_ptr bdd_or(DdManager * dd, bdd_ptr a, bdd_ptr b)
{
  DdNode * result;

  result = Cudd_bddOr(dd, (DdNode *)a, (DdNode *)b);
  common_error(result, "bdd_or: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis    [Applies AND to the corresponding discriminants of f and g.]

  Description [Applies logical AND to the corresponding discriminants
  of f and g. f and g must be BDDs. Returns a pointer to the result if
  successful; a failure is generated otherwise.]

  SideEffects []

  SeeAlso     [bdd_or bdd_xor bdd_not]

******************************************************************************/
bdd_ptr bdd_and(DdManager * dd, bdd_ptr a, bdd_ptr b)
{
  DdNode * result;

  result = Cudd_bddAnd(dd, (DdNode *)a, (DdNode *)b);
  common_error(result, "bdd_and: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis           [Reads the constant 1 BDD of the manager.]

  Description        [Reads the constant 1 BDD of the manager.]

  SideEffects        []

  SeeAlso            [bdd_zero]
******************************************************************************/
bdd_ptr bdd_one(DdManager * dd)
{
  DdNode * result = Cudd_ReadOne(dd);

  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis [Universally abstracts all the variables in cube from fn.]

  Description [Universally abstracts all the variables in cube from fn.
  Returns the abstracted BDD if successful; a failure is generated
  otherwise.]

  SideEffects []

  SeeAlso     [bdd_forsome]

******************************************************************************/
bdd_ptr bdd_forall(DdManager * dd, bdd_ptr fn, bdd_ptr cube)
{
  DdNode * result;
  DdNode * notfn;

  notfn = bdd_not(dd, fn);
  result = Cudd_bddExistAbstract(dd, (DdNode *)notfn, (DdNode *)cube);
  common_error(result, "bdd_forsome: result = NULL");
  Cudd_Ref(result);
  bdd_free(dd, notfn);
  notfn = bdd_not(dd, result);
  bdd_free(dd, result);
  return((bdd_ptr)notfn);
}

/**Function********************************************************************

  Synopsis [Existentially abstracts all the variables in cube from fn.]

  Description [Existentially abstracts all the variables in cube from fn.
  Returns the abstracted BDD if successful; a failure is generated
  otherwise.]

  SideEffects []

  SeeAlso     [bdd_forall]

******************************************************************************/
bdd_ptr bdd_forsome(DdManager * dd, bdd_ptr fn, bdd_ptr cube)
{
  DdNode * result;

  result = Cudd_bddExistAbstract(dd, (DdNode *)fn, (DdNode *)cube);
  common_error(result, "bdd_forsome: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis    [Computes the intersection between two BDD cubes.]

  Description [Computes the difference between two BDD cubes, i.e. the
  cube of BDD variables belonging to cube a AND belonging to cube
  b. Returns a pointer to the resulting cube; a failure is generated
  otherwise.]

  SideEffects []

  SeeAlso     [bdd_cube_union,bdd_cube_diff]

******************************************************************************/
bdd_ptr bdd_cube_intersection(DdManager * dd, bdd_ptr a, bdd_ptr b)
{
  bdd_ptr result,tmp;
  tmp = bdd_cube_diff(dd , a , b);
  result= bdd_cube_diff(dd , a , tmp);
  bdd_free(dd,tmp);
  common_error(result, "bdd_cube_intersection: result = NULL");
  return(result);
}

/**Function********************************************************************

  Synopsis    [Computes the difference between two BDD cubes.]

  Description [Computes the difference between two BDD cubes, i.e. the
  cube of BDD variables belonging to cube a and not belonging to cube
  b. Returns a pointer to the resulting cube; a failure is generated
  otherwise.]

  SideEffects []

  SeeAlso     [add_cube_diff]

******************************************************************************/
bdd_ptr bdd_cube_diff(DdManager * dd, bdd_ptr a, bdd_ptr b)
{
  DdNode * result;

  result = Cudd_bddExistAbstract(dd, (DdNode *)a, (DdNode *)b);
  common_error(result, "bdd_cube_diff: result = NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis    [Applies AND to the corresponding discriminants of f and g.]

  Description [Applies logical AND to the corresponding discriminants
  of f and g and stores the result in f. f and g must be two BDDs. The
  result is referenced.]

  SideEffects [The result is stored in the first operand and referenced.]

  SeeAlso     [bdd_and]

******************************************************************************/
void bdd_and_accumulate(DdManager * dd, bdd_ptr * a, bdd_ptr b)
{
  DdNode * result;

  result = Cudd_bddAnd(dd, (DdNode *)*a, (DdNode *)b);
  common_error(result, "bdd_and_accumulate: result = NULL");
  Cudd_Ref(result);
  Cudd_RecursiveDeref(dd, (DdNode *)*a);
  *a = result;
  return;
}

/**Function********************************************************************

  Synopsis           [Returns the index of the lowest variable in the BDD a.]

  Description        [Returns the index of the lowest variable in the
  BDD, i.e. the variable in BDD a with the highest position in the
  ordering. ]

  SideEffects        []

******************************************************************************/
int bdd_get_lowest_index(DdManager * dd, bdd_ptr a)
{
  int result;
  
  result = Cudd_NodeReadIndex((DdNode *)a);
  return(result);
}

/**Function********************************************************************

  Synopsis           [Dereference an BDD node. If it dies, recursively decreases
  the reference count of its children.]

  Description        [Decreases the reference count of node. If the node dies,
  recursively decreases the reference counts of its children. It is used to
  dispose off a BDD that is no longer needed.]

  SideEffects        [The reference count of the node is decremented by one,
  and if the node dies a recursive dereferencing is applied to its children.]

  SeeAlso            []
******************************************************************************/
void bdd_free(DdManager * dd, bdd_ptr dd_node) 
{
  common_error(dd_node, "bdd_free: dd_node = NULL");
  Cudd_RecursiveDeref(dd, (DdNode *)dd_node);
}


/**Function********************************************************************

  Synopsis    [Applies NOT to the corresponding discriminant of f.]

  Description [Applies logical NOT to the corresponding discriminant of f.
  f must be a BDD. Returns a pointer to the result if successful; a
  failure is generated otherwise.]

  SideEffects []

  SeeAlso     [bdd_and bdd_xor bdd_or bdd_imply]

******************************************************************************/
bdd_ptr bdd_not(DdManager * dd, bdd_ptr fn)
{
  DdNode * result;

  result = Cudd_Not(fn);
  common_error(result, "bdd_not: result == NULL");
  Cudd_Ref(result);
  return((bdd_ptr)result);
}

/**Function********************************************************************

  Synopsis           [Check il the BDD is one.]

  Description        [Check il the BDD is one.]

  SideEffects        []

  SeeAlso            [bdd_one]
******************************************************************************/
int bdd_is_one(DdManager * dd, bdd_ptr f)
{
  return((DdNode *)f == Cudd_ReadOne(dd));
}

int bdd_is_zero(DdManager * dd, bdd_ptr f)
{
  return((DdNode *)f == Cudd_ReadLogicZero(dd));
}

/**Function********************************************************************

  Synopsis    [Applies OR to the corresponding discriminants of f and g.]

  Description [Applies logical OR to the corresponding discriminants
  of f and g and stores the result in f. f and g must be two BDDs. The
  result is referenced.]

  SideEffects [The result is stored in the first operand and referenced.]

  SeeAlso     [bdd_and]

******************************************************************************/
void bdd_or_accumulate(DdManager * dd, bdd_ptr * a, bdd_ptr b)
{
  DdNode * result;

  result = Cudd_bddOr(dd, (DdNode *)*a, (DdNode *)b);
  common_error(result, "bdd_or_accumulate: result = NULL");
  Cudd_Ref(result);
  Cudd_RecursiveDeref(dd, (DdNode *) *a);
  *a = result;
  return;
}

/**Function********************************************************************

  Synopsis    [Counts the number of BDD nodes in an BDD.]

  Description [Counts the number of BDD nodes in an BDD. Returns the number
  of nodes in the graph rooted at node.]

  SideEffects []

  SeeAlso     [bdd_count_minterm]

******************************************************************************/
int bdd_size(DdManager * dd, bdd_ptr fn)
{
  return(Cudd_DagSize((DdNode *)fn));
}

/**Function********************************************************************

  Synopsis    [Prints a disjoint sum of products.]

  Description [Prints a disjoint sum of product cover for the function
  rooted at node. Each product corresponds to a path from node to a
  leaf node different from the logical zero, and different from the
  background value. Uses the package default output file.  Returns 1
  if successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_PrintDebug Cudd_bddPrintCover]

******************************************************************************/
void bdd_print_minterms(DdManager *dd, bdd_ptr f)
{
  int i;
  i = Cudd_PrintMinterm(dd, f);
  if(i == 0)
    common_error(NULL, "bdd_print_minterms : failed\n");
}
