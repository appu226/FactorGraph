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



#include <factor_graph.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CLAUSE_SIZE 20
#define LAMBDA 40
#define MAX_MEM_MB 4000

int main(int, char**);
DdManager* ddm_init();
void fgtest1(factor_graph *fg);
void fgtest2(factor_graph *fg);
void fgtest3(factor_graph *fg, fgnode* fgn, int l);
bdd_ptr fgtest4(factor_graph *fg, fgnode* fgn, int l);
int main1(int argc, char ** argv);
int main2(int argc, char ** argv);
int best_lambda(factor_graph *fg, fgnode *fgn);
void remove_comments(FILE *f);

void remove_comments(FILE *f)
{
  char c = '\n';
  char buffer[1000];
  while(c == '\n')  fscanf(f, "%c", &c);
  while(c == 'c' || c == 'a' || c == 'e')
  {
    fscanf(f, "%[^\n]", buffer);
    c = '\n';
    while(c == '\n')  fscanf(f, "%c", &c);
  }
  ungetc(c, f);
}

void merge_heur3(factor_graph *fg)
{
  int i, j;
  do
  {
    i = 0;//no change
    fgnode_list *fl;
    fgedge_list *el;
    fgnode *v1, *v2, *fn;
    fl = fg->fl;
    do
    {
      if(fl->died <= fg->time || fl->n->num_neigh != 2)
      {
        fl = fl->next;
        continue;
      }
      fn = fl->n;
      el = fn->neigh;
      v1 = NULL;
      v2 = NULL;
      do
      {
        if(el->died <= fg->time) {el = el->next; continue;}
        if(v1 == NULL)
          v1 = el->e->vn;
        else
        {
          assert(v2 == NULL);
          v2 = el->e->vn;
          //break;
        }
        el = el->next;
      } while(el != fn->neigh);
      
      el = v1->neigh;
      do
      {
        if(el->died <= fg->time || el->e->fn == fn || !factor_graph_is_connected(fg->m, el->e->fn, v2))
        {
          el = el->next;
          continue;
        }
        actually_merge(fg, el->e->fn, fn);
        i = 1;
      } while(el != v1->neigh && !i);
      
      
      fl = fl->next;
    } while(fl != fg->fl && !i);
    
  } while(i);
}

bdd_ptr fgtest4(factor_graph *fg, fgnode* fgn, int l)
{
  bdd_ptr *ans, res, res2, temp;
  int sz, i;
  res = factor_graph_make_acyclic(fg, fgn, l);
  if(res == NULL)
  {
    fgdm("made acyclic", l);
    assert(factor_graph_converge(fg) > 0);
    fgdm("converged", 1);
    ans = factor_graph_incoming_messages(fg, fgn, &sz);
    res = bdd_one(fg->m);
    for(i = 0; i < sz; i++)
    {
      bdd_and_accumulate(fg->m, &res, ans[i]);
      bdd_free(fg->m, ans[i]);
    }
    free(ans);
    return res;
  }
  fg->time++;
  fgdm("assigning true ", bdd_get_lowest_index(fg->m, res));
  assert(factor_graph_assign_var(fg, res) == 0);
  res2 = fgtest4(fg, fgn, l);
  factor_graph_rollback(fg);
  temp = bdd_not(res);
  res = temp;
  fg->time++;
  fgdm("assigning false ", bdd_get_lowest_index(fg->m, res));
  assert(factor_graph_assign_var(fg, res) == 0);
  temp = fgtest4(fg, fgn, l);
  fgdm("accumulating ", bdd_get_lowest_index(fg->m, res));
  bdd_free(fg->m, res);
  bdd_or_accumulate(fg->m, &res2, temp);
  bdd_free(fg->m, temp);
  factor_graph_rollback(fg);
  return res2;
}

