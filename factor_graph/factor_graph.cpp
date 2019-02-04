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



#include <time.h>
#include <queue>
#include "factor_graph.h"


#define max(x, y) (((x) > (y)) ? (x) : (y))
#define IS_UNVISITED(x) ((x)->color == 0)
#define IS_VISITED(x) ((x)->color == 1)
#define SET_UNVISITED(x) ((x)->color = 0)
#define SET_VISITED(x) ((x)->color = 1)
#define SKIP_DEAD(list, fg)  if(list->died <= fg->time) { \
  list = list->next; \
  continue; }
#define TIME_INFTY 99999

/* ------------- Debugging ----------------------------*/


/* ------------- Profiling ----------------------------*/

//extern double time_var_node, time_func_node, time_find_parent, time_combine, time_exist;

/* ------------- Function Declarations ----------------*/

/* ------------- Factor Graph Datastructure ----------------*/
int factor_graph_add_var(factor_graph *fg, bdd_ptr v);
int factor_graph_add_func(factor_graph *fg, bdd_ptr f);
int factor_graph_add_edge(factor_graph * fg, fgnode * f, fgnode * v);
int factor_graph_add_varnode(factor_graph *fg, fgnode* vn);
int factor_graph_add_funcnode(factor_graph *fg, fgnode* fn);
void factor_graph_unhide_funcnode(factor_graph *, fgnode *);
void factor_graph_unhide_varnode(factor_graph *, fgnode *);
void factor_graph_unhide_edge(factor_graph *, fgedge *);
void factor_graph_delete_var(factor_graph * fg, bdd_ptr v);
void factor_graph_delete_varnode(factor_graph * fg, fgnode* v);
void factor_graph_delete_funcnode(factor_graph * fg, fgnode* f);
void factor_graph_delete_edge(factor_graph *fg, fgedge * e);
void factor_graph_hide_funcnode(factor_graph *, fgnode *);
void factor_graph_hide_varnode(factor_graph *, fgnode *);
void factor_graph_hide_edge(factor_graph *fg, fgedge *e);
int factor_graph_is_acyclic(factor_graph *fg, fgnode ** cycle);
int merge_heur2(factor_graph *fg,fgnode *n1,fgnode *n2,int l);
int merge_heur1(factor_graph *fg,fgnode *n1,fgnode *n2,int l);
int merge(factor_graph *fg,fgnode *n1,fgnode *n2,int l);
int compute_cost(factor_graph *fg,fgnode *n1,fgnode *n2);
void factor_graph_reset_messages(factor_graph *fg);
/* ------------- fgnode Datastructure ----------------*/
fgnode * fgnode_new_func(factor_graph *, bdd_ptr f);
fgnode * fgnode_new_var(factor_graph * fg, bdd_ptr v);
void fgnode_delete(DdManager *m, fgnode *n);
fgnode *fgnode_new_composite_node(factor_graph *fg, fgnode *fn1,fgnode *fn2);
int var_node_pass_messages(factor_graph *fg, fgnode *n, fgnode_list *queue);
int func_node_pass_messages(factor_graph *fg, fgnode *n, fgnode_list *queue);
void fgnode_printname(fgnode *n);
int fgnode_support_size(DdManager *m, fgnode *n);
int fgnode_intersects_var(factor_graph *fg, fgnode *n, bdd_ptr var);

/* ------------- fgnode_list Datastructure ----------------*/
fgnode_list * fgnode_list_add_var(factor_graph *fg, fgnode_list * L, bdd_ptr n);
fgnode_list * fgnode_list_new_var(factor_graph *fg, bdd_ptr n);
fgnode_list * fgnode_list_delete(fgnode_list * fgnl);
fgnode_list * fgnode_list_new_func(factor_graph *, bdd_ptr n);
fgnode_list * fgnode_list_add_func(factor_graph *, fgnode_list * L, bdd_ptr n);
fgnode_list * fgnode_list_add_node(factor_graph * fg, fgnode_list * L, fgnode * n);

/* ------------- fgedge Datastructure ----------------*/
void fgedge_delete(DdManager *m, fgedge *e);

/* ------------- fgedge_list Datastructure ----------------*/
fgedge_list * fgedge_list_new(factor_graph *fg, fgnode *f, fgnode *v);
fgedge_list * fgedge_list_delete(fgedge_list *el);
fgedge_list * fgedge_list_add_edge(factor_graph *fg, fgedge_list * L, fgedge * n);

/* --------------- Miscellaneous ------------------------- */
int bdd_and_exist_vector(DdManager *m, bdd_ptr *f, bdd_ptr* ss, int size, bdd_ptr V);
void var_to_eliminate(factor_graph *, fgnode*, bdd_ptr *, int*);


/* ------------- Function Definitions ----------------*/

/* ------------- Factor Graph Datastructure ----------------*/


/*Function to check whether a factor graph is acyclic or not
Input : Facor Graph that is to check
Output: returns 0 if Factor Graph is cyclic
returns 1 if Factor Fraph is acyclic
*/
int factor_graph_is_acyclic(factor_graph *fg, fgnode ** cycle){
  fgnode_list *fl,*vl;
  fgnode_list *queue;  
  fgnode *u;
  fgedge_list *v;
  *cycle = NULL;
  fl=fg->fl;
  vl=fg->vl;


  // set colors to 0
  // set parents to null
  do{
    fl->n->color=0;
    fl->n->parent = NULL;
    fl=fl->next;
  }while(fl!=fg->fl);

  do{
    vl->n->color=0;
    vl->n->parent = NULL;
    vl=vl->next;
  }while(vl!=fg->vl);




  while(1) {
    // skip dead and visited funcs
    while(fl->n->died <= fg->time || fl->n->color == 1) {
      //if(fl->n->died <= fg->time)
      //  fgdm("dead node", fl->n->died);
      fl = fl->next;
      if(fl == fg->fl)
        return 1;
    }

    // add one node into the queue
    // and mark it as visited
    fl->n->color=1;
    queue= fgnode_list_add_node(fg, NULL, fl->n);


    // breadth first traversal
    while(queue!=NULL) {
      // u is an element of the queue
      // v are the edges of u
      u=queue->n;
      queue = fgnode_list_delete(queue);
      v=u->neigh;

      // loop over neighbours v of u
      if (u->num_neigh > 0) 
        do {
          SKIP_DEAD(v, fg);
          // visit for FUNC_NODE-s
          if(u->type==FUNC_NODE){
            // skip if current neighbour is the parent of u
            if(v->e->vn == u->parent) {
              v = v->next;
              continue;
            }

            // if current neighbour has already been visited,
            // then we've found a cyle
            if(v->e->vn->color==1){
              *cycle = u;
              return 0;
            }
            // else, mark u as parent of current neighbour
            // and add it to the queue
            else{
              v->e->vn->color=1;
              v->e->vn->parent = u;
              if(queue == NULL)
                queue = fgnode_list_add_node(fg, queue, v->e->vn);
              else
                fgnode_list_add_node(fg, queue, v->e->vn);
            }
          }
          else{
            // visit for VAR_NODE-s
            if(v->e->fn == u->parent) {
              v = v->next;
              continue;
            }
            if(v->e->fn->color==1){
              *cycle = u;
              return 0;
            }
            else{
              v->e->fn->color=1;
              v->e->fn->parent = u;
              if(queue == NULL)
                queue = fgnode_list_add_node(fg, queue, v->e->fn);
              else
                fgnode_list_add_node(fg, queue, v->e->fn);
            }
          }
          v=v->next;
        }while(v!=u->neigh);
    }
  }
  assert(0); //THIS LINE SHOULD BE UNREACHABLE
  return 0;
}

void actually_merge(factor_graph *fg,fgnode *n1,fgnode *n2){
  bdd_ptr vars;
  fgnode *n;
  if(n1->type==VAR_NODE){
    assert(n2->type == VAR_NODE && "Attempted to group func node and var node");
    vars=bdd_cube_union(fg->m,n1->ss[0],n2->ss[0]);
    factor_graph_group_vars(fg,vars);
    bdd_free(fg->m, vars);
    return;
  }
  else{
    assert(n2->type != VAR_NODE && "Attempted to group func node and var node");
    n=fgnode_new_composite_node(fg, n1,n2);
    factor_graph_add_funcnode(fg,n);   
    factor_graph_hide_funcnode(fg,n1);
    factor_graph_hide_funcnode(fg,n2);
    //factor_graph_delete_funcnode(fg,n1);
    //factor_graph_delete_funcnode(fg,n2);
  }
} 


int compute_cost(factor_graph *fg,fgnode *n1,fgnode *n2){

  if(n1->type == VAR_NODE)
  {
    assert(n2->type == VAR_NODE && "Computing the cost of unmatched nodes\n");
    bdd_ptr temp;
    temp = bdd_and(fg->m, n1->f[0], n2->f[0]);
    int ret = bdd_size(fg->m, temp) - 1;
    bdd_free(fg->m, temp);
    return ret;
  }

  assert(n1->type == FUNC_NODE);
  assert(n2->type == FUNC_NODE);

  bdd_ptr ss = bdd_one(fg->m);
  int i;
  for(i = 0; i < n1->fs; i++)
    bdd_and_accumulate(fg->m, &ss, n1->ss[i]);
  for(i = 0; i < n2->fs; i++)
    bdd_and_accumulate(fg->m, &ss, n2->ss[i]);

  int ret = bdd_size(fg->m, ss) - 1;
  bdd_free(fg->m, ss);
  return ret;
}

int merge(factor_graph *fg,fgnode *n1,fgnode *n2,int l)
{
  //return merge_heur1(fg, n1->parent, n2, l);
  return merge_heur2(fg, n1, n2, l);
}

/***** Heuristic 1 : merge n1 with n2, ****************
 ******************* their parents with each other, ***
 ******************* and so on ************************/
int merge_heur1(factor_graph *fg,fgnode *n1,fgnode *n2,int l){
  fgnode *dn1;
  fgnode *dn2;
  bdd_ptr temp = NULL;
  int i;
  while(n1!=n2){
    /*
       printf("merging");
       fgnode_printname(n1);
       printf("and");
       fgnode_printname(n2);
       printf("\n");
       */
    assert(n1->type == n2->type);

    if(n1->type==VAR_NODE){
      temp=bdd_and(fg->m,n1->f[0],n2->f[0]);
      i=bdd_size(fg->m,temp)-1;
      bdd_free(fg->m, temp);
      if(i>l){
        return 0;
      }
    }
    else{
      i = compute_cost(fg, n1, n2);
      if(i > l){
        printf("LAMBDA = %d < %d\n", l, i);
        return 0;
      }
    }
    dn1 = n1->parent;
    dn2 = n2->parent;
    actually_merge(fg,n1,n2);
    n1 = dn1;
    n2 = dn2;
  }
  return 1;
}

