#ifndef _SRT_H
#define _SRT_H

#include <list>
#include <queue>
#include <stack>
#include <cstring>
#include <string>
#include "qbf.h"

class SRT;
class SRTNode;
class SRTChildren;
class SRTLeaf;

typedef std::queue<SRTNode*>         SRTNode_Queue;
typedef std::stack<SRTNode*>         SRTNode_Stack;
typedef std::vector<SRTNode*>        SRTNode_List;
typedef std::vector<SRTNode_List>    SRTList_List;

class SRTChildren {
  public:
    SRTNode* t;	// Then child
    SRTNode* e;	// Else child
};

class SRTLeaf {
  public:
    BDD_List* func;			// Array of BDD's associated with this leaf node
    //    Vector_Int* dependency;	// Array of indices of variables on which this leaf depends

    /* Functions of SRTLeaf */
    //    void build_dependency(DdManager *m, BDD_List* added_bdds = NULL);
    void set_func(DdManager *m, BDD_List* newfunc);
    void add_func(DdManager *m, BDD_List *addedfunc);
    bool check_func(DdManager *m, bdd_ptr check);
    void print(DdManager *m);
};

class SRTNode {
  public:
    int qstart;		// start index of quantifiers associated with this SRT Node
    int splitvar;	// last bit: 0 if this is leaf node, 1 otherwise
                  // if this bit is 1, then remaining bits give the index of variable which is split
    SRTNode *parent;
    DdManager *m;
    union {						// Every node is either leaf or an internal node
      SRTChildren children;
      SRTLeaf     leaf;
    } type;


    // TEMPORARY
    ~SRTNode()
    {
      if(is_leaf())
      {
        for(int i = 0; i < type.leaf.func->size(); i++)
          bdd_free(m, type.leaf.func->at(i));
        delete type.leaf.func;
        //	delete type.leaf.dependency;
      } else
      {
        delete type.children.t;
        delete type.children.e;
      }
    }

    SRTNode(DdManager *m) {
      qstart = 0;
      splitvar = 0;
      type.leaf.func = new BDD_List();
      //      type.leaf.dependency = new Vector_Int();
      this->m = m;
      parent = NULL;
    }

    /* Functions on an SRTNode which can be performed without changing the table */
    int is_leaf () const;

    /* Functions on non-leaf nodes */
    int get_split_var() const;
    int is_quant_operator() const;
    int is_pack_operator() const;
    void apply_operator(boolop op);
    SRTNode* SRT_Then();
    SRTNode* SRT_Else();
    void merge(Quant* q, int threshold, boolop op);		// This must have been split on the innermost variable 
    void merge_folding (Quant *q, int threshold, boolop op);
    void merge_heuristic (Quant* q, int threshold, boolop op);
    // Apply merge operation to quantify out the innermost variable
    void form_final_tree(Quant* q, SRTNode* t, SRTNode* e, Deduction_List& dl, boolop op, int threshold);
    //    void synchronize_ordering(FreqMap &freqinfo);
    //    void pull_up_split_var(int var);
    //    int get_child_qstart();
    //    void set_child_qstart();

    /* Functions on leaf nodes */
    //    int no_dependent() const;
    //    int is_dependent (int v) const;
    BDD_List* get_func();
    void for_all_innermost(DdManager* d, Quant* q);	// quantifies out innermost universal block of q from leaf node n
    void putBDD(BDD_List* func);	// replace functions by those in argument
    void append(BDD_List* func);	// appends given functions represented as BDD's to the leaf node
    void append(SRTNode* n);	// appends given functions represented as BDD's to the leaf node
    void do_append(SRTNode* n);	// appends given functions represented as BDD's to the leaf node
    //    void append(SRTNode *node, Vector_Int &to_split, Vector_Int &already_split);
    void apply_deduction(Deduction d);
    void deduce();						// finds and applies deductions
    void split(int var);			// Split this leaf node on variable var
    void join_bdds(DdManager*);

    /* Functions on nodes and formulae */
    //	Formula* generate_formula();

    /* Functions for printing */
    void print_indent(int level);

    /* Other Functions */
    void find_frequency(FreqMap& freqinfo, Deduction_List& dl);
    BDD_List* evaluate(Quant* q, Deduction_List& dl);

};


class SRT {
  public:
    Quant* quant_info;				       // Maintains variables and their quantifiers in order
    DdManager* ddm;						       // DdManager of all Cudd operations
    SRTNode* root;						       // Root node of the tree
    CNF_vector* global_clauses;			 // Global CNF_vector for storing implicitly appended clauses
    SRTNode_List* leaves;				     // Queue of leaves
    SRTList_List* split_list;			   // Queues of nodes split on a given variable for all variables in quantifier order
    //  This is just an optimization
    //  Have to analyze how optimal this will be

    // TEMPORARY
    SRT(int n);
    void split(std::string splitnode, int var);

    /* Constructor */
    SRT();

    /* Denstructor */
    ~SRT();

    /* Functions on the whole SRT Table */
    void parse_qdimacs_srt(char*);
    Quant* SRT_vars ();
    CNF_vector* SRT_clauses ();
    SRTNode* SRT_getroot ();
    DdManager* SRT_getddm();
    //    SRTNode* SRT_split (SRTNode* n, int var);
    void SRT_merge (int v, int threshold);
    void SRT_print();
};



/* Functions of deduction */
int is_true(Deduction d);
int make_true_deduction(int var);
int make_false_deduction(int var);
int deduction_give_var(Deduction d);

/* Functions of BDD's */
Deduction_List BDD_deduce(bdd_ptr b);			// sets deductions in d and returns number of deductions
BDD_List* BDD_op(DdManager *m, boolop op, BDD_List* bdd1, BDD_List* bdd2);
bdd_ptr BDD_not(bdd_ptr b);
void BDD_for_all_innermost(DdManager*, bdd_ptr&, Quant*);
Vector_Int BDD_support_set(DdManager *m, bdd_ptr bdd);


/* global function */
SRTNode* make_leaf(DdManager *d, CNF_vector* CV);					// Create a temporary leaf out of CV, its not a leaf of tree, qstart = 0, splitvar = 0
//SRTNode * merge_nodes(DdManager *m, SRTNode *n1, SRTNode *n2, boolop op);

/* factor_graph functions */
factor_graph *make_factor_graph(DdManager * ddm, SRTNode * node);
void factor_graph_eliminate(factor_graph*fg, int start, int end, SRTNode *curr);
void factor_graph_eliminate(factor_graph*fg, fgnode *n, SRTNode *curr);

/* profiling */
void profile();
#endif


