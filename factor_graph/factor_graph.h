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




#ifndef FACTOR_GRAPH
#define FACTOR_GRAPH

#include<stdio.h>
#include <util.h>
#include <dd.h>

#define FACTOR_GRAPH_DEBUG 1

typedef enum fgnode_type_enum{VAR_NODE, FUNC_NODE} fgnode_type;

struct fgnode;
struct fgnode_list;
struct fgedge;
struct fgedge_list;
struct factor_graph;

struct fgnode
{
  int id;
  bdd_ptr *f, *ss;
  struct fgnode *parent;
  struct fgedge_list *neigh;
  int num_neigh;
  int num_messages;
  int fs;
  int color;
  fgnode_type type;
  int born;
  int died;
};

struct fgnode_list
{
  struct fgnode_list *next;
  struct fgnode_list *prev;
  fgnode * n;
  int born;
  int died;
};

struct fgedge
{
  fgnode* fn;
  fgnode* vn;
  bdd_ptr msg_fv;
  bdd_ptr msg_vf;
  int id;
  int born;
  int died;
};

struct fgedge_list
{
  struct fgedge_list *next;
  struct fgedge_list *prev;
  fgedge *e;
  int born;
  int died;
};

struct factor_graph
{
  fgnode_list *fl;
  fgnode_list *vl;
  fgedge_list *el;
  int num_funcs, num_vars, num_edges;
  int max_fid, max_vid, max_eid;
  DdManager *m;
  int time;
};

extern factor_graph * factor_graph_new(DdManager *m,bdd_ptr *f, int size);
extern void factor_graph_delete(factor_graph *fg);
extern int factor_graph_converge(factor_graph *fg);
extern int factor_graph_acyclic_messages(factor_graph *fg, fgnode* root);
extern int factor_graph_group_vars(factor_graph *fg, bdd_ptr vars);
extern int factor_graph_verify(factor_graph *fg);
extern int factor_graph_test(DdManager *dd);
extern bdd_ptr *vector_to_bdd(DdManager *m, int **cnf, int *clssz, int cnfsz, int *ressz);
extern void factor_graph_print(factor_graph *fg, const char * dotfile, const char * fgfile);
extern bdd_ptr factor_graph_make_acyclic(factor_graph *fg,fgnode *v,int l);
extern void factor_graph_rollback(factor_graph *fg);
extern int factor_graph_assign_var(factor_graph *fg, bdd_ptr var);
extern bdd_ptr* factor_graph_incoming_messages(factor_graph * fg, fgnode * V, int *size);
extern void actually_merge(factor_graph *fg,fgnode *n1,fgnode *n2);
extern int factor_graph_is_connected(DdManager * m, fgnode* f, fgnode *v);
extern fgnode * factor_graph_get_varnode(factor_graph *fg, bdd_ptr v);

#define fgdm(m, i) printf("%s: %s %d\n", __FUNCTION__, m, i); fflush(stdout)
//#define fgdm(i) 0

#endif /* FACTOR_GRAPH */