/****** Merge Heuristic 2 : Do only the cheapest merge ******/
int merge_heur2(factor_graph *fg,fgnode *n1,fgnode *n2,int l)
{
  fgnode *ptr1, *ptr2, *min1, *min2, *term;
  int mincost = (0x1<<30), curcost;

  ptr1 = n1->parent; ptr2 = n2;
  while(ptr1 != ptr2)
  {
    ptr1 = ptr1->parent;
    ptr2 = ptr2->parent;
  }

  term = ptr1;
  if(term->parent != NULL)
    term = term->parent;

  for(ptr1 = n1; ptr1 != term; ptr1 = ptr1->parent)
  {
    for(ptr2 = n1; ptr2 != term; ptr2 = ptr2->parent)
    {
      if(ptr1 == ptr2)
        continue;
      if(ptr1->type == VAR_NODE && ptr2->type != VAR_NODE)
        continue;
      if(ptr2->type == VAR_NODE && ptr1->type != VAR_NODE)
        continue;
      curcost = compute_cost(fg, ptr1, ptr2);
      if(curcost < mincost)
      {
        mincost = curcost;
        min1 = ptr1;
        min2 = ptr2;
      }
    }
    for(ptr2 = n2; ptr2 != term; ptr2 = ptr2->parent)
    {
      if(ptr1 == ptr2)
        continue;
      if(ptr1->type == VAR_NODE && ptr2->type != VAR_NODE)
        continue;
      if(ptr2->type == VAR_NODE && ptr1->type != VAR_NODE)
        continue;
      curcost = compute_cost(fg, ptr1, ptr2);
      if(curcost < mincost)
      {
        mincost = curcost;
        min1 = ptr1;
        min2 = ptr2;
      }
    }
  }
  for(ptr1 = n2; ptr1 != term; ptr1 = ptr1->parent)
  {
    for(ptr2 = n2; ptr2 != term; ptr2 = ptr2->parent)
    {
      if(ptr1 == ptr2)
        continue;
      if(ptr1->type == VAR_NODE && ptr2->type != VAR_NODE)
        continue;
      if(ptr2->type == VAR_NODE && ptr1->type != VAR_NODE)
        continue;
      curcost = compute_cost(fg, ptr1, ptr2);
      if(curcost < mincost)
      {
        mincost = curcost;
        min1 = ptr1;
        min2 = ptr2;
      }
    }
  }
  if(mincost > l)
  {
    //printf("LAMBDA = %d < %d\n", l, mincost);
    return 0;
  }
  actually_merge(fg, min1, min2);
  return 1;
}

bdd_ptr factor_graph_make_acyclic(factor_graph *fg,fgnode *v,int l){
  int is_acyclic=0;
  fgnode_list *fl,*vl;
  fgnode_list *queue;
  bdd_ptr ans;
  fgnode *u;
  fgnode *n1;
  fgnode *n2;
  fgnode *cycle;
  fgedge_list *el;
  int to_break=0;
  queue = NULL;
  while(1){
    while(queue != NULL)
      queue = fgnode_list_delete(queue);
    fl=fg->fl;
    vl=fg->vl;
    do{
      fl->n->color=0;
      fl->n->parent=NULL;
      fl=fl->next;
    }while(fl!=fg->fl);

    do{
      vl->n->color=0;
      vl->n->parent=NULL;
      vl=vl->next;
    }while(vl!=fg->vl);
    queue = fgnode_list_add_node(fg, queue, v);
    v->color=1;
    v->parent=NULL;
    to_break = 0;
    while((queue!=NULL)&&(to_break==0))
    {
      u=queue->n;
      //fgdm("exploring", u->id);
      queue=fgnode_list_delete(queue);

      el=u->neigh;
      if(el != NULL)
      {
        do
        {
          SKIP_DEAD(el, fg);
          if(el->e->vn==u->parent || el->e->fn == u->parent)
          {
            el = el->next;
            continue;
          }
          if(u->type == FUNC_NODE){
            if(el->e->vn->color==0){
              el->e->vn->color=1;
              el->e->vn->parent=u;
              queue = fgnode_list_add_node(fg, queue, el->e->vn);
              //fgdm("pushing v", el->e->vn->id);
              if(queue != NULL) queue = queue->next;
            }
            else{
              n1=el->e->vn;
              n2=el->e->fn;
              to_break=1;
              break;
            }
          }
          else{
            assert(u->type == VAR_NODE);
            if(el->e->fn->color==0){
              el->e->fn->color=1;
              el->e->fn->parent=u;
              queue = fgnode_list_add_node(fg, queue,el->e->fn);
              //fgdm("pushing f", el->e->fn->id);
              if(queue != NULL) queue = queue->next;
            }
            else{
              n1=el->e->fn;
              n2=el->e->vn;
              to_break=1;
              break;
            }
          }
          el = el->next;
        }while(el!=u->neigh);
      }
    }
    while(queue != NULL)
      queue = fgnode_list_delete(queue);
    if(!to_break)
    {
      cycle = NULL;
      if(factor_graph_is_acyclic(fg,  &cycle))
        return NULL;
      else
      {
        //fgdm("cycle with ", cycle->id);
        //factor_graph_print(fg, "temp_cycle.dot", "somefg.txt");
        return factor_graph_make_acyclic(fg, cycle, l);
      }
    }
    if(!merge(fg,n1,n2,l))
    {
      to_break = 0;
      ans = bdd_one(fg->m);
      while(n1->parent != n2)
      {
        var_to_eliminate(fg, n1, &ans, &to_break);
        var_to_eliminate(fg, n2, &ans, &to_break);
        n1 = n1->parent;
        n2 = n2->parent;
      }
      return ans;
    }
    //printf(".");
    //fflush(stdout);
    if(factor_graph_verify(fg) < 0)
    {
      fgdm("verification failed", factor_graph_verify(fg));
      exit(0);
    }

  }
}






int factor_graph_add_funcnode(factor_graph *fg, fgnode* fn)
{
  fgnode_list* newf;
  int conn;
  if(fg->fl == NULL)
  {
    fg->fl = newf = fgnode_list_add_node(fg, fg->fl, fn);
  }
  else
    newf = fgnode_list_add_node(fg, fg->fl, fn);

  if(newf == NULL)
    return -1;

  newf->n->id = ++(fg->max_fid);
  fg->num_funcs++;

  fgnode_list* viter = fg->vl;
  do
  {
    SKIP_DEAD(viter, fg);
    conn = factor_graph_is_connected(fg->m, newf->n, viter->n);
    if(conn == 1)
    {
      if(factor_graph_add_edge(fg, newf->n, viter->n) == -1)
      {
        factor_graph_delete_funcnode(fg, fn);
        return -1;
      }
    }else if(conn == -1)
    {
      factor_graph_delete_funcnode(fg, fn);
      return -1;
    }
    viter = viter->next;
  }while(viter != fg->vl);

  return newf->n->id;
}

void factor_graph_hide_varnode(factor_graph *fg, fgnode *n)
{
  fgedge_list *el;
  fgedge_list *nextel;
  fgnode_list *vnl;

  if(n->died <= fg->time)
    return;
  if(fg->time == n->born)
  {
    factor_graph_delete_varnode(fg, n);
    return;
  }

  el = n->neigh;
  do {
    nextel = el->next;
    factor_graph_hide_edge(fg, el->e);
    el = nextel;
  }while(el != n->neigh);

  vnl = fg->vl;
  do {
    if(vnl->n == n)
    {
      assert(vnl->died == n->died);
      vnl->died = fg->time;
      n->died = fg->time;
      fg->num_vars--;
      break;
    }
    vnl = vnl->next;
  }while(vnl != fg->vl);
  assert(n->died == fg->time);
}

void factor_graph_unhide_edge(factor_graph *fg, fgedge *e)
{
  fgedge_list *el;
  assert(e->born <= fg->time);
  if(e->died != fg->time)
    return;
  factor_graph_unhide_funcnode(fg, e->fn);
  factor_graph_unhide_varnode(fg, e->vn);

  el = e->fn->neigh;
  do{
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = TIME_INFTY;
      e->fn->num_neigh++;
      break;
    }
    el = el->next;
  }while(el != e->fn->neigh);

  el = e->vn->neigh;
  do{
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = TIME_INFTY;
      e->vn->num_neigh++;
      break;
    }
    el = el->next;
  }while(el != e->vn->neigh);

  el = fg->el;
  do {
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = TIME_INFTY;
      fg->num_edges++;
      break;
    }
    el = el->next;
  }while(el != fg->el);

  e->died = TIME_INFTY;
}

void factor_graph_unhide_varnode(factor_graph *fg, fgnode *n)
{
  fgnode_list *vl;
  assert(n->born <= fg->time);
  if(n->died != fg->time)
    return;
  vl = fg->vl;
  do {
    if(vl->n == n)
    {
      assert(vl->died == n->died);
      vl->died = TIME_INFTY;
      fg->num_vars++;
      break;
    }
    vl = vl->next;
  }while(vl != fg->vl);
  n->died = TIME_INFTY;
}

void factor_graph_unhide_funcnode(factor_graph *fg, fgnode *n)
{
  fgnode_list *fl;
  assert(n->born <= fg->time);
  if(n->died != fg->time)
    return;
  fl = fg->fl;
  do {
    if(fl->n == n)
    {
      assert(fl->died == n->died);
      fl->died = TIME_INFTY;
      fg->num_funcs++;
      break;
    }
    fl = fl->next;
  }while(fl != fg->fl);
  n->died = TIME_INFTY;
}

void factor_graph_hide_funcnode(factor_graph *fg, fgnode *n)
{
  fgedge_list *el;
  fgnode_list *fnl;
  fgedge_list *nextel;

  if(n->died <= fg->time)
    return;
  if(n->born == fg->time)
  {
    factor_graph_delete_funcnode(fg, n);
    return;
  }

  el = n->neigh;
  do {
    nextel = el->next;
    factor_graph_hide_edge(fg, el->e);
    el = nextel;
  }while(el != n->neigh);

  fnl = fg->fl;
  do {
    if(fnl->n == n)
    {
      assert(fnl->died == n->died);
      fg->num_funcs--;
      fnl->died = fg->time;
      n->died = fg->time;
      break;
    }
    fnl = fnl->next;
  }while(fnl != fg->fl);
}

void factor_graph_hide_edge(factor_graph *fg, fgedge *e)
{
  fgedge_list *el;
  if(e->died <= fg->time)
    return;

  if(e->born == fg->time)
  {
    factor_graph_delete_edge(fg, e);
    return;
  }

  el = fg->el;
  do
  {
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = fg->time;
      break;
    }
    el = el->next;
  }while(el != fg->el);

  assert(e != NULL);
  assert(e->fn != NULL);
  el = e->fn->neigh;
  do
  {
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = fg->time;
      break;
    }
    el = el->next;
  }while(el != e->fn->neigh);

  el = e->vn->neigh;
  do
  {
    if(el->e == e)
    {
      assert(el->died == e->died);
      el->died = fg->time;
      break;
    }
    el = el->next;
  }while(el != e->vn->neigh);

  e->died = fg->time;
  e->vn->num_neigh--;
  e->fn->num_neigh--;
  fg->num_edges--;
}