int best_lambda(factor_graph *fg, fgnode *fgn)
{
  int ans;
  printf("enter lambda -> ");
  scanf("%d", &ans);
  return ans;
  
  /*
  int ans = 1, mintime = 99999;
  int cur;
  //mintime is the log of the time taken
  for(cur = fg->num_vars/2; cur > fg->num_vars/10; cur = cur * .9)
  {
    int curtime = 0, prevtime = -1;
    bdd_ptr attempt;
    printf("cur = %d", cur);
    fflush(stdout);
    fg->time++;
    while(1) 
    {
      merge_heur3(fg);
      attempt = factor_graph_make_acyclic(fg, fgn, cur);
      if(attempt == NULL)
      {
        break;
      }
      curtime++;
      printf(".");
      factor_graph_assign_var(fg, attempt);
      bdd_free(fg->m, attempt);
    }
    printf("\n");
    if(curtime >= mintime || curtime >= 10 || curtime == prevtime)
    {
      factor_graph_rollback(fg);
      continue;
    }
    prevtime = curtime;
    printf("converging...");
    time_t tt = time(NULL);
    factor_graph_converge(fg);
    tt = time(NULL) - tt;
    while(tt != 0)
    {
      curtime++;
      tt>>1;
    }
    if(curtime < mintime)
    {
      mintime = curtime;
      ans = cur;
    }
    factor_graph_rollback(fg);
  }
  return ans;
  */
}

int main(int argc, char ** argv)
{
  main2(argc, argv);
}

