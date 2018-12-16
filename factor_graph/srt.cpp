#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include "srt.h"

#ifdef DEBUG
#define DBG(stmt) stmt
#else
#define DBG(stmt) (void)0
#endif

#define absolute(x) (x<0 ? -x : x)

/*extern double time_message_passing, time_var_node, time_func_node, time_find_parent, time_combine, time_exist;

void profile() {
  std::cout<<"Time for message passing: "<<time_message_passing<<std::endl;
  std::cout<<"Time in varnode message passing: "<<time_var_node<<std::endl;
  std::cout<<"Time in funcnode message passing: "<<time_func_node<<std::endl;
  std::cout<<"Time for finding parent: "<<time_find_parent<<std::endl;
  std::cout<<"Time for combining messages: "<<time_combine<<std::endl;
  std::cout<<"Time for existential quantification: "<<time_exist<<std::endl;
  return;
}*/

void remove_comments(std::ifstream& fin)
{
  char c;
  fin>>c;
  std::string s;
  //DBG(std::cout<<"c is "<<c<<std::endl);
  while(c == 'c') {
    getline(fin, s);
//    DBG(std::cout<<"removed "<<c<<s<<std::endl);
    fin>>c;
  }
  fin.unget();
  return;
}

void read_FV(std::ifstream& fin, int &F, int &V)
{
  char c;
  std::string s;
  fin>>c>>s;
  if(c != 'p' || s != "cnf") {
    std::cerr<<"error in reading F V"<<std::endl;
//    DBG(std::cerr<<"c is "<<c<<"\nand s is "<<s<<std::endl);
    exit(-1);
  }
  fin>>V>>F;
}

DdManager* ddm_init()
{
  DdManager *m;
  //UNIQUE_SLOTS = 256
  //CACHE_SLOTS = 262144
  m = Cudd_Init(0, 0, 256, 262144, 0);
  common_error(m, "srt.cpp : Could not initialize DdManager\n");
  return m;
}


/* Functions on an SRTNode which can be performed without changing the table */
int SRTNode::is_leaf () const {
  return 1-(splitvar & 0x1);
}

/* Functions on non-leaf nodes */
int SRTNode::get_split_var() const {
  if(is_leaf())
    return -1;
  return (splitvar >> 1);
}

int SRTNode::is_quant_operator() const {
  assert(!is_leaf());
  return (get_split_var() >= qstart);
}

int SRTNode::is_pack_operator() const {
  assert(!is_leaf());
  return (1-is_quant_operator());
}

  SRTNode* SRTNode::SRT_Then() {
    if(is_leaf())
      return NULL;
    return (type.children.t);
  }

  SRTNode* SRTNode::SRT_Else() {
    if(is_leaf())
      return NULL;
    return (type.children.e);
  }

/* Functions on leaf nodes */
/*int SRTNode::no_dependent() const {
  if(is_leaf()) {
    return type.leaf.dependency->size();
  }
  return -1;
}

int SRTNode::is_dependent(int v) const {
  int i;
  int tot_dep = no_dependent();
  Vector_Int* dep = type.leaf.dependency;
  assert(tot_dep >= 0);
  for(i=0; i<tot_dep; i++) {
    if(v == (*dep)[i])
      return 1;
  }
  return 0;
}*/

BDD_List* SRTNode::get_func() {
  if(is_leaf()) {
    return type.leaf.func;
  }
  return NULL;
}

void SRTNode::for_all_innermost(DdManager* d, Quant* q) { // quantifies out innermost universal block of q from leaf node n
  assert(is_leaf());
  BDD_List* bdds = get_func();
  for(BDD_Iterator iter = bdds->begin(); iter!=bdds->end(); ++iter ) {
    BDD_for_all_innermost(d,*iter,q);
  }
  return;
}

void SRTLeaf::add_func(DdManager *m, BDD_List *addedfunc)
{
  for(int i = 0; i < addedfunc->size(); i++)
    func->push_back(bdd_dup(addedfunc->at(i)));
//  build_dependency(m, addedfunc);
}

void SRTLeaf::set_func(DdManager *m, BDD_List *newfunc)
{
  func = new BDD_List(*newfunc);
//  build_dependency(m);
}

bool SRTLeaf::check_func(DdManager *m, bdd_ptr check) {
  bdd_ptr temp = bdd_one(m), temp2;
  for(int i=0;i<func->size();i++) {
    temp2 = bdd_and(m,temp,(*func)[i]);
    bdd_free(m,temp);
    temp = temp2;
  }
  bool ans = (temp == check);
  bdd_free(m,temp);
  return ans;
}

void SRTLeaf::print(DdManager *m) {
  printf("\n\nPRINTING LEAF:\n");
  for(int i=0;i<func->size();i++) {
    printf("BDD %d\n",i);
    bdd_print_minterms(m,(*func)[i]);
  }
  printf("\n\n");
  return;
}

/*void set_func(CNF_vector *cnfv, DdManager *m)
  {
  BDD_List * = CNF_to_BDD();
  }*/