int factor_graph_add_varnode(factor_graph *fg, fgnode* vn)
{
  fgnode_list* newv;
  int conn;
  if(fg->vl == NULL)
  {
    newv = fgnode_list_add_node(fg, fg->vl, vn);
  }
  else
    newv = fgnode_list_add_node(fg, fg->vl, vn);

  if(newv == NULL)
    return -1;

  newv->n->id = ++(fg->max_vid);
  fg->num_vars++;

  fgnode_list* fiter = fg->fl;
  do
  {
    SKIP_DEAD(fiter, fg);
    conn = factor_graph_is_connected(fg->m, fiter->n, newv->n);
    if(conn == 1)
    {
      if(factor_graph_add_edge(fg, fiter->n, newv->n) == -1)
      {
        factor_graph_delete_varnode(fg, vn);
        return -1;
      }
    }else if(conn == -1)
    {
      factor_graph_delete_varnode(fg, vn);
      return -1;
    }
    fiter = fiter->next;
  }while(fiter != fg->fl);

  return newv->n->id;
}




/** Creates an edge in a factor graph
 * INPUTS : fg - pointer to the factor graph
 *          f - pointer to the function node
 *          v - pointer to the variable node
 * OUTPUT : successful - edge id
 *          unsuccessful - (-1)
 */
int factor_graph_add_edge(factor_graph *fg, fgnode * f, fgnode * v)
{
  fgedge_list *el;
  fgedge *e;

  el = fgedge_list_new(fg, f, v);
  if(el==NULL)
    return -1;

  if(fg->el == NULL)
    fg->el = el;
  else
  {
    el->next = fg->el;
    el->prev = fg->el->prev;
    if(el->prev != NULL)
      el->prev->next = el;
    fg->el->prev = el;
  }
  el->e->id = ++(fg->max_eid);
  fg->num_edges++;

  e = el->e;
  el = (fgedge_list *)malloc(sizeof(fgedge_list));
  if(el == NULL)
    return -1;
  el->e = e;
  if(f->neigh == NULL)
  {
    f->neigh = el;
    el->next = el->prev = el;
  }
  else
  {
    el->next = f->neigh;
    el->prev = f->neigh->prev;
    el->next->prev = el;
    if(el->prev != NULL)
      el->prev->next = el;
  }
  el->e->id = fg->max_eid;
  el->born = fg->time;
  el->died = TIME_INFTY;
  f->num_neigh++;

  el = (fgedge_list *)malloc(sizeof(fgedge_list));
  if(el == NULL)
    return -1;
  el->e = e;
  if(v->neigh == NULL)
  {
    el->next = el->prev = el;
    v->neigh = el;
  }
  else
  {
    el->next = v->neigh;
    el->prev = v->neigh->prev;
    el->next->prev = el;
    if(el->prev != NULL)
      el->prev->next = el;
  }
  el->e->id = fg->max_eid;
  el->born = fg->time;
  el->died = TIME_INFTY;
  v->num_neigh++;


  return el->e->id;
}

/** Adds a variable node to a factor graph
 * INPUTS : fg - pointer to factor graph
 *          v - bdd cube of the variables
 * OUTPUT : variable id of the new node, or returns -1 if unsuccessful
 */
int factor_graph_add_var(factor_graph *fg, bdd_ptr v)
{
  fgnode_list* newv;
  int conn;
  if(fg->vl == NULL)
  {
    newv = fg->vl = fgnode_list_new_var(fg, v);
  }
  else
    newv = fgnode_list_add_var(fg, fg->vl, v);

  if(newv == NULL)
    return -1;

  newv->n->id = ++(fg->max_vid);
  fg->num_vars++;

  fgnode_list* fiter = fg->fl;
  if(fiter != NULL) do
  {
    SKIP_DEAD(fiter, fg);
    conn = factor_graph_is_connected(fg->m, fiter->n, newv->n);
    if(conn == 1)
    {
      if(factor_graph_add_edge(fg, fiter->n, newv->n) == -1)
      {
        factor_graph_delete_var(fg, v);
        return -1;
      }
    }else if(conn == -1)
    {
      factor_graph_delete_var(fg, v);
      return -1;
    }
    fiter = fiter->next;
  }while(fiter != fg->fl);

  return newv->n->id;
}

/** Answers whether a function node's support set intersects with a variable node
 * INPUTS : m - DdManager
 *          f - the function node
 *          v - the variable node
 * OUTPUT : intersects - 1
 *          does not intersect - 0
 */
int factor_graph_is_connected(DdManager * m, fgnode* f, fgnode *v)
{
  int ans;
  int i;
  if(f == NULL || v == NULL)
    return 0;
  bdd_ptr intsecn;
  bdd_ptr temp;
  if(f->type == FUNC_NODE)
  {
    temp = bdd_one(m);
    for(i = 0; i < f->fs; i++)
      bdd_and_accumulate(m, &temp, f->ss[i]);
    intsecn = bdd_cube_intersection(m, temp, v->ss[0]);
    bdd_free(m, temp);
  }
  else
  {
    assert(f->type == VAR_NODE);
    intsecn = bdd_cube_intersection(m, f->ss[0], v->ss[0]);
  }
  ans = (bdd_is_one(m, intsecn) ? 0 : 1);
  bdd_free(m, intsecn);
  return ans;
}


/** Adds a function node to a factor graph
 * INPUTS : fg - pointer to factor graph
 *          f - (bdd pointer to) function to be added
 * OUTPUT : successful - id of the new function node
 *          unsuccessful - (-1)
 */
int factor_graph_add_func(factor_graph *fg, bdd_ptr f)
{
  fgnode_list* newf;
  int conn;
  if(fg->fl == NULL)
  {
    newf = fg->fl = fgnode_list_new_func(fg, f);
  }
  else
    newf = fgnode_list_add_func(fg, fg->fl, f);

  if(newf == NULL)
    return -1;

  newf->n->id = ++(fg->max_fid);
  fg->num_funcs++;

  fgnode_list* viter = fg->vl;
  if(viter != NULL)
  {
    do
    {
      conn = factor_graph_is_connected(fg->m, newf->n, viter->n);
      if(conn == 1)
      {
        if(factor_graph_add_edge(fg, newf->n, viter->n) == -1)
        {
          factor_graph_delete_funcnode(fg, newf->n);
          return -1;
        }
      }
      viter = viter->next;
    }while(viter != fg->vl);
  }

  return newf->n->id;
}

/** Deletes a variable from a graph, including all its neighboring edges
*/
void factor_graph_delete_varnode(factor_graph * fg, fgnode* v)
{
  int i;
  fgnode_list * vl = fg->vl;
  fgnode * vn;

  //Search for the var node
  if (vl->n != v)
    do {
      vl = vl->next;
    }while((vl->n != v || vl->n->type == FUNC_NODE) && vl != fg->vl);
  if(vl->n->type == FUNC_NODE || vl->n != v) // node does not exist
    return;
  vn = vl->n;

  //delete all neighboring edges from the graph
  while(vn->neigh != NULL)
    factor_graph_delete_edge(fg, vn->neigh->e);

  //delete the variable from the graph
  if(vl->died > fg->time)
    fg->num_vars --;
  //printf("deleting v%d (%d, %d)\n", vl->n->id, vl->n->born, vl->n->died);
  //fflush(stdout);
  assert(vl->born == vn->born);
  assert(vl->n == vn);
  assert(vl->died == vn->died);
  fgnode_delete(fg->m, vn);
  if(fg->vl == vl)
    fg->vl = fgnode_list_delete(vl);
  else
    fgnode_list_delete(vl);
}

/** Deletes a variable from a graph, including all its neighboring edges
*/
void factor_graph_delete_var(factor_graph * fg, bdd_ptr v)
{
  int i;
  fgnode_list * vl = fg->vl;
  fgnode * vn;

  //Search for the var node
  do {
    vl = vl->next;
  }while((vl->n->f[0] != v || vl->n->type == FUNC_NODE) && vl != fg->vl);
  if(vl->n->type == FUNC_NODE || vl->n->f[0] != v) // node does not exist
    return;
  vn = vl->n;

  //delete all neighboring edges from the graph
  while(vn->neigh != NULL)
    factor_graph_delete_edge(fg, vn->neigh->e);

  //delete the variable from the graph
  fgnode_delete(fg->m, vn);
  if(fg->vl == vl)
    fg->vl = fgnode_list_delete(vl);
  else
    fgnode_list_delete(vl);
  fg->num_vars --;
}

/** Deletes a function from a graph, including all its neighboring edges
*/
void factor_graph_delete_func(factor_graph * fg, bdd_ptr f)
{
  int i;
  fgnode_list * fl = fg->fl;
  fgnode * fn;

  //Search for the func node
  if(fl->n->f[0] != f)
    do
      fl = fl->next;
    while((fl->n->f[0] != f || fl->n->type == VAR_NODE)  && fl != fg->fl);
  if(fl->n->type == VAR_NODE || fl->n->f[0] != f) // node does not exist
    return;
  fn = fl->n;

  //delete all neighboring edges from the graph
  while(fn->neigh != NULL)
    factor_graph_delete_edge(fg, fn->neigh->e);

  //delete the function from the graph
  fgnode_delete(fg->m, fn);
  if(fg->fl == fl)
    fg->fl = fgnode_list_delete(fl);
  else
    fgnode_list_delete(fl);
  fg->num_funcs --;
}

/** Deletes a function from a graph, including all its neighboring edges
*/
void factor_graph_delete_funcnode(factor_graph * fg, fgnode* f)
{
  int i;
  fgnode_list * fl = fg->fl;
  fgnode * fn;

  //Search for the func node
  if(fl->n != f)
    do
      fl = fl->next;
    while((fl->n != f || fl->n->type == VAR_NODE)  && fl != fg->fl);
  if(fl->n->type == VAR_NODE || fl->n != f) // node does not exist
    return;
  fn = fl->n;

  //delete all neighboring edges from the graph
  while(fn->num_neigh > 0)
  {
    factor_graph_delete_edge(fg, fn->neigh->e);

  }

  if(fn->died > fg->time)
    fg->num_funcs --;
  //delete the function from the graph
  fgnode_delete(fg->m, fn);
  if(fg->fl == fl)
    fg->fl = fgnode_list_delete(fl);
  else
    fgnode_list_delete(fl);
}

