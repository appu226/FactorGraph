#include <stdio.h>
#include <cudd.h>
#include <set>

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

DdNode * Cudd_bddAndAbstractMulti(DdManager *manager, std::set<DdNode *> const & f, DdNode *cube);
DdNode * Cudd_bddAndMulti(DdManager *manager, std::set<DdNode *> const & f);