int main2(int argc, char ** argv)
{
  DdManager *ddm;
  int numvars, numclauses;
  int **cnf, *clssz;
  int i;
  int j;
  char command[1001];
  char param1[100], param2[300];
  factor_graph *fg;
  bdd_ptr nextv;
  bdd_ptr *funcs;
  FILE * inputfile;

  /* Initialize the DD Manager*/
  ddm = ddm_init();

  /* Input the problem*/
  
  if(argc != 2)
  {
    printf("Usage : \n\t %s <filename>\n", argv[0]);
    return 0;
  }

  inputfile = fopen(argv[1], "r");
  remove_comments(inputfile);
  fscanf(inputfile, "p cnf %d %d", &numvars, &numclauses);
  printf("numclauses is %d, numvars = %d\n", numclauses, numvars);

  cnf = (int **)malloc(sizeof(int *) * numclauses);
  clssz = (int *)malloc(sizeof(int) * numclauses);
  common_error(cnf, "main.c : out of mem while inputting problem\n");
  common_error(clssz, "main.c : out of mem while inputting problem\n");
  for(i = 0; i < numclauses; i++)
  {
    remove_comments(inputfile);
    cnf[i] = (int *)malloc(sizeof(int) * MAX_CLAUSE_SIZE);
    common_error(cnf[i], " out of mem while inputting problem\n");
    clssz[i] = 0;
    fscanf(inputfile, "%d", &j);
    while(j != 0)
    {
      assert(clssz[i] < MAX_CLAUSE_SIZE && "MAX_CLAUSE_SIZE insufficient\n");
      cnf[i][clssz[i]] = j;
      clssz[i]++;
      fscanf(inputfile, "%d", &j);
    }
  }
  fclose(inputfile);
  
  /* Create the factor graph*/
  funcs = vector_to_bdd(ddm, cnf, clssz, numclauses, &j);
  common_error(funcs, "main.c : could not convert array to bdd\n");
  fg = factor_graph_new(ddm, funcs, numclauses);
  printf("factor graph created\n");
  fflush(stdout);
  
  /* Command line */
  do {
    printf("-> ");
    scanf("%s", command);
    if(!strcmp(command, "exit"))
      break;
    else if(!strcmp(command, "quit"))
      break;
    else if(!strcmp(command, "help"))
    {
      printf("quit/exit\t\t : end the program\n");
      printf("help\t\t\t : print this message\n");
      printf("verify\t\t\t : verify the factor graph\n");
      printf("print\t\t\t : print the factor graph to files\n");
      printf("createpng\t\t : create a png of the factorgraph\n");
      printf("checkpoint\t\t : set checkpoint %d\n", (fg->time + 1));
      printf("rollback\t\t : rollback to checkpoint %d\n", (fg->time - 1));
      printf("makeacyclic\t\t : attempt to make the graph acyclic\n");
      printf("converge\t\t : pass messages to convergence\n");
      printf("setvar\t\t\t : set a variable to be true or false\n");
      printf("assertmsg\t\t : check whether message passing works\n");
      printf("mergevar\t\t : merge two variable nodes\n");
      printf("mergefunc\t\t : merge two function nodes\n");
      printf("mergeheur3\t\t : merge 2-neigh func nodes with other func nodes of super set of neighbors\n");
      printf("fgtest3\t\t\t : make the graph acyclic by recursively assigning values to variables\n");
      printf("fgtest4\t\t\t : compare the exact answer timewise with factorgraph result\n");
      printf("randomgroup\t\t : make a random grouping of variables\n");
    }
    else if(!strcmp(command, "fgtest4"))
    {
      printf("Enter number of vars to group -> ");
      scanf("%d", &i);
      time_t tt1, tt2;
      
      printf("grouping vars...\n");
      fg->time++;
      fgnode_list *vl = fg->vl;
      bdd_ptr vargrp = bdd_one(fg->m);
      do{
        if(vl->died <= fg->time)
        {
          vl = vl->next;
          continue;
        }
        if(random() % fg->num_vars < i)
          bdd_and_accumulate(fg->m, &vargrp, vl->n->ss[0]);
        vl = vl->next;
      }while(vl != fg->vl);
      assert(factor_graph_group_vars(fg, vargrp) == 0);
      bdd_free(fg->m, vargrp);
      
      i = fg->max_vid;
      do{
        if(vl->died <= fg->time)
        {
          vl = vl->prev;
          continue;
        }
        if(vl->n->id == i)
          break;
        vl = vl->prev;
      }while(vl != fg->vl);
      assert(vl->n->id == i);
      
      printf("computing best possible lambda...\n");
      int lam = best_lambda(fg, vl->n);
      //int lam = 85;
      printf("lam = %d\n", lam);
      
      printf("computing fg answer...\n");
      tt1 = time(NULL);
      //bdd_ptr res1 = fgtest4(fg, vl->n, lam);
      bdd_ptr res1 = bdd_one(fg->m);
      tt1 = time(NULL) - tt1;
      printf("computed in %g seconds.\n", (double)tt1);
      fflush(stdout);
      
      printf("computing actual answer...\n");
      tt2 = time(NULL);
      bdd_ptr fand, block, res2;
      fand = bdd_one(fg->m);
      fgnode_list* fl = fg->fl;
      do
      {
        if(fl->died <= fg->time)
        {
          fl = fl->next;
          continue;
        }
        int fi;
        for(fi = 0; fi < fl->n->fs; fi++)
          bdd_and_accumulate(fg->m, &fand, fl->n->f[fi]);
        fl = fl->next;
      } while(fl != fg->fl);
      res2 = bdd_support(fg->m, fand);
      block = bdd_cube_diff(fg->m, res2, vl->n->ss[0]);
      bdd_free(fg->m, res2);
      res2 = bdd_forsome(fg->m, fand, block);
      bdd_free(fg->m, fand);
      bdd_free(fg->m, block);
      tt2 = time(NULL) - tt2;
      printf("computed in %g seconds.\n", (double)tt2);
      fflush(stdout);
      if(res1 == res2)
        printf("answers match :)\n");
      else
      {
        printf("answers don\'t match :(, ");
        fand = bdd_and(fg->m, res1, res2);
        if(fand == res2)
          printf("but brute ans is an overapproximation of fg answer :)\n");
        else
          printf("and brute ans is not an overapproximation of fg answer :(\n");
        bdd_free(fg->m, fand);
      }
      bdd_free(fg->m, res1);
      bdd_free(fg->m, res2);
    }
    else if(!strcmp(command, "randomgroup"))
    {
      printf("Enter number of vars -> ");
      scanf("%d", &i);
      fgnode_list *vl = fg->vl;
      bdd_ptr vargrp = bdd_one(fg->m);
      do{
        if(vl->died <= fg->time)
        {
          vl = vl->next;
          continue;
        }
        if(random() % fg->num_vars < i)
          bdd_and_accumulate(fg->m, &vargrp, vl->n->ss[0]);
        vl = vl->next;
      }while(vl != fg->vl);
      assert(factor_graph_group_vars(fg, vargrp) == 0);
      bdd_free(fg->m, vargrp);
      factor_graph_rollback(fg);
    }
    else if(!strcmp(command, "fgtest3"))
    {
      printf("enter LAMBDA -> ");
      scanf("%d", &i);
      printf("enter id of the variable node -> ");
      scanf("%d", &j);
      fgnode_list* vl = fg->vl;
      do
      {
        if(vl->died <= fg->time)
        {
          vl = vl->next;
          continue;
        }
        if(vl->n->id == j)
          break;
        vl = vl->next;
      } while(vl != fg->vl);
      if(vl->n->id != j)
        printf("invalid node id\n");
      fgtest3(fg, vl->n, i);
    }
    else if(!strcmp(command, "mergeheur3"))
    {
      merge_heur3(fg);
    }
    else if(!strcmp(command, "mergevar"))
    {
      fgnode_list *vl1, *vl2;
      printf("enter first var id -> ");
      scanf("%d", &i);
      vl1 = fg->vl;
      do
      {
        if(vl1->died <= fg->time)
        {
          vl1 = vl1->next;
          continue;
        }
        if(vl1->n->id == i)
          break;
        vl1 = vl1->next;
      } while(vl1 != fg->vl);
      if(vl1->n->id != i)
      {
        printf("no such variable\n");
        continue;
      }
      
      printf("enter second var id -> ");
      scanf("%d", &i);
      vl2 = fg->vl;
      do
      {
        if(vl2->died <= fg->time)
        {
          vl2 = vl2->next;
          continue;
        }
        if(vl2->n->id == i)
          break;
        vl2 = vl2->next;
      } while(vl2 != fg->vl);
      if(vl2->n->id != i)
      {
        printf("no such variable\n");
        continue;
      }
      
      actually_merge(fg, vl1->n, vl2->n);
      printf("merge successful.\n");
    }
    else if(!strcmp(command, "mergefunc"))
    {
      fgnode_list *fl1, *fl2;
      printf("enter first func id -> ");
      scanf("%d", &i);
      fl1 = fg->fl;
      do
      {
        if(fl1->died <= fg->time)
        {
          fl1 = fl1->next;
          continue;
        }
        if(fl1->n->id == i)
          break;
        fl1 = fl1->next;
      } while(fl1 != fg->fl);
      if(fl1->n->id != i)
      {
        printf("no such function\n");
        continue;
      }
      
      printf("enter second func id -> ");
      scanf("%d", &i);
      fl2 = fg->fl;
      do
      {
        if(fl2->died <= fg->time)
        {
          fl2 = fl2->next;
          continue;
        }
        if(fl2->n->id == i)
          break;
        fl2 = fl2->next;
      } while(fl2 != fg->fl);
      if(fl2->n->id != i)
      {
        printf("no such function\n");
        continue;
      }
      
      fg->time++;
      actually_merge(fg, fl1->n, fl2->n);
      printf("merge successful.\n");
    }
    else if(!strcmp(command, "assertmsg"))
    {
      fgnode_list *vl;
      fgnode_list *fl;
      fgedge_list *el;
      bdd_ptr msgans, actans;
      bdd_ptr varcube, vbar;
      
      printf("enter varnode id -> ");
      scanf("%d", &i);
      
      printf("computing the message from message passing...\n");
      factor_graph_converge(fg);
      vl = fg->vl;
      if(vl != NULL) do
      {
        if(vl->died <= fg->time)
        {
          vl = vl->next;
          continue;
        }
        if(vl->n->id == i)
          break;
        vl = vl->next;
      }while(vl != fg->vl);
      if(vl->n->id != i)
      {
        printf("var node id incorrect\n");
        continue;
      }
      msgans = bdd_one(fg->m);
      int ressize;
      bdd_ptr* res;
      res = factor_graph_incoming_messages(fg, vl->n, &ressize);
      for(j = 0; j < ressize; j++)
      {
        bdd_and_accumulate(fg->m, &msgans, res[j]);
        bdd_free(fg->m, res[j]);
      }
      free(res);
      printf("done\n");
      
      printf("computing the actual answer...\n");
      actans = bdd_one(fg->m);
      varcube = bdd_one(fg->m);
      fl = fg->fl;
      do
      {
        if (fl->died <= fg->time)
        {
          fl = fl->next;
          continue;
        }
        for (j = 0; j < fl->n->fs; j++)
        {
          bdd_and_accumulate(fg->m, &actans, fl->n->f[j]);
          bdd_and_accumulate(fg->m, &varcube, fl->n->ss[j]);
        }
        fl = fl->next;
      } while(fl != fg->fl);
      vbar = bdd_cube_diff(fg->m, varcube, vl->n->ss[0]);
      bdd_free(fg->m, varcube);
      if(!bdd_is_one(fg->m, vbar))
        varcube = bdd_forsome(fg->m, actans, vbar);
      else
        varcube = bdd_dup(actans);
      bdd_free(fg->m, vbar);
      bdd_free(fg->m, actans);
      actans = varcube;
      
      printf("done\n");
      
      bdd_ptr myans1 = bdd_or(fg->m,bdd_new_var_with_index(fg->m,0),bdd_new_var_with_index(fg->m,1));
      bdd_ptr myans2 = bdd_and(fg->m,
      				bdd_or(fg->m,bdd_new_var_with_index(fg->m,0),bdd_new_var_with_index(fg->m,2)),
      				bdd_or(fg->m,bdd_new_var_with_index(fg->m,1),bdd_not(bdd_new_var_with_index(fg->m,2))));
      
      if(msgans == actans) {
      	if(msgans == myans1)
      		printf("MATCH1\n");
      	else if(msgans == myans2)
      		printf("MATCH2\n");
      		
        printf("answers match :)\n");
      }
      else
      {
        printf("answers don't match :(, ");
        bdd_and_accumulate(fg->m, &msgans, actans);
        if(msgans == actans)
          printf("but msg_ans is an overapproximation of actual_ans :)\n");
        else
          printf("and msg_ans is NOT an overapproximation of actual_ans :(\n");
      }
      bdd_free(fg->m, msgans);
      bdd_free(fg->m, actans);
    }
    else if(!strcmp(command, "setvar"))
    {
      printf("enter var id -> ");
      scanf("%d", &i);
      bdd_ptr var = bdd_new_var_with_index(fg->m, i);
      bdd_ptr temp;
      do {
        printf("enter value (0 or 1) -> ");
        scanf("%d", &j);
      }while(j*j - j);//while j is neither 0 nor 1
      
      if(j) temp = bdd_dup(var);
      else  temp = bdd_not(var);
      printf("assigning var returns %d\n", factor_graph_assign_var(fg, temp));
      bdd_free(fg->m, var);
      bdd_free(fg->m, temp);
    }
    else if(!strcmp(command, "converge"))
    {
      printf("passing messages...\n");
      printf("convergence took %d iterations.", factor_graph_converge(fg));
    }
    else if(!strcmp(command, "checkpoint"))
      printf("checkpoint %d set\n", ++(fg->time));
    else if(!strcmp(command, "rollback"))
    {
      factor_graph_rollback(fg);
      printf("rolled back to checkpoint %d\n", fg->time);
    }
    else if(!strcmp(command, "verify"))
      printf("verification returned %d\n", factor_graph_verify(fg));
    else if(!strcmp(command, "print"))
    {
      printf("F G file -> ");
      scanf("%s", param1);
      printf("dot file -> ");
      scanf("%s", param2);
      factor_graph_print(fg, param2, param1);
    }
    else if(!strcmp(command, "createpng"))
    {
      printf("Name of dot file (without extension) -> %s", param2);
      scanf("%s", param1);
      
      sprintf(param2, "%s.dot", param1);
      factor_graph_print(fg, param2, "tempfile101");
      system("rm -f tempfile101");
      
      sprintf(param2, "fdp -Tpng -o %s.png %s.dot", param1, param1);
      printf("%s\n", param2);
      system(param2);
      sprintf(param2, "eog %s.png &", param1);
      printf("%s\n", param2);
      system(param2);
    }
    else if(!strcmp("makeacyclic", command))
    {
      printf("enter lambda -> ");
      scanf("%d", &i);
      printf("enter starting variable node id -> ");
      scanf("%d", &j);
      fgnode_list *vl = fg->vl;
      if(vl != NULL) do
      {
        if(vl->died <= fg->time)
          continue;
        if(vl->n->id == j)
          break;
        vl = vl->next;
      }while(vl != fg->vl);
      if(vl->n->id != j)
      {
        printf("Invalid node id\n");
        continue;
      }
      nextv = factor_graph_make_acyclic(fg, vl->n, i);
      if(nextv == NULL)
        printf("Successfully made acyclic\n");
      else
      {
        printf("Unsuccessful.\nWe suggest you get rid of var %d\n", bdd_get_lowest_index(fg->m, nextv));
        bdd_free(fg->m, nextv);
      }
    }
    else
      printf("Unable to parse command. \nType `help\' to see list of commands.\n");
    
  }while(1);
  
  /* Delete the factor graph */
  factor_graph_delete(fg);
  return 0;
}