/** Deletes an edge from a graph. Does not delete the nodes.
*/
void factor_graph_delete_edge(factor_graph *fg, fgedge * e)
{
  fgedge_list * el;
  fgedge_list * iter;
  int i;

  //delete edge list from function node
  el = e->fn->neigh;

  if(el->e == e)
  {
    if(el->died > fg->time)
      e->fn->num_neigh--;
    e->fn->neigh = fgedge_list_delete(el);
  }
  else
  {
    do
    {
      if(el->e == e)
      {
        if(el->died > fg->time)
          e->fn->num_neigh--;
        fgedge_list_delete(el);
        break;
      }
      el = el->next;
    }while(el != e->fn->neigh);
  }

  //delete edge list from var node
  el = e->vn->neigh;
  if(el->e == e)
  {
    if(el->died > fg->time)
      e->vn->num_neigh--;
    e->vn->neigh = fgedge_list_delete(el);
  }
  else
    do
    {
      if(el->e == e)
      {
        if(el->died > fg->time)
          e->vn->num_neigh--;
        fgedge_list_delete(el);
        break;
      }
      el = el->next;
    }while(el != e->vn->neigh);

  el = fg->el;
  if(el->e == e)
  {
    if(el->died > fg->time)
      fg->num_edges--;
    fg->el = fgedge_list_delete(el);
  }
  else
  {
    do
    {
      if(el->e == e)
      {
        if(el->died > fg->time)
          fg->num_edges--;
        fgedge_list_delete(el);
        break;
      }
      el = el->next;
    }while(el != fg->el);
  }
  fgedge_delete(fg->m, e);

}

/* ------------- fgnode Datastructure ----------------*/

int fgnode_intersects_var(factor_graph *fg, fgnode *n, bdd_ptr var)
{
  int i;
  bdd_ptr temp;
  if(n->type == FUNC_NODE)
  {
    for(i = 0; i < n->fs; i++)
    {
      temp = bdd_cube_intersection(fg->m, n->ss[i], var);
      if(!bdd_is_one(fg->m, temp))
      {
        bdd_free(fg->m, temp);
        return 1;
      }
      bdd_free(fg->m, temp);
    }
    return 0;
  }
  temp = bdd_cube_intersection(fg->m, n->ss[0], var);
  if(bdd_is_one(fg->m, temp))
  {
    bdd_free(fg->m, temp);
    return 0;
  }
  bdd_free(fg->m, temp);
  return 1;
}

fgnode * fgnode_new_func(factor_graph *fg, bdd_ptr f)
{
  fgnode * fgn = (fgnode *)malloc(sizeof(fgnode));
  if(fgn == NULL)
    return NULL;
  fgn->f = (bdd_ptr*)malloc(sizeof(bdd_ptr ));
  fgn->ss = (bdd_ptr*)malloc(sizeof(bdd_ptr ));
  if(fgn->f == NULL || fgn->ss == NULL)
  {
    if(fgn->f != NULL) free(fgn->f);
    if(fgn->ss != NULL) free(fgn->ss); 
    free(fgn);
    return NULL;
  }
  fgn->f[0] = bdd_dup(f);
  fgn->ss[0] = bdd_support(fg->m, f);
  fgn->id = -1;
  fgn->neigh = NULL;
  fgn->num_neigh =  0;
  fgn->type = FUNC_NODE;
  fgn->color = 0;
  fgn->parent = NULL;
  fgn->fs = 1;
  fgn->born = fg->time;
  fgn->died = TIME_INFTY;
  return fgn;
}

fgnode * fgnode_new_var(factor_graph *fg, bdd_ptr v)
{
  fgnode * fgn = (fgnode*)malloc(sizeof(fgnode));
  if(fgn == NULL)
    return NULL;
  fgn->id = -1;
  fgn->neigh = NULL;
  fgn->num_neigh =  0;
  fgn->type = VAR_NODE;
  fgn->f = (bdd_ptr *)malloc(sizeof(bdd_ptr));
  if(fgn->f == NULL)
  {
    free(fgn);
    return NULL;
  }
  fgn->f[0] = bdd_dup(v);
  fgn->ss = (bdd_ptr *)malloc(sizeof(bdd_ptr));
  if(fgn->ss == NULL)
  {
    bdd_free(fg->m, fgn->f[0]);
    free(fgn->f);
    free(fgn);
    return NULL;
  }
  fgn->ss[0] = bdd_dup(v);
  fgn->fs = 1;
  fgn->color = 0;
  fgn->parent = NULL;
  fgn->born = fg->time;
  fgn->died = TIME_INFTY;
  return fgn;
}

void fgnode_delete(DdManager *m, fgnode *n)
{
  int i;
  if(n->f != NULL)
  {
    for(i = 0; i < n->fs; i++)
      if(n->f[i] != NULL)
        bdd_free(m, n->f[i]);
    free(n->f);
  }
  if(n->ss != NULL)
  {
    for(i = 0; i < n->fs; i++)
      if(n->ss[i] != NULL)
        bdd_free(m, n->ss[i]);
    free(n->ss);
  }

  while(n->num_neigh > 0)
  {
    n->neigh = fgedge_list_delete(n->neigh);
    n->num_neigh--;
  }
  free(n);
}

fgnode *fgnode_new_composite_node(factor_graph *fg, fgnode *fn1,fgnode *fn2)
{
  int i;
  fgnode *F=(fgnode *)malloc(sizeof(fgnode));
  F->born = fg->time;
  F->died = TIME_INFTY;
  if(F == NULL)
    return NULL;
  F->id = -1;
  F->neigh = NULL;
  F->num_neigh = 0;
  F->type = FUNC_NODE;
  F->color = 0;
  F->parent = NULL;
  if(fn1->type==FUNC_NODE && fn2->type==FUNC_NODE){
    F->fs=fn1->fs + fn2->fs;
    F->f=(bdd_ptr *)malloc(sizeof(bdd_ptr)*(F->fs));
    for(i=0; i < fn1->fs; i++)
      F->f[i]=bdd_dup(fn1->f[i]);
    for(i=0; i < fn2->fs; i++)
      F->f[i + fn1->fs]=bdd_dup(fn2->f[i]);
    F->ss = (bdd_ptr *)malloc(sizeof(bdd_ptr)*(F->fs));
    for(i=0;i<fn1->fs;i++)
      F->ss[i]=bdd_dup(fn1->ss[i]);
    for(i=0;i<fn2->fs;i++)
      F->ss[i+fn1->fs]=bdd_dup(fn2->ss[i]);
  }
  else
  {
    fgdm("fn1->type : ", fn1->type);
    fgdm("fn2->type : ", fn2->type);
    printf("fgnode_new_composite_node : unexpected node type\n");
    fflush(stdout);
    return NULL;
  }

  return F;
}


int var_node_pass_messages(factor_graph *fg, fgnode *n, fgnode_list *queue)
{
  assert(n->type == VAR_NODE);
  // compute the AND of all incoming messages
  bdd_ptr and_all_incoming = bdd_one(fg->m);
  for_each_list(n->neigh,
                [&](fgedge_list * el) {
                    bdd_and_accumulate(fg->m, &and_all_incoming, el->e->msg_fv);
                },
                fg->time);

  int error = 0;
  // for each outgoing edge
  for_each_list(n->neigh,
                [&](fgedge_list * el) {

                    // exit on memory errors
                    if (error)
                        return;

                    // compute the and of
                    //   the outgoing message
                    //   and
                    //   the AND of all incoming messages
                    bdd_ptr new_outgoing = bdd_and(fg->m, and_all_incoming, el->e->msg_vf);
                    
                    // if the message needs updating
                    if (new_outgoing != el->e->msg_vf)
                    {
                        // assign the new message to the outgoing edge
                        bdd_free(fg->m, el->e->msg_vf);
                        el->e->msg_vf = new_outgoing;

                        // add the func node to the queue if appropriate
                        if (IS_UNVISITED(el->e->fn))
                        {
                            SET_VISITED(el->e->fn);
                            if (NULL == fgnode_list_add_node(fg, queue, el->e->fn))
                              error = 1;
                        }
                    }
                    else
                        // else throw this new msg away
                        bdd_free(fg->m, new_outgoing);

                },
                fg->time);
  bdd_free(fg->m, and_all_incoming);
  return error * -1;
}



int func_node_pass_messages(factor_graph *fg, fgnode *n, fgnode_list *queue)
{
  assert(FUNC_NODE == n->type);
  
  // compute the conjunction of all incoming messages
  bdd_ptr and_all_incoming = bdd_one(fg->m);
  for_each_list(n->neigh,
                [&](fgedge_list *el) 
                {
                  bdd_and_accumulate(fg->m, &and_all_incoming, el->e->msg_vf);
                },
                fg->time);


  // conjoin it with all the functions in the func node
  for (int fi = 0; fi < n->fs; ++fi)
    bdd_and_accumulate(fg->m, &and_all_incoming, n->f[fi]);


  // variable to catch memory errors in loop
  int error = 0;

  // on each edge
  for_each_list(n->neigh,
                [&](fgedge_list * el)
                {
                  // exit on memory error
                  if (error)
                    return;

                  // compute the complement of the support set
                  bdd_ptr all_vars = bdd_support(fg->m, el->e->msg_fv);
                  bdd_ptr ssbar = bdd_cube_diff(fg->m, all_vars, el->e->vn->ss[0]);
                  bdd_free(fg->m, all_vars);

                  // compute:
                  //   the and of all incoming messages
                  //   AND
                  //   the previous outgoing message
                  // and project it onto the var node of the outgoing edge
                  bdd_ptr new_outgoing = bdd_and_exists(fg->m, and_all_incoming, el->e->msg_fv, ssbar);
                  bdd_free(fg->m, ssbar);
                  // check if the new_outgoing is better
                  if (new_outgoing != el->e->msg_fv)
                  {
                    bdd_free(fg->m, el->e->msg_fv);
                    el->e->msg_fv = new_outgoing;
                    if (IS_UNVISITED(el->e->vn))
                    {
                      if (NULL == fgnode_list_add_node(fg, queue, el->e->vn))
                        error = 1;
                      SET_VISITED(el->e->vn);
                    }
                  } else
                    bdd_free(fg->m, new_outgoing);
                },
                fg->time);

  bdd_free(fg->m, and_all_incoming);
  return error * -1;
}