/*void SRTLeaf::build_dependency(DdManager *m, BDD_List* added_bdd) {
//  if(dependency != NULL && added_bdd == NULL) {
//  delete dependency;
//  dependency = NULL;
//  dependency = new Vector_Int();
//  }
  if(added_bdd == NULL) {
    added_bdd = func;
  }


  for(BDD_Iterator iter = added_bdd->begin(); iter!=added_bdd->end(); ++iter) {
    Vector_Int ss = BDD_support_set(m, *iter);
    for(Vector_Int::iterator it = ss.begin(); it!=ss.end(); ++it) {
      int found = 0;
      for(Vector_Int::iterator it2 = dependency->begin(); it2!=dependency->end(); ++it2) {
  if(*it == *it2) {
    found = 1;
    break;
  }
      }
      if(!found)
  dependency->push_back(*it);
    }
  }
  return;
}*/

void SRTNode::putBDD(BDD_List* func) {  // replace functions by those in argument
  assert(is_leaf());
  type.leaf.func = func;

  // TEMPORARY
  if(func == NULL)
    type.leaf.func = new BDD_List();

//  free(type.leaf.dependency);
//  type.leaf.build_dependency(m);
}

void SRTNode::append(BDD_List* added_bdds) {  // appends given functions represented as BDD's to the leaf node
  assert(is_leaf());
  if(added_bdds == NULL)
    return;
  for(int i = 0; i < added_bdds->size(); i++)
    type.leaf.func->push_back(bdd_dup(added_bdds->at(i)));      // bdd_dup is necessary since each use must get one ref. Hence, can't just do vector->insert(vect->end(),added_bdds->begin(),added_bdds->end());
//  type.leaf.build_dependency(m, added_bdds);
  return;
}

void SRTNode::append(SRTNode* node) { 
  
  // Sanity Check: This leaf shouldn't be dependent on and thus shouldn't have a split in ancestor on any variable in node
  Set_Int already_split;
  SRTNode* ancestor = this->parent;
  while(ancestor != NULL) {
    already_split.insert(ancestor->get_split_var());
    ancestor = ancestor->parent;
  }
  
  Set_Int node_split;
  SRTNode_Queue queue;
  queue.push(node);
  while(!queue.empty()) {
    SRTNode* curr = queue.front();
    queue.pop();
    if(curr->is_leaf())
      continue;
    node_split.insert(curr->get_split_var());
    queue.push(curr->SRT_Then());
    queue.push(curr->SRT_Else());
  } 
  
  Set_Int intersect;
  set_intersection(already_split.begin(), already_split.end(), node_split.begin(), node_split.end(), inserter(intersect,intersect.begin()) );
  assert(intersect.size() == 0);
  
  do_append(node);
  return;
}

void SRTNode::do_append(SRTNode* node) {
  if(node->is_leaf()) {
      append(node->get_func());
      return;
  }
  split(node->get_split_var());
  SRT_Then()->append(node->SRT_Then());
  SRT_Else()->append(node->SRT_Else());
  return;
}
  
  
/*void SRTNode::append(SRTNode* node) { 
  // appends given functions represented as BDD's to the leaf node
  if(node->is_leaf()) {
    append(node->get_func());
  }
  //Vector_Int must_have_split;
  int must_have_split;
  Vector_Int already_split; // maintains splits from root to this node
  Vector_Int to_split;
  SRTNode *ancestor = this;
  while(ancestor->parent != NULL)
  {
    if(ancestor == ancestor->parent->type.children.t)
      already_split.push_back(ancestor->parent->get_split_var());
    else
    {
      assert(ancestor == ancestor->parent->type.children.e);
      already_split.push_back(ancestor->parent->get_split_var() * -1);
    }
    ancestor = ancestor->parent;
  }
  ancestor = node->parent;
  while(ancestor != NULL)
  {
    must_have_split = ancestor->get_split_var();
    to_split.push_back(must_have_split);
    for(Vector_Int::iterator i = already_split.begin(); i != already_split.end(); i++)
    {
      if(absolute(*i) == must_have_split)
      {
  to_split.pop_back();
  break;
      }
    }
    ancestor = ancestor->parent;
  }

  append(node, to_split, already_split);

  return;
}

void SRTNode::append(SRTNode *node, Vector_Int &to_split, Vector_Int &already_split)
{
  if(to_split.size() > 0)
  {
    split(to_split.back());
    to_split.pop_back();
    this->type.children.t->append(node, to_split, already_split);
    this->type.children.e->append(node, to_split, already_split);
    return;
  }
  if(!node->is_leaf())
  {
    int splvar = node->get_split_var();
    for(Vector_Int::iterator i = already_split.begin(); i != already_split.end(); i++)
    {
      if(absolute(*i) == splvar)
      {
  int j = *i;
  to_split.erase(i);
  if(j > 0)
    append(node->type.children.t, to_split, already_split);
  else
    append(node->type.children.e, to_split, already_split);
  return;
      }
    }
    split(splvar);
    this->type.children.t->append(node->type.children.t, to_split, already_split);
    this->type.children.e->append(node->type.children.e, to_split, already_split);
    return;
  }
  append(node->get_func());

}*/

