#ifndef _DS_H
#define _DS_H

#include <vector>
#include <set>
#include <map>
#include "factor_graph.h"

class QBlock;
class Quant;
class MClause;
class CNF_vector;

typedef enum {AND, OR}               boolop;
typedef int Deduction;			// last bit is 0 if ~v and 1 if its v, remaining bits represent quantifier order of v
typedef std::vector<QBlock*>         QBlock_List;
typedef std::vector<bdd_ptr>         BDD_List;
typedef BDD_List::iterator           BDD_Iterator;
typedef BDD_List::reverse_iterator   BDD_RIterator;
typedef std::vector<int>             Vector_Int;
typedef std::vector<MClause*>        MClause_List;
typedef std::set<int>                Set_Int;
typedef std::vector<Deduction>       Deduction_List;
typedef std::map<int,int>            FreqMap;

// variables are represented by quantifier ordering

class QBlock {
  public:
    int is_universal;		// (just for optimization)
    int start;				// variables in the quantifier block
    int end;
};

/* Maintains variables and their quantifiers in order */
class Quant {
  public:
    QBlock_List blocks;		// List of blocks from outermost to innermost
    BDD_List vars;			// array of variables (should be kept with QBlock?) , arranged in quantifier order
    int* dimacs_srt;			// dimacs_srt[i] = +/- j , if i is the dimacs file index , j is quantifier order, + if universal, - if existential
    int* srt_dimacs;			// reverse mapping, quant_bdd[i] = +/- j , if i is the quantifier order , j is bddindex, + if universal, - if existential
    DdManager *ddm;

    /* Functions */

    // TEMPORARY
    Quant();
    Quant(DdManager* ddm, int n);
    ~Quant();
    bool empty();
    int no_vars();
    QBlock* innermost();
    void remove_innermost();
    int is_universal_innermost();
    int is_existential_innermost();
    int is_universal(int v);
    int is_existential(int v);
    bdd_ptr get_var_quant_index (int quant_index);
    void print();
};

class MClause {
  public:
    Vector_Int vars;		// +x if positive literal, -x if ~x

    /* Functions */
    int no_vars();
    void drop_vars(int start, int end);
    void print();

};

class CNF_vector {
  public:
    MClause_List clauses;

    /* Constructor */
    CNF_vector();
    CNF_vector(int);

    /* Functions of CNF_vector */
    int no_clauses();
    void drop_vars(int start, int end); 		// drops (universally quantifes) variables in vars
    //		CNF_vector* filter(Vector_Int& vars);	// returns the set of clauses that depend on V, and deletes them,
    CNF_vector* filter_innermost(Quant* q);		// filters clauses depending variables from innermost
    // quantifier block of q, ideally only this is required
    Vector_Int* support_set();				// returns the variables in the support set
    void print();
};

/* BDD-CNF Conversions */
CNF_vector* BDD_List_to_CNF (DdManager* d, Quant* q, BDD_List* bdds);
CNF_vector* BDD_to_CNF (DdManager* d, Quant* q, bdd_ptr bdd);
CNF_vector* BDD_to_CNF_helper (DdManager* d, Quant* q, bdd_ptr bdd, bool val);
bdd_ptr CNF_to_BDD (CNF_vector* cnf, DdManager *m);
BDD_List* CNF_to_BDD_List(CNF_vector *cnf, DdManager *dd);

/* CNF-BLIF Conversions */
void CNF_to_BLIF(CNF_vector* cnf, const char* filename);
CNF_vector* BLIF_to_CNF (const char* input, int start_tempvar);

/* BDD-BLIF Conversions */
void write_blif(DdManager* d, Quant* q, BDD_List* bdds, const char* filename);
#endif