/** Acyclic version of var_node_pass_messages
*/
int var_node_pass_messages_up(factor_graph *fg, fgnode *n, fgnode **parent)
{
  fgedge_list *el, *eparent;
  bdd_ptr F;
  int error = 0;
  assert(n->type == VAR_NODE && "Var_node_pass_messages_up called on a non-var-node\n");
  el = n->neigh;
  F = bdd_one(fg->m);
  double init,final;

  // Find the parent, the edge where message from function to variable is not passed
  //  init = clock();
  do {
    SKIP_DEAD(el,fg);
    if(el->e->msg_fv == NULL) {
      assert(el->e->msg_vf == NULL);
      eparent = el;

      // Check that rest of messages must be passed
      el = el->next;
      while(el != n->neigh) {
        SKIP_DEAD(el,fg);
        assert(el->e->msg_fv != NULL && "More than one message unpassed in acyclic var node message passing");
        el = el->next;
      }

      break;

    }
    el = el->next;
  } while(el != n->neigh);
  //	final = clock();
  //	time_find_parent += (double)(final-init) / ((double)CLOCKS_PER_SEC);

  assert(eparent != NULL && "Parent not found in acyclic var node message passing");

  el = n->neigh;
  do {
    SKIP_DEAD(el,fg);
    if(el == eparent) {
      el = el->next;
      continue;
    }
    //	  init = clock();
    bdd_and_accumulate(fg->m,&F,el->e->msg_fv);
    //    final = clock();
    //		time_combine += (double)(final-init) / ((double)CLOCKS_PER_SEC);

    el = el->next;
  } while(el != n->neigh);

  eparent->e->msg_vf = bdd_dup(F);
  eparent->e->fn->num_messages++;



  /*  if(IS_VISITED(eparent->e->fn)) {
      fgdm("Num_neighbours: ",eparent->e->fn->num_neigh);
      fgdm("Num_messages: ",eparent->e->fn->num_messages);
      factor_graph_print(fg,"factgrph.dot","tempfile");
      system("rm -f tempfile");
      system("fdp -Tpng -o factgrph.png factgrph.dot");
      }*/



  //  assert(IS_UNVISITED(eparent->e->fn) && "Parent already visited");
  *parent = eparent->e->fn;
  bdd_free(fg->m, F);

  return error*(-1);
}

/** Acyclic version of func_node_pass_messages
*/
int func_node_pass_messages_up(factor_graph *fg, fgnode *n, fgnode **parent)
{
  assert(n->type == FUNC_NODE);
  fgedge_list *el, *eparent;
  bdd_ptr F;
  int i;
  int error = 0;
  int exist_error = 0;
  int num_funcs = n->num_neigh + n->fs - 1;
  bdd_ptr * f1 = (bdd_ptr *)malloc(sizeof(bdd_ptr) * num_funcs);
  bdd_ptr *ss1 = (bdd_ptr *)malloc(sizeof(bdd_ptr) * num_funcs);
  double init,final;

  if(f1 == NULL || ss1 == NULL)
  {
    if(f1 != NULL)      free(f1);
    if(ss1 != NULL)     free(ss1);
    fgdm("error allocating f1 or ss1, num_funcs =", num_funcs);
    return 1;
  }


  el = n->neigh;
  F = bdd_one(fg->m);

  // Find the parent, the edge where message from function to variable is not passed
  //  init = clock();
  do {
    SKIP_DEAD(el,fg);
    if(el->e->msg_vf == NULL) {
      assert(el->e->msg_fv == NULL);
      eparent = el;

      // Check that rest of messages must be passed
      el = el->next;
      while(el != n->neigh) {
        SKIP_DEAD(el,fg);
        assert(el->e->msg_vf != NULL && "More than one message unpassed in acyclic var node message passing");
        el = el->next;
      }

      break;

    }
    el = el->next;
  } while(el != n->neigh);
  //	final = clock();
  //	time_find_parent += (double)(final-init) / ((double)CLOCKS_PER_SEC);

  assert(eparent != NULL && "Parent not found in acyclic var node message passing");

  for(i = 0; i < n->fs; i++)  //the complete array of functions
  {
    f1[i] = bdd_dup(n->f[i]);
    ss1[i] = bdd_dup(n->ss[i]);
  }
  el = n->neigh;
  do
  {
    SKIP_DEAD(el, fg);
    if(el != eparent)
    {
      f1[i] = bdd_dup(el->e->msg_vf);
      ss1[i] = bdd_support(fg->m, el->e->msg_vf);
      i++;
    }
    el = el->next;
  }while(el != n->neigh);

  assert(i == num_funcs);

  exist_error = bdd_and_exist_vector(fg->m, f1, ss1, num_funcs, eparent->e->vn->ss[0]);

  if(exist_error == -1)
  {
    for(i = 0; i < num_funcs; i++)
    {
      if( f1[i] != NULL) bdd_free(fg->m,  f1[i]);
      if(ss1[i] != NULL) bdd_free(fg->m, ss1[i]);
    }
    fgdm("error in existential quantification", 0);
    error = 1;
    free(f1);
    free(ss1);
    return error;
  }

  F = bdd_one(fg->m);

  for(i = 0; i < num_funcs && !error; i++)
  {
    if(f1[i] != NULL) 
    {
      //    	init = clock();
      bdd_and_accumulate(fg->m, &F, f1[i]);
      //      final = clock();
      //			time_combine += (double)(final-init) / ((double)CLOCKS_PER_SEC);
      bdd_free(fg->m, f1[i]);
    }
    if(ss1[i] != NULL) bdd_free(fg->m, ss1[i]);
  }
  eparent->e->msg_fv = bdd_dup(F);
  eparent->e->vn->num_messages++;
  assert(IS_UNVISITED(eparent->e->vn));
  *parent = eparent->e->vn;
  bdd_free(fg->m, F);
  free(f1);
  free(ss1);
  return error;
}





void fgnode_printname(fgnode *n)
{
  if(n == NULL)
    printf(" NULL ");
  else if(n->type == FUNC_NODE)
    printf(" F%d ",n->id);
  else if(n->type == VAR_NODE)
    printf(" v%d ",n->id);
  else
    printf(" u%d ", n->id);
}

int fgnode_support_size(DdManager *m, fgnode *n)
{
  if(n->type == VAR_NODE)
    return (bdd_size(m, n->ss[0]) - 1);

  assert(n->type == FUNC_NODE);

  bdd_ptr temp = bdd_one(m);
  int i;
  for(i = 0; i < n->fs; i++)
    bdd_and_accumulate(m, &temp, n->ss[i]);
  i = (bdd_size(m, temp) - 1);
  bdd_free(m, temp);
  return i;
}


/* ------------- fgnode_list Datastructure ----------------*/

/** creates a new variable node and adds it to the given fgnode_list
 * INPUTS : L - the fgnode_list to be appended
 *          n - bdd_ptr to the node
 * OUTPUT : successful - pointer the node list
 *          unsuccessful - NULL
 */
fgnode_list * fgnode_list_add_var(factor_graph *fg, fgnode_list * L, bdd_ptr n)
{
  fgnode_list * newfgnl = fgnode_list_new_var(fg, n);
  if(newfgnl == NULL)
    return NULL;
  newfgnl->next = L;
  newfgnl->prev = L->prev;
  if(L->prev != NULL)
    L->prev->next = newfgnl;
  L->prev = newfgnl;
  return newfgnl;
}

/** creates a new variable node and uses it to create a new fgnode_list
 * INPUTS : n - bdd_ptr to the node
 * OUTPUT : successful - pointer the new node list
 *          unsuccessful - NULL
 */
fgnode_list * fgnode_list_new_var(factor_graph *fg, bdd_ptr n)
{
  fgnode *var;
  fgnode_list *fgnl ;

  var = fgnode_new_var(fg, n);
  if(var == NULL)
    return NULL;

  fgnl = (fgnode_list *)malloc(sizeof(fgnode_list));
  if(fgnl == NULL)
  {
    fgnode_delete(fg->m, var);
    return NULL;
  }

  fgnl->n = var;
  fgnl->next = fgnl->prev = fgnl;
  fgnl->born = fg->time;
  fgnl->died = TIME_INFTY;
  return fgnl;
}

/** creates a new function node and uses it to create a new fgnode_list
 * INPUTS : n - bdd_ptr to the node
 * OUTPUT : successful - pointer the new node list
 *          unsuccessful - NULL
 */
fgnode_list * fgnode_list_new_func(factor_graph *fg, bdd_ptr n)
{
  fgnode *func;
  fgnode_list *fgnl ;

  func = fgnode_new_func(fg, n);
  if(func == NULL)
    return NULL;

  fgnl = (fgnode_list *)malloc(sizeof(fgnode_list));
  if(fgnl == NULL)
  {
    fgnode_delete(fg->m, func);
    return NULL;
  }

  fgnl->n = func;
  fgnl->next = fgnl->prev = fgnl;
  fgnl->born = fg->time;
  fgnl->died = TIME_INFTY;

  return fgnl;
}

fgnode_list *fgnode_list_delete(fgnode_list * nl)
{
  fgnode_list* result = (nl == nl->next ? NULL : nl->next);
  if(nl->next != NULL)
    nl->next->prev = nl->prev;
  if(nl->prev != NULL)
    nl->prev->next = nl->next;

  free(nl);
  return result;
}

/** creates a new function node and adds it to the given fgnode_list
 * INPUTS : L - the fgnode_list to be appended
 *          n - bdd_ptr to the node
 * OUTPUT : successful - pointer the node list
 *          unsuccessful - NULL
 */
fgnode_list *fgnode_list_add_func(factor_graph *fg, fgnode_list * L, bdd_ptr n)
{
  fgnode_list * newfgnl = fgnode_list_new_func(fg, n);
  if(newfgnl == NULL)
    return NULL;
  newfgnl->next = L;
  newfgnl->prev = L->prev;
  if(L->prev != NULL)
    L->prev->next = newfgnl;
  L->prev = newfgnl;
  return newfgnl;
}

/** adds a given fgnode to a fgnode list
 * INPUTS : L - the fgnode_list to be appended
 *          n - pointer to the fgnode
 * OUTPUT : successful - pointer the node list
 *          unsuccessful - NULL
 */
fgnode_list *fgnode_list_add_node(factor_graph *fg, fgnode_list * L, fgnode * n)
{
  fgnode_list * newfgnl = (fgnode_list *)malloc(sizeof(fgnode_list));
  if(newfgnl == NULL)
    return NULL;
  newfgnl->n = n;
  if(L == NULL)
    newfgnl->next = newfgnl->prev = newfgnl;
  else
  {
    newfgnl->next = L;
    newfgnl->prev = L->prev;
    if(L->prev != NULL)
      L->prev->next = newfgnl;
    L->prev = newfgnl;
  }
  newfgnl->born = fg->time;
  newfgnl->died = TIME_INFTY;
  return newfgnl;
}

/* ------------- fgedge Datastructure ----------------*/
void fgedge_delete(DdManager *m, fgedge *e)
{
  if(e->msg_fv != NULL)
    bdd_free(m, e->msg_fv);
  if(e->msg_vf != NULL)
    bdd_free(m, e->msg_vf);
  free(e);
}

/* ------------- fgedge_list Datastructure ----------------*/
fgedge_list * fgedge_list_new(factor_graph *fg, fgnode *f, fgnode *v)
{
  fgedge * e;
  fgedge_list *el;
  e = (fgedge *)malloc(sizeof(fgedge));
  if(e == NULL)
    return NULL;

  el = (fgedge_list *)malloc(sizeof(fgedge_list));
  if(el == NULL)
  {
    free(e);
    return NULL;
  }

  e->id = -1;
  e->fn = f;
  e->vn = v;
  e->msg_fv = e->msg_vf = NULL;
  el->next = el;
  el->prev = el;
  el->e = e;
  e->born = fg->time;
  e->died = TIME_INFTY;
  el->born = fg->time;
  el->died = TIME_INFTY;
  return el;
}