void SRTNode::split(int var) {

  bdd_ptr vava = bdd_new_var_with_index(this->m, var);
  bdd_ptr notvava = bdd_not(this->m, vava);
  assert(is_leaf());
//  DBG(std::cout<<var<<" "<<is_leaf()<<std::endl);

  SRTNode* t = new SRTNode(m);
  SRTNode* e = new SRTNode(m);

  int newstart = (var < qstart ? qstart : var+1); // If split is on quantified variable, then new start else original
  t->qstart = e->qstart = newstart; 
  t->splitvar = e->splitvar = 0;    // These are leaf nodes

  
  int lfs = type.leaf.func->size();
  DBG(std::cout<<"Split on variable "<<var<<std::endl);
//  DBG(std::cout<<"Leaf func size "<<lfs<<std::endl);
  t->type.leaf.func = new BDD_List();
  for(BDD_Iterator iter = type.leaf.func->begin(); iter!=type.leaf.func->end(); ++iter) {
    bdd_ptr temp = bdd_cofactor(t->m, *iter, vava);
    if(!bdd_is_one(t->m,temp))
      t->type.leaf.func->push_back(temp);
    else
      bdd_free(t->m,temp);
  }
  
  e->type.leaf.func = new BDD_List();
  for(BDD_Iterator iter = type.leaf.func->begin(); iter!=type.leaf.func->end(); ++iter) {
    bdd_ptr temp = bdd_cofactor(t->m, *iter, notvava);
    if(!bdd_is_one(t->m,temp))
      e->type.leaf.func->push_back(temp);
    else
      bdd_free(t->m,temp);
  }

  bdd_free(t->m, vava);
  bdd_free(t->m, notvava);

//  t->type.leaf.build_dependency(m);
//  e->type.leaf.build_dependency(m);

  splitvar = (var << 1);
  splitvar |= 0x1;    // Set last bit to 1 for indicating split, remaining bits show the variable on which this is split
  type.children.t = t;
  type.children.e = e;
  t->parent = this;
  e->parent = this;

  /*leaves->erase(find(leaves->begin(),leaves->end(),this);
  leaves->push_back(t);
  leaves->push_back(e);*/

  //   (*split_list)[var]->push_back(this);

  return;
}

void SRTNode::join_bdds(DdManager* d) {
  assert(is_leaf());
  BDD_List* func = type.leaf.func;
  if(func == NULL)
    return;
  bdd_ptr res = bdd_one(d);
  for(BDD_List::iterator it = func->begin(); it!=func->end(); it++)
  {
    bdd_and_accumulate(d,&res,*it);
    bdd_free(d, *it);
  }

  type.leaf.func->resize(0);
  type.leaf.func->push_back(res);
  return;
}

void SRTNode::apply_operator(boolop op) {

  DdManager* dd = m;

  if(is_leaf())
    return;
  SRTNode* t = type.children.t;
  SRTNode* e = type.children.e;
  t->apply_operator(op);  // doesn't matter fow now what we pass because we're only applying op on innermost split hence this will be pack operator
  e->apply_operator(op);  // but think later
  
  assert(t->is_leaf());
  assert(e->is_leaf());
  
  bool is_quant = is_quant_operator();
  
  int spvar = get_split_var();
  
  splitvar = 0;   // make this node a leaf
  type.leaf.func = new BDD_List();
  if(is_quant) {    // quant
    BDD_List* ins = BDD_op(dd,op,t->type.leaf.func,e->type.leaf.func);
    type.leaf.func->insert(type.leaf.func->end(),ins->begin(),ins->end());
  }
  else {              // pack
    bdd_ptr var = bdd_new_var_with_index(dd,spvar);
    bdd_ptr not_var = bdd_not(dd,var);
    
    BDD_List* var_list = new BDD_List();
    var_list->push_back(var);
    
    BDD_List* not_var_list = new BDD_List();
    not_var_list->push_back(not_var);
    
    BDD_List* ans1 = BDD_op(dd,OR,not_var_list,t->type.leaf.func);    // ~x v F(x=true)
    type.leaf.func->insert(type.leaf.func->end(),ans1->begin(),ans1->end());
    
    BDD_List* ans2 = BDD_op(dd,OR,var_list,e->type.leaf.func);
    type.leaf.func->insert(type.leaf.func->end(),ans2->begin(),ans2->end());      // x v F(x=false)
    
    bdd_free(dd,var);
    bdd_free(dd,not_var);
    delete var_list;
    delete not_var_list;
  }
  
  delete t;
  delete e;
  return;   
}

void SRTNode::merge(Quant* q, const int threshold, boolop op) {
  if(op == OR)
    merge_heuristic(q,1,op);
  else
    merge_folding(q,0,op);
  return;
}

void SRTNode::merge_folding (Quant* q, int threshold, boolop op) {
  DBG(std::cout<<"merging "<<get_split_var()<<" with op "<<(op==AND?"AND":"OR")<<std::endl);
  apply_operator(op);
  return;
}

BDD_List* reduce_BDD_List(BDD_List* l, int limit, Quant *q) {
  int red = (int)ceil((double)l->size()/limit);
  BDD_List* ans = new BDD_List();
  int i=0;
  bdd_ptr temp = bdd_one(q->ddm);
  
  while(i < l->size()) {
    if(i!=0 && i%red == 0 && !bdd_is_one(q->ddm,temp)) {
      ans->push_back(temp);
      temp = bdd_one(q->ddm);
    }
    bdd_and_accumulate(q->ddm,&temp,l->at(i));
    i++;
  }
  if(!bdd_is_one(q->ddm,temp))
    ans->push_back(temp);
  return ans;
}