int main1(int argc, char ** argv)
{
  DdManager *ddm;
  int numvars, numclauses;
  int **cnf, *clssz;
  int i;
  int j;
  char commentline[1001];
  bdd_ptr *funcs;
  factor_graph *fg;
  bdd_ptr nextv;

  /* Initialize the DD Manager*/
  ddm = ddm_init();

  /* Input the problem*/

  scanf("p cnf %d %d", &numvars, &numclauses);
  printf("numclauses is %d\n", numclauses);
  printf("numvars is %d\n", numvars);

  cnf = (int **)malloc(sizeof(int *) * numclauses);
  clssz = (int *)malloc(sizeof(int) * numclauses);
  common_error(cnf, "main.c : out of mem while inputting problem\n");
  common_error(clssz, "main.c : out of mem while inputting problem\n");
  for(i = 0; i < numclauses; i++)
  {
    cnf[i] = (int *)malloc(sizeof(int) * MAX_CLAUSE_SIZE);
    common_error(cnf[i], " out of mem while inputting problem\n");
    clssz[i] = 0;
    scanf("%d", &j);
    while(j != 0)
    {
      assert(clssz[i] < MAX_CLAUSE_SIZE && "MAX_CLAUSE_SIZE insufficient\n");
      cnf[i][clssz[i]] = j;
      clssz[i]++;
      scanf("%d", &j);
    }
  }
  /* Create the factor graph*/
  funcs = vector_to_bdd(ddm, cnf, clssz, numclauses, &j);
  common_error(funcs, "main.c : could not convert array to bdd\n");
  fg = factor_graph_new(ddm, funcs, numclauses);
  printf("factor graph created\n");
  fflush(stdout);
  
  fgtest1(fg);
  //fgtest2(fg);
  //fgtest3(fg, LAMBDA);
  
  //factor_graph_print(fg, "fg1.dot", "temp.txt");
  //fg->time++;
  //factor_graph_make_acyclic(fg, fg->vl->n, fg->num_vars);
  //factor_graph_rollback(fg);
  //factor_graph_print(fg, "fg2.dot", "temp.txt");
  
  //printf("convergence took %d iterations\n", factor_graph_converge(fg));

  /* Destroy the factor graph and the DD Manager*/
  fgdm("deleting fg", 0);
  factor_graph_delete(fg);
  fgdm("done", 0);
  Cudd_Quit(ddm);
  return 0;
}