fgedge_list * fgedge_list_delete(fgedge_list *el)
{
  fgedge_list * result = (el == el->next ? NULL : el->next);
  if(el->next != NULL)
    el->next->prev = el->prev;
  if(el->prev != NULL)
    el->prev->next = el->next;
  free(el);
  return result;
}

fgedge_list *fgedge_list_add_edge(factor_graph *fg, fgedge_list * L, fgedge * e)
{
  fgedge_list *newfgnl = (fgedge_list *)malloc(sizeof(fgedge_list));
  if(newfgnl == NULL)
    return L;
  newfgnl->e=e;
  if(L == NULL){
    newfgnl->prev=newfgnl;
    newfgnl->next=newfgnl;
  }
  else
  {
    newfgnl->next = L;
    newfgnl->prev = L->prev;
    if(L->prev != NULL)
      L->prev->next = newfgnl;
    L->prev = newfgnl;
  }
  newfgnl->born = fg->time;
  newfgnl->died = TIME_INFTY;
  return newfgnl;
}



/* ------------- External functions ----------------- */

fgnode * factor_graph_get_varnode(factor_graph *fg, bdd_ptr v)
{
  fgnode_list *vl = fg->vl;
  bdd_ptr temp = NULL;
  do {
    SKIP_DEAD(vl, fg);
    temp = bdd_cube_intersection(fg->m, v, vl->n->ss[0]);
    if(bdd_is_one(fg->m, temp))
    {
      bdd_free(fg->m, temp);
      temp = NULL;
    }
    else
      break;
    vl = vl->next;
  }while(vl != fg->vl);
  if(temp == NULL)
    return NULL;
  bdd_free(fg->m, temp);
  return vl->n;
}

bdd_ptr* factor_graph_incoming_messages(factor_graph * fg, fgnode * V, int *size)
{
  bdd_ptr * result;
  fgedge_list *el;

  if(V->num_neigh == 0)
  {
    *size = 0;
    return NULL;
  }
  result = (bdd_ptr *)malloc(sizeof(bdd_ptr) * V->num_neigh);
  *size = 0;
  el = V->neigh;
  if(el != NULL) do
  {
    SKIP_DEAD(el, fg);
    result[*size] = bdd_dup(el->e->msg_fv);
    (*size)++;
    el = el->next;
  }while(el != V->neigh);
  assert(*size == V->num_neigh);
  return result;
}

int factor_graph_assign_var(factor_graph *fg, bdd_ptr var)
{
  fgnode_list *vl;
  fgnode * old_v;
  fgnode *newn;
  fgnode_list *newneigh;
  bdd_ptr temp, unnegated;
  fgedge_list *el, *nextel;
  int found, i;

  unnegated = bdd_support(fg->m, var);
  vl = fg->vl;
  found = 0;
  do {
    SKIP_DEAD(vl, fg);
    temp = bdd_cube_intersection(fg->m, unnegated, vl->n->ss[0]);
    if(!bdd_is_one(fg->m, temp))
    {
      bdd_free(fg->m, temp);
      found = 1;
      break;
    }
    bdd_free(fg->m, temp);
    vl = vl->next;
  }while(vl != fg->vl);
  //fgdm("here1", 0);
  if(!found)
    return 0; //no errors
  old_v = vl->n;

  el = old_v->neigh;
  newneigh = NULL;
  do {
    SKIP_DEAD(el, fg);
    newn = (fgnode *)malloc(sizeof(fgnode));
    if(newn == NULL) return 1; //error
    newn->type = FUNC_NODE;
    newn->born = fg->time;
    newn->died = TIME_INFTY;
    newn->parent = NULL;
    newn->id = -1;
    newn->neigh = NULL;
    newn->num_neigh = 0;
    SET_UNVISITED(newn);
    newn->f  = (bdd_ptr *)malloc(sizeof(bdd_ptr) * (int)(el->e->fn->fs));
    newn->ss = (bdd_ptr *)malloc(sizeof(bdd_ptr) * (int)(el->e->fn->fs));
    if(newn->f == NULL || newn->ss == NULL) return 1;
    newn->fs = 0;
    for(i = 0; i < el->e->fn->fs; i++)
    {
      temp = bdd_cofactor(fg->m, el->e->fn->f[i], var);
      if(bdd_is_one(fg->m, temp))
      {
        bdd_free(fg->m, temp);
        continue;
      }
      newn->f[newn->fs] = temp;
      newn->ss[newn->fs] = bdd_support(fg->m, temp);
      newn->fs++;
    }
    if(newn->fs == 0)
      fgnode_delete(fg->m, newn);
    else
    {
      newneigh = fgnode_list_add_node(fg, newneigh, newn);
      if(newneigh == NULL)
        return 1;
    }
    nextel = el->next;
    factor_graph_hide_funcnode(fg, el->e->fn);
    el = nextel;
  }while(el != old_v->neigh && old_v->neigh != NULL);
  temp = bdd_cube_diff(fg->m, old_v->ss[0], unnegated);
  bdd_free(fg->m, unnegated);

  factor_graph_hide_varnode(fg, old_v);

  if(!bdd_is_one(fg->m, temp))
  {
    newn = fgnode_new_var(fg, temp);
    if(newn == NULL)
      return 1;
    factor_graph_add_varnode(fg, newn);
  }

  bdd_free(fg->m, temp);

  while(newneigh != NULL)
  {
    factor_graph_add_funcnode(fg, newneigh->n);
    //if(newneigh->n->id == 295)
    //  fgdm("i was right!", 0);
    newneigh = fgnode_list_delete(newneigh);
  }
  if(factor_graph_verify(fg) < 0)
  {
    fgdm("verification failed", factor_graph_verify(fg));
    exit(0);
  }
  return 0;
}

void factor_graph_rollback(factor_graph *fg)
{
  fgnode_list *nl, *nnl;
  fgedge_list *el, *nel;
  int i;

  //add back deleted function nodes
  nl = fg->fl;
  do {
    if(nl->born < fg->time && nl->died == fg->time)
      factor_graph_unhide_funcnode(fg, nl->n);
    nl = nl->next;
  }while(nl != fg->fl);

  //fgdm("added back deleted function nodes", factor_graph_verify(fg));

  //add back deleted variable nodes
  nl = fg->vl;
  do {
    if(nl->born < fg->time && nl->died == fg->time)
      factor_graph_unhide_varnode(fg, nl->n);
    nl = nl->next;
  }while(nl != fg->vl);
  //fgdm("added back deleted variable nodes", factor_graph_verify(fg));

  //add back deleted edges
  el = fg->el;
  do {
    if(el->born < fg->time && el->died == fg->time)
      factor_graph_unhide_edge(fg, el->e);
    el = el->next;
  }while(el != fg->el);
  //fgdm("added back deleted edges", factor_graph_verify(fg));

  //remove added edges
  el = fg->el;
  do
  {
    nel = (el->next == el ? NULL : el->next);
    if(el->born == fg->time)
    {
      //fgdm("removing edge ", el->e->id);
      factor_graph_delete_edge(fg, el->e);
      //fgdm("removed edge ", 0);
    }
    el = nel;
  }while(el != fg->el && el != NULL);
  //fgdm("removed added edges", factor_graph_verify(fg));

  //remove added functions
  nl = fg->fl;
  do
  {
    nnl = nl->next;
    assert(nl->died == nl->n->died);
    if(nl->born == fg->time)
      factor_graph_delete_funcnode(fg, nl->n);
    nl = nnl;
  }while(nl != fg->fl);
  //fgdm("removed added func nodes", factor_graph_verify(fg));

  //remove added variables
  nl = fg->vl;
  do
  {
    nnl = nl->next;
    if(nl->born == fg->time)
      factor_graph_delete_varnode(fg, nl->n);
    nl = nnl;
  }while(nl != fg->vl);
  //fgdm("removed added var nodes", factor_graph_verify(fg));

  fg->time--;

  if(factor_graph_verify(fg) < 0)
  {
    fgdm("verification failed", factor_graph_verify(fg));
    exit(0);
  }

}

/**
 * Creates a new factor graph
 * INPUTS : m - manager
 *          f - array of bdds holding the functions in the function nodes
 *          size - size of the array f
 * OUTPUT : pointer to a factor graph if successful, else returns NULL
 */
factor_graph * factor_graph_new(DdManager *m,bdd_ptr *f, int size)
{
  factor_graph * fg = (factor_graph *)malloc(sizeof(factor_graph));
  int i;
  bdd_ptr supp;
  bdd_ptr v;
  bdd_ptr temp;

  if(fg == NULL)
    return NULL;
  fg->m = m;
  fg->num_funcs = 0;
  fg->num_vars = 0;
  fg->num_edges = 0;
  fg->max_fid = -1;
  fg->max_vid = -1;
  //dummy nodes to mark the beginning of the lists
  fg->time = -1;
  fg->vl = fgnode_list_new_var( fg, bdd_one( fg->m ) );
  fg->fl = fgnode_list_new_func( fg, bdd_one( fg->m ) );
  fg->el = NULL;
  factor_graph_add_edge(fg, fg->fl->n, fg->vl->n);
  fg->el->died = fg->vl->died = fg->fl->died = -1;
  fg->vl->n->died = fg->fl->n->died = -1;  
  fg->time = 1;

  supp = bdd_vector_support(m, f, size);
  while(!bdd_is_one(m, supp))
  {
    v = bdd_new_var_with_index(m, bdd_get_lowest_index(m, supp));
    if(factor_graph_add_var(fg, v) == -1)
    {
      factor_graph_delete(fg);
      return NULL;
    }
    temp = bdd_cube_diff(m, supp, v);
    bdd_free(m, supp);
    bdd_free(m, v);
    supp = temp;
  }

  for(i = 0; i < size; i++)
  {
    if(factor_graph_add_func(fg, f[i]) == -1)
    {
      factor_graph_delete(fg);
      return NULL;
    }
  }

  return fg;
}

/** Deletes a factor graph and frees all memory
*/
void factor_graph_delete(factor_graph *fg)
{
  while(fg->vl != NULL)
    factor_graph_delete_varnode(fg, fg->vl->n);
  while(fg->fl != NULL)
    factor_graph_delete_funcnode(fg, fg->fl->n);
  free(fg);
}

/** Function to set all messages in the factor graph to true
*/
void factor_graph_reset_messages(factor_graph *fg)
{
  //set all messages to true
  fgedge_list *el;
  el = fg->el;
  if(el != NULL) do {
    SKIP_DEAD(el, fg);
    if(el->e->msg_fv != NULL)
    {
      bdd_free(fg->m, el->e->msg_fv);
      //el->e->msg_fv = NULL;
    }
    el->e->msg_fv = bdd_one(fg->m);

    if(el->e->msg_vf != NULL)
    {
      bdd_free(fg->m, el->e->msg_vf);
      //el->e->msg_vf = NULL;
    }
    el->e->msg_vf = bdd_one(fg->m);

    el = el->next;
  } while(el != fg->el);

}