void SRTNode::form_final_tree(Quant* q, SRTNode* t, SRTNode* e, Deduction_List& dl, boolop op, int threshold) {

  assert(is_leaf());

  FreqMap freqinfo;
  DBG(std::cout<<"Finding frequency"<<std::endl);
  t->find_frequency(freqinfo,dl);
  e->find_frequency(freqinfo,dl);
  
  bool to_eval = false;
  int var, freq;
  
  //take variable with maximum frequency
  if(freqinfo.empty()) {
    to_eval = true;
  }   
  else {
    var = freqinfo.begin()->first;
    freq = freqinfo.begin()->second;
    if(freq < threshold)
      to_eval = true;
  }
  
  if(to_eval) {
    BDD_List *lt, *le, *ltn, *len;
    DBG(std::cout<<"Evaluating Left Tree, which "<<(t->is_leaf()?"is":"isn't")<<" a leaf"<<std::endl);
    lt = t->evaluate(q,dl);
    DBG(std::cout<<"Evaluating Right Tree, which "<<(e->is_leaf()?"is":"isn't")<<" a leaf"<<std::endl);
    le = e->evaluate(q,dl);
    
    if(lt->size() > 10) {
      DBG(std::cout<<"Reducing Left from "<<lt->size()<<" to 10"<<std::endl);
      ltn = reduce_BDD_List(lt,10,q);
      for(int i=0;i<lt->size();i++)
        if(lt->at(i))
          bdd_free(q->ddm,lt->at(i));
//      delete lt;
      lt = ltn;
    }
    
    if(le->size() > 10) {
      DBG(std::cout<<"Reducing Right from "<<le->size()<<" to 10"<<std::endl);
      len = reduce_BDD_List(le,10,q);
      for(int i=0;i<le->size();i++)
        if(le->at(i))
          bdd_free(q->ddm,le->at(i));
//      delete le;
      le = len;
    }
    
    DBG(std::cout<<"Combining Reduced Evaluations"<<std::endl);
    type.leaf.func = BDD_op(q->ddm, op, lt, le);
    if(lt) {
      for(int i=0;i<lt->size();i++)
        bdd_free(q->ddm, lt->at(i));
    }
    if(le) {
      for(int i=0;i<le->size();i++)
        bdd_free(q->ddm, le->at(i));
    }
    return;
  }
  else {
    DBG(std::cout<<"Splitting in final tree at "<<var<<std::endl);
    split(var);
    dl.push_back(var<<1+1);
    type.children.t->form_final_tree(q, t, e, dl, op, threshold);
    dl.pop_back();
    dl.push_back(var<<1);
    type.children.e->form_final_tree(q, t, e, dl, op, threshold);
    dl.pop_back();
    return;
  }

  return;
}

/* Find the number of nodes below this node, on just true paths in case splitvar is assigned true in dl and simillarly for false
 * Return total number of occurrences below this node */
void SRTNode::find_frequency(FreqMap& freqinfo, Deduction_List& dl) {
  if(is_leaf()) {
    return;
  }
  int var = get_split_var();

  int found = 0;

  //DBG(printf("Deductions size: %d\n",dl.size()));

  for(Deduction_List::iterator iter = dl.begin(); iter!=dl.end(); ++iter) {
    if(deduction_give_var(*iter) == var) {
      if(is_true(*iter))
        found = 1;    // true
      else
        found = 2;    // false
      break;
    }
  }

  if(found == 0) {
    SRT_Then()->find_frequency(freqinfo,dl);
    SRT_Else()->find_frequency(freqinfo,dl);
    freqinfo[var] += 1;
    return;
  }
  // Don't set when its in deduction because we don't want it as a candidate for splitting now
  else if(found==1) {
    SRT_Then()->find_frequency(freqinfo,dl);
    return;
  }
  else {
    SRT_Else()->find_frequency(freqinfo,dl);
    return;
  }
  return;
}

BDD_List* SRTNode::evaluate(Quant* q,Deduction_List& dl) {
  if(is_leaf()) {
    BDD_List* ans = type.leaf.func;
    for(int i=0;i<ans->size();i++)
      Cudd_Ref(ans->at(i));
    return ans;
  }
  int var = get_split_var();
  bdd_ptr bdd_var = q->get_var_quant_index(var);
  bdd_ptr not_bdd_var = bdd_not(q->ddm,bdd_var);
  BDD_List var_list;
  var_list.push_back(bdd_var);
  BDD_List not_var_list;
  not_var_list.push_back(not_bdd_var);

  int found = 0;
  for(Deduction_List::iterator iter = dl.begin(); iter!=dl.end(); ++iter) {
    if(deduction_give_var(*iter) == var) {
      if(is_true(*iter))
        found = 1;    // true
      else
        found = 2;    // false
      break;
    }
  }

  BDD_List *lt, *le;

  if(found == 0 || found == 1)
    lt = SRT_Then()->evaluate(q,dl);

  if(found == 0 || found == 2)
    le = SRT_Else()->evaluate(q,dl);

  if(found == 0) {
    BDD_List* ans = BDD_op(q->ddm, AND, BDD_op(q->ddm, OR, &not_var_list, lt), BDD_op(q->ddm, OR, &var_list, le));
    bdd_free(q->ddm,bdd_var);
    bdd_free(q->ddm,not_bdd_var);
    if(lt) {
      for(int i=0;i<lt->size();i++)
        bdd_free(q->ddm, lt->at(i));
    }
    if(le) {
      for(int i=0;i<le->size();i++)
        bdd_free(q->ddm, le->at(i));
    }
    return ans;
  }
  else {
    bdd_free(q->ddm,bdd_var);
    bdd_free(q->ddm,not_bdd_var);
    if(found == 1)
      return lt;
    else if(found == 2)
      return le;
  }
  return NULL;
}