void fgtest3(factor_graph *fg, fgnode *n, int l)
{
  bdd_ptr nextv, notnextv;
  nextv = factor_graph_make_acyclic(fg, n, l);
  if(nextv == NULL)
  {
    fgdm("made acyclic", 0);
    assert(factor_graph_converge(fg) >= 0);
    return;
  }
  fgdm("still not acyclic", 0);
  //fgdm("make_acyclic failed", fg->num_vars);
  fg->time++;
  fgdm("setting to true", bdd_get_lowest_index(fg->m, nextv));
  assert(factor_graph_assign_var(fg, nextv) == 0);
  fgtest3(fg, n, l);
  factor_graph_rollback(fg);
  fg->time++;
  notnextv = bdd_not(nextv);
  fgdm("setting to false", bdd_get_lowest_index(fg->m, nextv));
  assert(factor_graph_assign_var(fg, notnextv) == 0);
  bdd_free(fg->m, notnextv);
  fgtest3(fg, n, l);
  fgdm("done with", bdd_get_lowest_index(fg->m, nextv));
  factor_graph_rollback(fg);
}

void fgtest2(factor_graph *fg)
{
  int i, j;
  bdd_ptr nextv;
  for(i = fg->num_vars; i >= 5; i = (int)(i * .9))
  {
    //fgdm("iter", i);
    fg->time++;
    j = -1;
    do {
      while(fg->vl->died <= fg->time)
        fg->vl = fg->vl->next;
      nextv = factor_graph_make_acyclic(fg, fg->vl->n, i);
      fgdm("made acyclic", 0);
      j++;
      if(nextv == NULL)
        break;
      fgdm("make_acyclic failed", fg->num_vars);
      factor_graph_assign_var(fg, nextv);
      fgdm("assigned var", 0);
    }while(nextv != NULL);
    printf("LAMBDA = %d, vars eliminated = %d, convergence = %d\n", i, j, factor_graph_converge(fg));
    fflush(stdout);
    //factor_graph_print(fg, "acyclic.dot", "fgoutput.txt");
    factor_graph_rollback(fg);
    assert(factor_graph_verify(fg) > 0);
  }
}