/** Function to reset messages to NULL and num_messages to 0
*/
void factor_graph_reset_messages_num(factor_graph *fg)
{
  //set number of messages received from neighbours to be 0
  fgnode_list *viter;
  fgnode_list *fiter;
  fgedge_list *el;

  //set all messages to NULL
  el = fg->el;
  if(el != NULL) do {
    SKIP_DEAD(el, fg);
    if(el->e->msg_fv != NULL)
    {
      bdd_free(fg->m, el->e->msg_fv);
    }
    el->e->msg_fv = NULL;

    if(el->e->msg_vf != NULL)
    {
      bdd_free(fg->m, el->e->msg_vf);
    }
    el->e->msg_vf = NULL;

    el = el->next;
  } while(el != fg->el);

  viter = fg->vl;
  do
  {
    SKIP_DEAD(viter, fg);
    viter->n->num_messages = 0;
    viter = viter->next;
  }while(viter != fg->vl);

  fiter = fg->fl;
  do
  {
    SKIP_DEAD(fiter, fg);
    fiter->n->num_messages = 0;
    fiter = fiter->next;
  }while(fiter != fg->fl);

  return;
}


int factor_graph_acyclic_messages(factor_graph *fg, fgnode* root)
{
  int i,j,var;
  fgnode_list *queue = NULL, *bfs = NULL;
  fgnode_list *itr;
  fgnode *n, *nn, *parent;
  fgedge_list *el;
  int error = 0;
  double init,final;

  if(!root)
    return 1;

  if(fg->el == NULL)
    return 1;

  assert(root->type == VAR_NODE && "Root is not a variable node in factor_graph_acyclic_messages");

  factor_graph_reset_messages_num(fg);

  queue = NULL;

  itr = fg->fl;
  do
  {
    SKIP_DEAD(itr, fg);
    SET_UNVISITED(itr->n);
    if(itr->n->num_neigh == 1) {
      queue = fgnode_list_add_node(fg, queue, itr->n);
      SET_VISITED(itr->n);
    }
    itr = itr->next;
  }while(itr != fg->fl);

  itr = fg->vl;
  do
  {
    SKIP_DEAD(itr, fg);
    SET_UNVISITED(itr->n);
    if(itr->n->num_neigh == 1 && itr->n != root) {
      queue = fgnode_list_add_node(fg, queue, itr->n);
      SET_VISITED(itr->n);
    }
    itr = itr->next;
  }while(itr != fg->vl);

  // There has to be a leaf
  if(queue == NULL)
  {
    fgdm("queue is null!", fg->num_funcs);
    return -1;
  }

  // While queue is not empty, take a node. See what link hasn't had its message set. 
  // Pass accumulated message of all other links on that link
  // If that node has now num_messages == num_neigh-1 and it is not root, then add it to the queue
  while(queue != NULL && !error)
  {
    n = queue->n;
    assert(IS_VISITED(n) && (n->num_messages == n->num_neigh-1 || n->num_messages == n->num_neigh) && "Messages not received from enough neighbours");
    queue = fgnode_list_delete(queue);
    SET_UNVISITED(n);
    if(n->num_messages == n->num_neigh)
      continue;

    if(n->type == VAR_NODE) {
      //		  init = clock();
      error = var_node_pass_messages_up(fg, n, &parent);
      //			final=clock();
      //			time_var_node += (double)(final-init) / ((double)CLOCKS_PER_SEC);
    }
    else {
      assert(n->type == FUNC_NODE);
      //		  init = clock();
      error = func_node_pass_messages_up(fg, n, &parent);
      //			final=clock();
      //			time_func_node += (double)(final-init) / ((double)CLOCKS_PER_SEC);
    }
    if(parent->num_messages == parent->num_neigh - 1 && parent != root) {
      //    	assert(!IS_VISITED(parent) && "Node with insufficient messages already visited");
      if(IS_UNVISITED(parent)) {
        queue = fgnode_list_add_node(fg, queue, parent);
        SET_VISITED(parent);
      }
    }
  }
  if(error)
  {
    while(queue != NULL)
      queue = fgnode_list_delete(queue);
    fgdm("error", error);
    return -1;
  }

  assert(root->num_messages == root->num_neigh && "Root didn't receive messages from all neighbours");

  return 1;
}



/** Passes messages in a factor graph till convergence
*/
int factor_graph_converge(factor_graph *fg)
{
  int i,j;
  fgnode_list *queue;
  fgnode_type curtype;
  fgnode_list *itr;
  fgnode * n;
  int iter;
  int error = 0;

  if(fg->el == NULL)
    return 1;

  factor_graph_reset_messages(fg);

  queue = NULL;
  itr = fg->fl;
  do
  {
    SKIP_DEAD(itr, fg);
    SET_VISITED(itr->n);
    queue = fgnode_list_add_node(fg, queue, itr->n);
    itr = itr->next;
  }while(itr != fg->fl);

  itr = fg->vl;
  do
  {
    SET_UNVISITED(itr->n);
    itr = itr->next;
  }while(itr != fg->vl);
  if(queue == NULL)
  {
    fgdm("queue is null!", fg->num_funcs);
    return -1;
  }
  curtype = FUNC_NODE;
  iter = 1;
  while(queue != NULL && !error)
  {
    //printf(".");
    //fflush(stdout);n
    n = queue->n;
    //fgdm("exploring node ", n->id);

    if(n->type != curtype)
    {
      curtype = n->type;
      iter++;
      //fgdm("iteration :", iter);
    }

    if(n->type == VAR_NODE)
    {
      error = var_node_pass_messages(fg, n, queue);
      //if(error)
      //  fgdm("error in var_node_pass_messagse", 0);
    }
    else {
      assert(n->type == FUNC_NODE);
      //fgdm("entering cpm", 0);
      error = func_node_pass_messages(fg, n, queue);
      //if(error)
      //  fgdm("error in func_node_pass_messages", 0);
      //fgdm("leaving cpm", 0);
    }
    queue = fgnode_list_delete(queue);
    SET_UNVISITED(n);
  }
  //printf("\n");
  if(error)
  {
    while(queue != NULL)
      queue = fgnode_list_delete(queue);
    fgdm("error", error);
    return -1;
  }

  return iter;
}

void factor_graph_print(factor_graph *fg, const char * dotfile, const char * fgfile)
{
  fgnode_list *fiter, *viter;
  fgedge_list *eiter;
  FILE *fgv;
  FILE *gv;
  if(strcmp(dotfile, "stdout"))
    gv = fopen(dotfile, "w");
  else
    gv = stdout;
  if(strcmp(fgfile, "stdout"))
    fgv = fopen(fgfile, "w");
  else
    fgv = stdout;

  int i;
  //printf("* * * * * * *\n* Printing factor graph:\n");
  fiter = fg->fl;
  if(fiter == NULL || fg->vl == NULL)
  {
    fprintf(fgv, "* factor graph has no functions/variables!!\n* * * * * * *\n");
    return;
  }
  fprintf(gv, "graph {\n");
  fprintf(fgv, "* Adjacency list:\n");
  do {
    fprintf(fgv, "* f%d : ", fiter->n->id);
    eiter = fiter->n->neigh;
    if(eiter != NULL) do {
      SKIP_DEAD(eiter, fg);
      fprintf(gv, "f%d -- v%d;\n", fiter->n->id, eiter->e->vn->id);
      fprintf(fgv, "v%d ", eiter->e->vn->id);
      eiter = eiter->next;
    }while(eiter != fiter->n->neigh);
    fprintf(fgv, "\n");
    fiter = fiter->next;
  }while(fiter != fg->fl);

  fprintf(fgv, "* Functions:\n");
  do {
    fprintf(fgv, "* f%d:\n",fiter->n->id);
    //if(fiter->n->type == FUNC_NODE)
    //  bdd_print_minterms(fg->m, fiter->n->f[0]);
    SKIP_DEAD(fiter, fg);
    fprintf(gv, "f%d [label=\"f%d(%d)\", shape=box];\n", fiter->n->id, fiter->n->id, fgnode_support_size(fg->m, fiter->n));
    fiter = fiter->next;
  }while(fiter != fg->fl);

  viter = fg->vl;
  fprintf(fgv, "* Variables:\n");
  do {
    SKIP_DEAD(viter, fg);
    fprintf(fgv, "* v%d:\n",viter->n->id);
    //bdd_print_minterms(fg->m, viter->n->f[0]);
    fprintf(gv, "v%d [label=\"v%d(%d)\"];\n", viter->n->id, viter->n->id, fgnode_support_size(fg->m, viter->n));
    viter = viter->next;
  }while(viter != fg->vl);

  fprintf(fgv, "* * * * * * *\n");
  fflush(stdout);
  fprintf(gv, "}\n");
  if(gv != stdout)
    fclose(gv);
  if(fgv != stdout)
    fclose(fgv);

  viter = fg->vl;
  /*do{
    fgdm("nei : ", viter->n->num_neigh);
    viter = viter->next;
    }while(viter != fg->vl);
    */ 
}



/** Groups the variable nodes that intersect with the given cube into a single variable node
 * INPUTS : fg - pointer to the factor graph
 *          vars - a pointer to the bdd cube of variables to be merged  
 * OUTPUT : 0 if successful
 *          -1 if unsuccessful
 */
int factor_graph_group_vars(factor_graph *fg, bdd_ptr vars)
{
  int count = 0;
  fgnode_list *vl = fg->vl;
  fgnode_list *nvl;
  bdd_ptr intscn, unn;

  do {
    SKIP_DEAD(vl, fg);
    intscn = bdd_cube_intersection(fg->m, vars, vl->n->f[0]);
    if(intscn == NULL)
      return -1;
    if(intscn != bdd_one(fg->m))
      count++;
    bdd_free(fg->m, intscn);
    vl = vl->next;
  }while(vl != fg->vl);

  if(count == 0 || count == 1)
    return 0;

  unn = bdd_one(fg->m);
  do {
    SKIP_DEAD(vl, fg);
    intscn = bdd_cube_intersection(fg->m, vars, vl->n->f[0]);
    if(intscn == NULL)
      return -1;
    nvl = (vl->next == vl? NULL : vl->next);
    if(intscn != bdd_one(fg->m))
    {
      bdd_and_accumulate(fg->m, &unn, vl->n->f[0]);
      //factor_graph_delete_var(fg, vl->n->f[0]);
      //assert(0); //fgtest3 debug : this function should not be called
      factor_graph_hide_varnode(fg, vl->n);
    }
    bdd_free(fg->m, intscn);
    vl = nvl;
  }while(vl != fg->vl || nvl==NULL);
  if(factor_graph_add_var(fg, unn) == -1)
    return -1;
  bdd_free(fg->m, unn);
  return 0;
}