void SRTNode::merge_heuristic (Quant* q, int threshold, boolop op) {
  DdManager *m = q->ddm;
  DBG(std::cout<<"merging "<<get_split_var()<<std::endl);
  int var = get_split_var(), freq;
  assert(!is_leaf());

  SRTNode* t = type.children.t;
  SRTNode* e = type.children.e;

  Deduction_List dl;

  qstart = 0;
  splitvar = 0;
  assert(is_leaf());

  DBG(std::cout<<"Forming final tree"<<std::endl);
  form_final_tree(q, t, e, dl, op, threshold);
  delete t;
  delete e;
  return;
  //printf("Node after forming full tree :\n");
  //this->print_indent(4);
/*  freq = freqinfo[var];

  freqinfo.erase(var);
  type.children.t->synchronize_ordering(freqinfo);
  type.children.e->synchronize_ordering(freqinfo);
  freqinfo[var] = freq;
  //printf("Node after synchronizing the order :\n");
  //this->print_indent(4);

  SRTNode *newnode = merge_nodes(q->ddm, t, e, op);
  this->qstart = newnode->qstart;
  this->splitvar = newnode->splitvar;
  delete type.children.e;
  delete type.children.t;
  this->type = newnode->type;
  free(newnode);
  if(this->parent == NULL)
    this->qstart = get_split_var();
  else
    this->parent->set_child_qstart();
  // IMP: recursively set qstart from parent.
  return;*/
}

/*void SRTNode::synchronize_ordering(FreqMap &freqinfo)
{
  //take variable with maximum frequency
  if(is_leaf())
  {
    assert(freqinfo.size() == 0);
    return;
  }
  int var = freqinfo.begin()->first, freq = freqinfo.begin()->second;
  freqinfo.erase(freqinfo.begin());
//  printf("pulling up %d\n", var);
  pull_up_split_var(var);
//  printf("after pulling up %d\n", var);
//  print_indent(4);
  type.children.t->synchronize_ordering(freqinfo);
  type.children.e->synchronize_ordering(freqinfo);
  freqinfo[var] = freq;

}

void SRTNode::set_child_qstart()
{
  if(is_leaf())
    return;
  int c = get_child_qstart();
  if(type.children.e->qstart != c)
  {
    type.children.e->qstart = c;
    type.children.e->set_child_qstart();
  }

  if(type.children.t->qstart != c)
  {
    type.children.t->qstart = c;
    type.children.t->set_child_qstart();
  }
}

int SRTNode::get_child_qstart()
{
  if(get_split_var() < qstart)
    return qstart;
  else
    return get_split_var() + 1;
}

void SRTNode::pull_up_split_var(int var)
{
  //TODO : MAKE more changes to qsplit
  int myvar = get_split_var();
  assert(!is_leaf());
//  printf("entering (%d)->pull_up(%d)\n", myvar, var);
  if(get_split_var() == var)
    return;
  type.children.t->pull_up_split_var(var);
  type.children.e->pull_up_split_var(var);

  SRTNode *n00, *n01, *n10, *n11;
  n11 = type.children.t->type.children.t;
  n10 = type.children.t->type.children.e;
  n01 = type.children.e->type.children.t;
  n00 = type.children.e->type.children.e;

  type.children.t->splitvar = splitvar;
  //type.children.t->type.children.t = n11;
  type.children.t->type.children.e = n01;
  //assert(n01->qstart == type.children.t->get_child_qstart());
  //assert(n11->qstart == type.children.t->get_child_qstart());
  n01->parent = type.children.t;

  type.children.e->splitvar = splitvar;
  //assert(n10->qstart == type.children.e->get_child_qstart());
  //assert(n00->qstart == type.children.e->get_child_qstart());
  type.children.e->type.children.t = n10;
  n10->parent = type.children.e;
  //type.children.e->type.children.e = n00;
  splitvar = var*2 + 1;
  set_child_qstart();
//  printf("result of (%d)->pull_up(%d):\n", myvar, var);
//  print_indent(4);

}*/

void SRTNode::apply_deduction(Deduction d) {
  // FILL IN
}

void SRTNode::deduce() {            // finds and applies deductions
  // FILL IN
}

/* Functions on nodes and formulae */
/*Formula* SRTNode::generate_formula() {
// FILL IN
return NULL;
}*/

/* Functions for printing */
void SRTNode::print_indent(int level) {
  for(int i=0;i<level;i++) {
    printf(" ");
  }
  if(this == NULL)
    printf("NULL\n");
  else if(is_leaf()) {
    std::cout << "Leaf(" << type.leaf.func->size() << ")\n";
  }
  else {
    printf("Non-leaf: ");
    printf("Split on %d\n",get_split_var());
    type.children.t->print_indent(level+4);
    type.children.e->print_indent(level+4);
  }
  return;
}

