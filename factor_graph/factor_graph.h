#include<stdio.h>
#include "util.h"
#include "dd.h"

#define FACTOR_GRAPH_DEBUG 1

#ifndef FACTOR_GRAPH
#define FACTOR_GRAPH

typedef enum fgnode_type_enum{VAR_NODE, FUNC_NODE} fgnode_type;

struct fgnode_str;
struct fgnode_list_str;
struct fgedge_str;
struct fgedge_list_str;
struct factor_graph_str;

typedef struct fgnode_str
{
  int id;
  bdd_ptr *f, *ss;
  struct fgnode_str *parent;
  struct fgedge_list_str * neigh;
  int num_neigh;
  int num_messages;
  int fs;
  int color;
  fgnode_type type;
  int born;
  int died;
  //struct fgnode_list_str *fgl; //TODO
} fgnode;

typedef struct fgnode_list_str
{
  struct fgnode_list_str *next;
  struct fgnode_list_str *prev;
  fgnode * n;
  int born;
  int died;
} fgnode_list;

typedef struct fgedge_str
{
  fgnode* fn;
  fgnode* vn;
  bdd_ptr msg_fv;
  bdd_ptr msg_vf;
  //int changed_fv, changed_vf;
  int id;
  int born;
  int died;
  //struct fgedge_list_str *vnl, *fnl, *fgl; //TODO
} fgedge;

typedef struct fgedge_list_str
{
  struct fgedge_list_str *next;
  struct fgedge_list_str *prev;
  fgedge *e;
  int born;
  int died;
} fgedge_list;

typedef struct factor_graph_str
{
  fgnode_list *fl;
  fgnode_list *vl;
  fgedge_list *el;
  int num_funcs, num_vars, num_edges;
  int max_fid, max_vid, max_eid;
  DdManager *m;
  int time;
  //fgnode_list *dnl;
  //fgedge_list *del;
} factor_graph;

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