int factor_graph_verify(factor_graph *fg)//performing 7 tests
{
  fgnode_list *fl;
  fgnode_list *vl;
  fgedge_list *el;
  fgedge_list *fl1;
  fgedge_list *vl1;
  int count = 0, temp;
  int is_correct = 1;
  int max_fid,max_vid,max_eid;
  max_fid = max_vid = max_eid = -1;
  fl=fg->fl;
  vl=fg->vl;
  //checking the number of function nodes
  if(fl != NULL) do{
    //printf("func id %d, born %d, died %d\n", fl->n->id, fl->n->born, fl->n->died);
    //printf("       list born %d, died %d\n", fl->born, fl->died);
    SKIP_DEAD(fl, fg);
    if(fl->n->type != FUNC_NODE)
    {
      fgdm("non func node in func list", 0);
      return -1;
    }
    if(fl->n->id > max_fid){
      max_fid=fl->n->id;
    }
    count++;
    if(fl->next->prev != fl)
    {
      fgdm("inconsistent func list", 0);
      return -2;
    }
    fflush(stdout);
    fl = fl->next;
  }while(fl != fg->fl);
  if(count != fg->num_funcs){
    fgdm("incorrect number of functions", count);
    fgdm("correct number is ", fg->num_funcs);
    return -3;
  }
  //if(max_fid != fg->max_fid){
  //  return -4;
  //}
  //  fgdm("num_funcs ok", 0);
  count = 0;//reset count for the check of the variable nodes
  //checking the number of variable nodes
  if(vl != NULL) do {
    SKIP_DEAD(vl, fg);
    if(vl->n->type != VAR_NODE)
      return -5;
    if(vl->n->id > max_vid){
      max_vid = vl->n->id;
    }
    count++;
    if(vl->next->prev != vl)
      return -6;
    vl = vl->next;
  }while(vl != fg->vl);
  if(count != fg->num_vars){
    fgdm("count is ", count);
    fgdm("num_vars is ", fg->num_vars);
    is_correct=-7;
    return is_correct;
  }
  //if(max_vid != fg->max_vid){
  //  is_correct = -8;
  //  return is_correct;
  //}
  //  fgdm("num_vars ok", 1);
  count = 0;  //reset count for the check of number of edges
  //checking the total no. of edges
  el = fg->el;
  if(el != NULL) do{
    SKIP_DEAD(el, fg);
    if(el->e->id > max_eid){
      max_eid = el->e->id;
    }
    if(el->e->fn->type != FUNC_NODE || el->e->vn->type != VAR_NODE)
    {
      fgdm("el->e->fn->type", el->e->fn->type);
      fgdm("el->e->vn->type", el->e->vn->type);
      fgdm("el->e->id", el->e->id);
      return -9;
    }
    count++;
    if(el->next->prev != el)
      return -10;

    fl1 = el->e->fn->neigh;
    temp = 0;
    if(fl1 != NULL) do{
      SKIP_DEAD(fl1, fg);
      if(el->e == fl1->e){
        temp = 1;
        break;
      }
      fl1 = fl1->next;
    }while(fl1 != el->e->fn->neigh);
    if(temp != 1)
      return -11;
    temp = 0;
    vl1 = el->e->vn->neigh;
    if(vl1 != NULL) do{
      SKIP_DEAD(vl1, fg);
      if(el->e == vl1->e){
        temp = 1;
        break;
      }
      vl1 = vl1->next;
    }while(vl1 != el->e->vn->neigh);
    if(temp != 1)
      return -12;


    el = el->next;
  }while(el != fg->el);
  if(count != fg->num_edges){
    is_correct = -13;
    return is_correct;
  }
  //if(max_eid != fg->max_eid){
  //  is_correct = -14;
  //  return is_correct;
  //}
  //  fgdm("num_edges ok", 2);
  //checking the no. of neighbours of each function node
  do{
    SKIP_DEAD(fl, fg);
    fl1 = fl->n->neigh;
    count = 0;
    if(fl1 != NULL) do{
      SKIP_DEAD(fl1, fg);
      count++;
      if(fl1->next->prev != fl1)
        return -15;
      fl1 = fl1->next;
    }while(fl1 != fl->n->neigh);
    if(count != fl->n->num_neigh){
      is_correct = -16;
      return is_correct;
    }
    fl = fl->next;
  }while(fl != fg->fl);

  count = 0;//reset count for the check of no. of neighbours of a variable node
  //checking no. of neighbours of a variable node
  do{
    SKIP_DEAD(vl, fg);
    vl1 = vl->n->neigh;
    count = 0;
    if(vl1 != NULL) do{
      SKIP_DEAD(vl1, fg);
      count++;
      if(vl1->next->prev != vl1)
        return -17;
      vl1 = vl1->next;
    }while(vl1 != vl->n->neigh);
    if(count != vl->n->num_neigh){
      is_correct = -18;
      return is_correct;
    }
    vl = vl->next;
  }while(vl != fg->vl);

  return is_correct;
}

int factor_graph_test(DdManager *m)
{
  return 0;
}

/* --------------- Miscellaneous ------------------------- */

void var_to_eliminate(factor_graph *fg, fgnode *n, bdd_ptr *ans, int *score)
{
  int i;
  int count;
  fgedge_list *e;
  bdd_ptr supp_set;
  bdd_ptr var, temp;
  if(n->type == FUNC_NODE)
  {
    //e = n->neigh;
    //do
    //{
    //  SKIP_DEAD(e, fg);
    //  var_to_eliminate(fg, e->e->vn, ans, score);
    //  e = e->next;
    //}while(e != n->neigh);
    return;
  }
  assert(n->type == VAR_NODE);
  //if i go crazy then will you still call me superman?
  supp_set = bdd_dup(n->ss[0]);
  while(supp_set != bdd_one(fg->m))
  {
    var = bdd_new_var_with_index(fg->m, bdd_get_lowest_index(fg->m, supp_set));
    temp = bdd_cube_diff(fg->m, supp_set, var);
    bdd_free(fg->m, supp_set);
    supp_set = temp;
    count = 0;
    e = n->neigh;
    do
    {
      SKIP_DEAD(e, fg);
      if(fgnode_intersects_var(fg, e->e->fn, var))
        count++;
      e = e->next;
    }while(e != n->neigh);
    if(count > *score)
    {
      *score = count;
      bdd_free(fg->m, *ans);
      *ans = bdd_dup(var);
    }

    bdd_free(fg->m, var);
  }
}

int bdd_and_exist_vector(DdManager *m, bdd_ptr *f, bdd_ptr* ss, int size, bdd_ptr V)
{
  bdd_ptr Vbar, temp, nextv;
  int i, min_index;
  Vbar = bdd_one(m);
  double init, final;

  if(Vbar == NULL)
    return -1;
  for(i = 0; i < size; i++)
    bdd_and_accumulate(m, &Vbar, ss[i]);

  temp = bdd_cube_diff(m, Vbar, V);
  bdd_free(m, Vbar);
  Vbar = temp;
  //temp = NULL;

  while(!bdd_is_one(m, Vbar)) {
    nextv = bdd_new_var_with_index(m, bdd_get_lowest_index(m, Vbar));
    min_index = -1;
    for(i = 0; i < size; i++) {
      if(f[i] == NULL)
        continue;
      temp = bdd_cube_intersection(m, nextv, ss[i]);
      if(bdd_is_one(m, temp)) {
        bdd_free(m, temp);
        continue;
      }
      bdd_free(m, temp);
      if(min_index == -1) {
        min_index = i;
        continue;
      }
      assert(&(f[min_index]) == (f + min_index));
      //  init = clock();
      bdd_and_accumulate(m, &(f[min_index]), f[i]);
      //	final = clock();
      //	time_combine += (double)(final-init) / ((double)CLOCKS_PER_SEC);
      bdd_free(m, f[i]);
      bdd_free(m, ss[i]);
      f[i] = NULL;
      ss[i] = NULL;
    }

    if(min_index != -1)
    {
      //	init = clock();
      temp = bdd_forsome(m, f[min_index], nextv);
      //	final = clock();
      //	time_exist += (double)(final-init) / ((double)CLOCKS_PER_SEC);
      bdd_free(m, f[min_index]);
      f[min_index] = temp;

      bdd_free(m, ss[min_index]);
      ss[min_index] = bdd_support(m, f[min_index]);
    }

    temp = bdd_cube_diff(m, Vbar, nextv);
    bdd_free(m, Vbar);
    bdd_free(m, nextv);
    Vbar = temp;
    //temp = NULL;
  }
  return 1;

}


bdd_ptr *vector_to_bdd(DdManager *m, int **cnf, int *clssz, int cnfsz, int *ressz)
{
  int i, j, n;
  bdd_ptr* res;
  bdd_ptr next, temp;

  res = (bdd_ptr *)malloc(sizeof(bdd_ptr)*cnfsz);
  if(res == NULL)
  {
    *ressz = -1;
    return NULL;
  }

  for(i = 0; i < cnfsz; i++)
  {
    res[i] = bdd_zero(m);
    for(j = 0; j < clssz[i]; j++)
    {
      n = cnf[i][j];
      if(n < 0) n = -1*n;
      next = bdd_new_var_with_index(m, n);
      if(cnf[i][j] < 0)
      {
        temp = bdd_not(m, next);
        bdd_free(m, next);
        next = temp;
      }
      bdd_or_accumulate(m, &(res[i]), next);
      bdd_free(m, next);
    }
    //printf("\n");
  }
  return res;
}

bool factor_graph_is_single_connected_component(factor_graph *fg)
{
  int const COLOR_UNVISITED = 0, COLOR_VISITED = 1;
  if (fg->fl == NULL || fg->vl == NULL)
    return false;
  fgnode * topNode = NULL;
  auto mark_unvisited = [&](fgnode_list *fl)
                        {
                          fl->n->color = COLOR_UNVISITED;
                          topNode = fl->n;
                        };
  for_each_list(fg->fl, mark_unvisited);
  for_each_list(fg->vl, mark_unvisited);

  if (NULL == topNode)
    return false;

  std::queue<fgnode*> q;
  q.push(topNode);
  topNode->color = COLOR_VISITED;

  while(!q.empty())
  {
    auto curNode = q.front();
    q.pop();
    for_each_list(curNode->neigh,
                  [&](fgedge_list * el)
                  {
                    fgnode * nextNode = (curNode->type == FUNC_NODE ? el->e->vn : el->e->fn);
                    if (nextNode->color == COLOR_UNVISITED)
                    {
                      nextNode->color = COLOR_VISITED;
                      q.push(nextNode);
                    }
                  });
  }

  bool isConnected = true;
  auto find_unvisited = [&](fgnode_list *nl)
                        {
                          if(nl->n->color == COLOR_UNVISITED)
                            isConnected = false;
                        };
  for_each_list(fg->fl, find_unvisited);
  for_each_list(fg->vl, find_unvisited);
  return isConnected;
}
