/* Functions on the whole SRT Table */
Quant* SRT::SRT_vars () {
  return quant_info;
}

CNF_vector* SRT::SRT_clauses () {
  return global_clauses;
}

SRTNode* SRT::SRT_getroot () {
  return root;
}

DdManager* SRT::SRT_getddm () {
  return ddm;
}

void SRT::SRT_merge (int v, const int threshold) {
  Quant* q = quant_info;
  SRTNode_Queue to_merge;
  to_merge.push(root);
  while(!to_merge.empty()) {
    SRTNode* curr = to_merge.front();
    to_merge.pop();
    assert(curr != NULL);
    if(curr->is_leaf())
      continue;
    if(curr->get_split_var() == v) {
      boolop op = (q->is_universal(v) ? AND : OR);
      curr->merge(q,threshold,op);
//      DBG(root->print_indent(4));
      continue;
    }
    to_merge.push(curr->SRT_Then());
    to_merge.push(curr->SRT_Else());
  }

  /*  Quant* q = quant_info;
  for(SRTNode_List::iterator it = (*split_list)[v].begin(); it!=(*split_list)[v].end(); it++) {
  boolop op = (q->is_universal(v) ? AND : OR);
  (*it)->merge(q,threshold,op);
  }*/

  return;
}

/*SRT::SRT(int n) {
  ddm = ddm_init();
  quant_info = new Quant(ddm,n);
  root = new SRTNode(ddm);
  root->qstart = 0;
  root->splitvar = 0;
  root->type.leaf.func = new BDD_List();
  root->type.leaf.dependency = new Vector_Int();
  leaves = new SRTNode_List();
  split_list = new SRTList_List();
}*/

SRT::SRT() {
  ddm = ddm_init();
  leaves = new SRTNode_List();
  split_list = new SRTList_List();
  root = new SRTNode(ddm);
  root->qstart = 1;
  root->splitvar = 0;
//  root->type.leaf.func->push_back(bdd_one(ddm));
  leaves->push_back(root);
}

SRT::~SRT() {
  //free(ddm);
  delete leaves;
  delete split_list;
  delete root;
}

void SRT::parse_qdimacs_srt(char* filename) {
  common_error(ddm, "srt.cpp : DdManager wasn't initialized when parse_qdimacs_srt was called\n");

  int F, V, j, varcount;
  char ae;
  std::ifstream fin;

  fin.open(filename);
//  DBG(std::cout<<"removing comments..."<<std::endl);

  remove_comments(fin);
//  DBG(std::cout<<"reading fg"<<std::endl);

  read_FV(fin, F, V);
//  DBG(std::cout<<"F = "<<F<<", V = "<<V<<std::endl);

  quant_info = new Quant(ddm,V);

  varcount = 1;
  while(1)
  {
    remove_comments(fin);
    fin>>ae;
    if(ae == 'e')
    {
//    DBG(std::cout<<"reading existential block ");
      QBlock *qb = new QBlock();
      qb->is_universal = 0;
      qb->start = varcount;
      do {
        fin>>j;
//      DBG(std::cout<<j<<" ");
        if(j) {
          quant_info->dimacs_srt[j] = -varcount;
          quant_info->srt_dimacs[varcount] = -j;
          varcount++;
        }
      } while(j);
      qb->end = varcount;
//    DBG(std::cout<<std::endl);
      quant_info->blocks.push_back(qb);
      
    } else if(ae == 'a') {
//      DBG(std::cout<<"reading universal block ");
        QBlock *qb = new QBlock();
        qb->is_universal = 1;
        qb->start = varcount;
        do {
          fin>>j;
//        DBG(std::cout<<j<<" ");
          if(j) {
            quant_info->dimacs_srt[j] = varcount;
            quant_info->srt_dimacs[varcount] = j;
            varcount++;
          }
        } while(j);
//      DBG(std::cout<<std::endl);
        qb->end = varcount;
        quant_info->blocks.push_back(qb);
    }
    else
    {
      fin.unget();
      break;
    }
  }

  global_clauses = new CNF_vector(F);

  for(int i = 0; i < F; i++) {
    MClause* mcl = new MClause();
    mcl->vars.clear();
    remove_comments(fin);
    fin>>j;
    while(j) {
//      DBG(std::cout<<j<<" ");
      mcl->vars.push_back(absolute(quant_info->dimacs_srt[absolute(j)]) * (j > 0 ? 1 : -1));
//      DBG(std::cout<< "j is "<<j<<" dimacs_srt[j] = " << quant_info->dimacs_srt[absolute(j)] <<std::endl);
      //mcl->vars.push_back(j);
      fin>>j;
    }
//    DBG(std::cout<<"\t\tclause"<<i<<std::endl);
    global_clauses->clauses[i] = mcl;
  }
  fin.close();

  split_list->resize(V+1);
  for(int i=0;i<V;i++)
    (*split_list)[i].clear();

  return;
}

/*void SRT::split(std::string splitnode, int var) {
  SRTNode* curr = root;
  for(int i=1;i<splitnode.size(); i++) {
    if(splitnode[i] == '1')
      curr = curr->SRT_Then();
    else
      curr = curr->SRT_Else();
  }
  curr->split(var);
  return;
}*/