void fgtest1(factor_graph *fg)
{
  int j;
  j = factor_graph_verify(fg);
  if(j < 0)
    printf("verification failed\n");

  printf("convergence took %d iterations\n", factor_graph_converge(fg));

  
  fg->time++;
  /* Try to make the factor graph acyclic*/
  if(factor_graph_make_acyclic(fg, fg->vl->n, LAMBDA) != NULL)
  {
    printf("make_acyclic failed :(\n");
  }
  else
    printf("make_acyclic apparently succeeded :/\n");

  fgdm("verifying acyclic graph", factor_graph_verify(fg));
  

  printf("convergence took %d iterations\n", factor_graph_converge(fg));
  
  fgdm("rolling back", 0);
  factor_graph_rollback(fg);
  fgdm("verifying after rollback : ", factor_graph_verify(fg));
  printf("convergence took %d iterations\n", factor_graph_converge(fg));
}

DdManager* ddm_init()
{
  DdManager *m;
  //UNIQUE_SLOTS = 256
  //CACHE_SLOTS = 262144
  m = Cudd_Init(0, 0, 256, 262144, 0);
  common_error(m, "main.c : Could not initialize DdManager\n");
  Cudd_SetMaxMemory(m, (unsigned long) MAX_MEM_MB * (unsigned long)1000000);
  printf("max memory allowed : %d MB\n", (int)(Cudd_ReadMaxMemory(m)/1000000));
  return m;
}