void SRT::SRT_print() {
  printf("SRT:\n");
  quant_info->print();
  root->print_indent(4);
  std::cout << "split_list has " << split_list->size() << " entries.\n";
  for(int i = 0; i < split_list->size(); i++)
    if(split_list->at(i).size() > 0)
      std::cout << "variable " << i << " has " << split_list->at(i).size() << " split nodes\n";
  std::cout << "SRT has " << leaves->size() << " leaves.\n";
  return;
}


/* Functions of deduction */
int is_true(Deduction d) {
  return ((int)d & 0x1);
}

int make_true_deduction(int var) {
  return ((var<<1) + 1);
}

int make_false_deduction(int var) {
  return (var<<1);
}

int deduction_give_var(Deduction d) {
  return ((int)d >> 1);
}

/* Functions of BDD's */
Deduction_List BDD_deduce(bdd_ptr b) {      // return vector of deductions
  // FILL IN
  Deduction_List ans;
  return ans;
}

BDD_List* BDD_op(DdManager *m,  boolop op, BDD_List* bdd1, BDD_List* bdd2) {
  // FILL IN
  BDD_List *result;
  if(op == AND) {
    result = new BDD_List();
    result->insert(result->end(),bdd1->begin(),bdd1->end());
    result->insert(result->end(), bdd2->begin(), bdd2->end());
    for(int i=0;i<result->size();i++)
      Cudd_Ref(result->at(i));
  }
  else if(op == OR)
  {
    result = new BDD_List();
    int s1 = bdd1->size();
    int s2 = bdd2->size();
    printf("Sizes: %d %d\n",s1,s2);
    for(int i1 = 0; i1 < s1; i1++) {
      for(int i2 = 0; i2 < s2; i2++) {
        result->push_back(bdd_or(m, bdd1->at(i1), bdd2->at(i2)));
        printf("%d %d done\n",i1,i2);
      }
    }
  }
  else
    assert(0);
  return result;;
}

void BDD_for_all_innermost(DdManager* d, bdd_ptr& bdd, Quant* q) {

  /*  // Change the input bdd itself
  CNF_vector* cnf = BDD_to_CNF(q,bdd);
  //  delete bdd;
  int start = q->innermost()->start, end = q->innermost()->end;
  cnf->drop_vars(start,end);
  bdd = CNF_to_BDD(cnf);
   * 
   */

  bdd_ptr block;
  bdd_ptr temp;
  int start = q->innermost()->start, end = q->innermost()->end;
  //DBG(std::cout<<"forall start = "<<start<<" end = "<<end<<std::endl);
  block = bdd_new_var_with_index(d,start);
  for(int i=start+1;i<end; i++) {
    temp = bdd_new_var_with_index(d,i);
    bdd_and_accumulate(d,&block,temp);
    bdd_free(d, temp);
  }

  temp = bdd_forall(d,bdd,block);
  bdd_free(d, bdd);
  bdd_free(d, block);
  bdd = temp;
  return;
}

Vector_Int BDD_support_set(DdManager *m, bdd_ptr bdd) {
  // FILL IN
  Vector_Int ans;
  bdd_ptr v, iter, temp;
  iter = bdd_dup(bdd);
  while(!bdd_is_one(m, iter) && !bdd_is_zero(m, iter))
  {
    v = bdd_new_var_with_index(m, bdd_get_lowest_index(m, iter));
    temp = bdd_cube_diff(m, iter, v);
    bdd_free(m, iter);
    iter = temp;
    ans.push_back(bdd_get_lowest_index(m, v));
    bdd_free(m, v);
  }
  bdd_free(m, iter);
  return ans;
}
/* GLOBAL FUNCTIONS */

/*SRTNode * merge_nodes(DdManager *m, SRTNode *n1, SRTNode *n2, boolop op)
{
//  DBG(printf("i love you, %d %d %d %d\n", n1->qstart, n1->splitvar, n2->qstart, n2->splitvar));
//  DBG(fflush(stdout));
  assert(n1->qstart == n2->qstart && n1->splitvar == n2->splitvar);
  SRTNode *newnode;
  if(n1->is_leaf())
  {
    assert(n2->is_leaf());
    newnode = new SRTNode(m);
    newnode->qstart = n1->qstart;
    newnode->splitvar = n1->splitvar;
    newnode->type.leaf.func = BDD_op(m, op, n1->type.leaf.func, n2->type.leaf.func);
    newnode->parent = NULL;
//    newnode->type.leaf.build_dependency(m);
    return newnode;
  }
  assert( ! n2->is_leaf());
  newnode = new SRTNode(m);
  newnode->qstart = n1->qstart;
  newnode->splitvar = n1->splitvar;
  newnode->type.children.t = merge_nodes(m, n1->type.children.t, n2->type.children.t, op);
  newnode->type.children.e = merge_nodes(m, n1->type.children.e, n2->type.children.e, op);
  newnode->type.children.t->parent = newnode;
  newnode->type.children.e->parent = newnode;
  return newnode;

}*/

SRTNode* make_leaf(DdManager *m, CNF_vector* CV) 
{ // Create a temporary leaf out of CV, its not a leaf of tree, qstart = 0, splitvar = 0
  SRTNode* ans = new SRTNode(m);
  ans->qstart = 0;
  ans->splitvar = 0;
  ans->type.leaf.func = CNF_to_BDD_List(CV, m); // TEMPORARY
//  ans->type.leaf.build_dependency(m);
  return ans;
}

/* factor_graph functions */
factor_graph *make_factor_graph(DdManager *m, SRTNode * node)
{
  int sz = node->type.leaf.func->size();
  bdd_ptr *funcs = new bdd_ptr[sz];
  assert(funcs != NULL);
  for(int i = 0; i < sz; i++)
    funcs[i] = bdd_dup(node->type.leaf.func->at(i));
  factor_graph* fg = factor_graph_new(m, funcs, sz); 
  assert(fg != NULL);
  for(int i = 0; i < sz; i++)
    bdd_free(m, funcs[i]);
  delete[] funcs;
  return fg;

}

/**existentially quantifies out the variables with index in [start, end) from the srtnode curr*/
void factor_graph_eliminate(factor_graph*fg, int start, int end, SRTNode *curr)
{
  
  DdManager* m = curr->m;

  assert(curr->is_leaf());
  assert(end > start);
  bdd_ptr v = bdd_one(m);
  bdd_ptr temp;
  for(int i = start; i < end; i++)
  {
    temp = bdd_new_var_with_index(m, i);
    bdd_and_accumulate(m, &v, temp);
    bdd_free(m, temp);
  }
  bdd_ptr supp = bdd_one(m);          // QUESTION: Can we find currently unhidden variables from Factor Graph efficiently?
  for(int i = curr->type.leaf.func->size(); i-- > 0;)
  {
    temp = bdd_support(m, curr->type.leaf.func->at(i));
    bdd_and_accumulate(m, &supp, temp);
    bdd_free(m, temp);
  }
  bdd_ptr vbar = bdd_cube_diff(m, supp, v);
  bdd_free(m, supp);
  bdd_free(m, v);

  if(bdd_is_one(m, vbar))
  {
    bdd_free(m, vbar);
    //all variables are to be eliminated
    if(end > start + 1)
    {
      factor_graph_eliminate(fg, start, end - 1, curr);   // QUESTION: shouldn't this be in the form of a loop of bdd_forsome ? why do the above part again and again?
    }
    v = bdd_one(m);
    for(int i = curr->type.leaf.func->size(); i-- > 0;)
    {
      bdd_and_accumulate(m, &v, curr->type.leaf.func->at(i));
      bdd_free(m, curr->type.leaf.func->at(i));
    }
    temp = bdd_new_var_with_index(m, end - 1);
    vbar = bdd_forsome(m, v, temp);
    
    bdd_free(m, v);
    bdd_free(m, temp);
    curr->type.leaf.func->resize(0);
    curr->type.leaf.func->push_back(vbar);
//    curr->type.leaf.build_dependency(m);
    return;
  }

  int group_result = factor_graph_group_vars(fg, vbar);
  assert(group_result == 0);  
  fgnode *n = factor_graph_get_varnode(fg, vbar);
  assert(n != NULL);
  factor_graph_eliminate(fg, n, curr);

  bdd_free(m, vbar);
}

void factor_graph_eliminate(factor_graph*fg, fgnode *n, SRTNode *curr)
{
  double init,final;
  
  fg->time++;
  
  bdd_ptr v = factor_graph_make_acyclic(fg, n, 50);
  
  if(v == NULL)
  {
    //make acyclic successful
    DBG(std::cout<<"make acyclic successful"<<std::endl);
    
//  init = clock();
  int conv = factor_graph_acyclic_messages(fg,n);
//  final=clock();
//  time_message_passing += (double)(final-init) / ((double)CLOCKS_PER_SEC);
  
    assert(conv > 0);
    DBG(std::cout<<"messages converged"<<std::endl);
    int sz;
    
    bdd_ptr * func = factor_graph_incoming_messages(fg, n, &sz);
  
    for(int i = curr->type.leaf.func->size(); i-- > 0;)
      bdd_free(fg->m, curr->type.leaf.func->at(i));
    delete curr->type.leaf.func;
    curr->type.leaf.func = new BDD_List();
    for(int i = 0; i < sz; i++) {
      if(!bdd_is_one(fg->m,func[i]))
        curr->type.leaf.func->push_back(func[i]);
    }
    free(func);
//    curr->type.leaf.build_dependency(fg->m, NULL);
    factor_graph_rollback(fg);
    return;
  }
  DBG(std::cout<<"make acyclic unsuccessful, splitting at "<<bdd_get_lowest_index(fg->m, v)<<std::endl);

  curr->split(bdd_get_lowest_index(fg->m, v));

  fg->time++;
  factor_graph_assign_var(fg, v);
  factor_graph_eliminate(fg, n, curr->type.children.t);
  factor_graph_rollback(fg);
  DBG(std::cout<<"done with "<<bdd_get_lowest_index(fg->m, v)<<" set to true"<<std::endl);

  fg->time++;
  bdd_ptr temp = bdd_not(fg->m, v);
  factor_graph_assign_var(fg, temp);
  factor_graph_eliminate(fg, n, curr->type.children.e);
  bdd_free(fg->m, temp);
  factor_graph_rollback(fg);
  DBG(std::cout<<"done with "<<bdd_get_lowest_index(fg->m, v)<<" set to false"<<std::endl);

  bdd_free(fg->m, v);
  factor_graph_rollback(fg);
}

