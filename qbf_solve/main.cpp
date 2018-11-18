#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <memory>
#include <time.h>
#include <string>
#include <srt.h>
using namespace std;

#ifdef DEBUG
#define DBG(stmt) stmt
#else
#define DBG(stmt) (void)0
#endif

#ifdef DEBUGMAIN
#define DBGMAIN(stmt) stmt
#else
#define DBGMAIN(stmt) (void)0
#endif

bool contains(DdManager *d, bdd_ptr varDep, bdd_ptr block) {
  bdd_ptr temp = bdd_cube_intersection(d, varDep, block);
  if(bdd_is_one(d, temp))
  {
    bdd_free(d, temp);
    return false;
  }
  bdd_free(d, temp);
  return true;
}

bool QBF_Solve(char* filename) {

  const int threshold = 1;
  int start, end;
  std::shared_ptr<SRT> t;
  Quant* q;
  CNF_vector* c;
  bdd_ptr varDep,supp;
  QBlock* qb;
  SRTNode_Queue leaves;
  SRTNode* curr;
  CNF_vector* cv;
  bool does_contain;
  SRTNode* Lnew;
  Vector_Int* ss;
  DdManager* d;
  SRTNode* root;
  bdd_ptr cv_bdd;
  bdd_ptr res,compand;
  bdd_ptr block;
  BDD_List* cv_bdd_list;

  t = std::make_shared<SRT>();
  DBG(cout<<"parsing..."<<endl);
  t->parse_qdimacs_srt(filename);
  d = t->SRT_getddm();
  varDep = bdd_one(d);
  q = t->SRT_vars();
	c = t->SRT_clauses();

	//  clock_t big_init, big_final;
	//  clock_t init, final;
	//  double time_merge = 0, time_fg = 0, time_convert = 0, time_forall = 0;

	//	t->SRT_getroot()->split(197);
	//	t->SRT_getroot()->SRT_Then()->split(84);
	//	t->SRT_getroot()->SRT_Else()->split(84);

	//  big_init = clock();
	DBGMAIN(cout<<"entering loop"<<endl);  
	while(!q->empty()) {
		DBGMAIN(cout<<"new iteration"<<endl);
		DBGMAIN(t->SRT_print());
		qb = q->innermost();
		start = qb->start;
		end = qb->end;
		DBGMAIN(cout<<"start = "<<start<<" end = "<<end<<endl);

		// MERGING
		//  init=clock();
		for(int v=start; v<end; v++)
			t->SRT_merge(v,threshold);
		//  final=clock();
		//  time_merge += (double)(final-init) / ((double)CLOCKS_PER_SEC);

		DBGMAIN(cout<<"computing varcube..."<<endl);
		block = bdd_one(d);
		for(int i=start;i<end; i++) {
			bdd_ptr temp_var = bdd_new_var_with_index(d,i);
			bdd_and_accumulate(d,&block,temp_var);
			bdd_free(d,temp_var);
		}
		does_contain = contains(d, varDep,block);

		if(qb->is_universal) {
			DBGMAIN(cout<<"Universal Quantification"<<endl);
			c->drop_vars(start,end);

			if(does_contain) {

				// FOR EACH LEAF, OPTIMIZE LATER BY TRAVERSING AND MAINTAINING A LIST OF LEAVES
				leaves.push(t->SRT_getroot());
				while(!leaves.empty()) {
					curr = leaves.front();
					leaves.pop();
					assert(curr != NULL);
					if(curr->is_leaf()) {
						//  init = clock();
						curr->for_all_innermost(d,q);
						curr->deduce();
						//  final=clock();
						//  time_forall += (double)(final-init) / ((double)CLOCKS_PER_SEC);
					}
					else {
						leaves.push(curr->SRT_Then());
						leaves.push(curr->SRT_Else());
					}
				}
			}
			// ELSE NOTHING TO DO, VARIABLES FROM GLOBAL CNF_VECTOR ALREADY DROPPED

		}
		else {	// EXISTENTIAL QUANTIFICATION
			DBGMAIN(cout<<"Existential Quantification"<<endl);
			cv = c->filter_innermost(q);
			/*
				 for(int cvs = 0; cvs < cv->clauses.size(); cvs ++)
				 {
				 DBG(cout<<"filtered clause ");
				 for(int cvscs = 0; cvscs < cv->clauses[cvs]->vars.size(); cvscs++)
				 DBG(cout<<cv->clauses[cvs]->vars[cvscs]<<" ");
				 DBG(cout<<endl);
				 }
				 */
			//cv_bdd = CNF_to_BDD(cv, d);
			cv_bdd_list = CNF_to_BDD_List(cv, d);
			DBGMAIN(cout<<"cv has "<<cv->clauses.size()<<" out of "<<c->clauses.size()<<" clauses"<<endl);

			if (does_contain) {
				DBGMAIN(cout<<"some leaf contains the vars..."<<endl);
				// FOR EACH LEAF, OPTIMIZE LATER BY TRAVERSING AND MAINTAINING A LIST OF LEAVES
				leaves.push(t->SRT_getroot());
				while(!leaves.empty()) {
					curr = leaves.front();
					leaves.pop();
					if(curr->is_leaf()) {	// FOR EACH LEAF
						curr->append(cv_bdd_list);

						//FACTOR GRAPH
						//  init = clock();	      
						//apply the split history of L to CV, add these clauses to L
						factor_graph *fg = make_factor_graph(d, curr);
						factor_graph_eliminate(fg, start, end, curr);
						factor_graph_delete(fg);
						curr->deduce();
						//  final=clock();
						//  time_fg += (double)(final-init) / ((double)CLOCKS_PER_SEC);
						/*curr->join_bdds(d);
							res = bdd_forsome(d,(*(curr->get_func()))[0],block);
							bdd_free(d, (*(curr->get_func()))[0]);
							(*(curr->get_func()))[0] = res;
							*/

					}
					else {
						leaves.push(curr->SRT_Then());
						leaves.push(curr->SRT_Else());
					}
				}
			}
			else {			// no leaves depend on V


				DBGMAIN(cout<<"no leaf contains the vars..."<<endl);
				Lnew = make_leaf(t->SRT_getddm(), cv);

				// no leaf depends on V, form a global leaf from CV


				//FACTOR GRAPH
				//  init = clock();	      
				factor_graph *fg = make_factor_graph(d, Lnew);
				factor_graph_eliminate(fg, start, end, Lnew);
				factor_graph_delete(fg);
				//  final=clock();
				//  time_fg += (double)(final-init) / ((double)CLOCKS_PER_SEC);

				// FOR EACH LEAF, OPTIMIZE LATER BY TRAVERSING AND MAINTAINING A LIST OF LEAVES
				leaves.push(t->SRT_getroot());
				while(!leaves.empty()) {
					curr = leaves.front();
					leaves.pop();
					if(curr->is_leaf()) {	// FOR EACH LEAF
						curr->append(Lnew);
						curr->deduce();
					}
					else {
						leaves.push(curr->SRT_Then());
						leaves.push(curr->SRT_Else());
					}
				}

			}

			ss = cv->support_set();	// CV.support_set

			for(Vector_Int::iterator i = ss->begin(); i!=ss->end();) {
				if((*i) >= start && (*i) < end)
					i = ss->erase(i);
				else
					i++;
			}	// CV.support_set - V

			// Now construct the bdd having these variables
			if(!ss->empty())
			{
				supp = bdd_one(d);
				for(int i=0;i<ss->size(); i++) {
					bdd_ptr temp = bdd_new_var_with_index(d,(*ss)[i]);
					bdd_and_accumulate(d,&supp,temp);
					bdd_free(d, temp);
				}

				bdd_and_accumulate(d,&varDep,supp);
				bdd_free(d, supp);
				// mark variables coming from CV into T*/
			}
		}

		DBGMAIN(printf("post removal : \n"));
		DBGMAIN(t->SRT_print());

		// MERGING

		//  init = clock();
		for(int v=end-1; v>=start; v--) {
			t->SRT_merge(v,threshold);
		}
		//  final=clock();
		//  time_merge += (double)(final-init) / ((double)CLOCKS_PER_SEC);


		//    assert(t->SRT_getroot()->is_leaf());
		//    printf("SIZE OF FUNC ON ROOT: %d\n",t->SRT_getroot()->type.leaf.func->size());
		//    t->SRT_getroot()->type.leaf.print(d);
		//    assert(t->SRT_getroot()->type.leaf.check_func(d,bdd_or(d,bdd_new_var_with_index(d,1),bdd_new_var_with_index(d,2))));


		q->remove_innermost();
		DBGMAIN(printf("post merge : \n"));
		DBGMAIN(t->SRT_print());
	}
	assert(t->SRT_getroot());
	assert(t->SRT_getroot()->is_leaf());

	//  big_final = clock();


	//  cout<<"Time for Merging: "<<time_merge<<endl;
	//  cout<<"Time for Factor Graph: "<<time_fg<<endl;
	//  cout<<"Time for Forall: "<<time_forall<<endl;
	//  cout<<"Time for BDD-CNF Conversion: "<<time_convert<<endl;

	//  profile();
	//  cout<<"Total Time: "<<(double)(big_final-big_init)/((double)CLOCKS_PER_SEC)<<endl;



	//assert(t->SRT_getroot()->get_func()->size()==1);
	for(int g = 0; g < t->global_clauses->clauses.size(); g++)
	{
		assert(t->global_clauses->clauses[g]->vars.size() == 0);
		return false;
	}
	for(int g = 0; g < t->SRT_getroot()->get_func()->size(); g++)	// size == 1??
		if( !bdd_is_one(t->SRT_getddm(),  (*(t->SRT_getroot()->get_func())) [g] ) )
			return false;
	return true;
}

int main(int argc, char **argv) {
	/*
		 int n;
		 printf("Enter number of variables: ");
		 scanf("%d",&n);
		 SRT* t = new SRT(n);
		 int option;
		 int mergevar;
		 int threshold;

		 while(true) {
		 printf("Select Option:\n");
		 printf("1. Print Tree\n");
		 printf("2. Split a node\n");
		 printf("3. Merge a node\n");
		 printf("4. Exit\n");
		 scanf("%d",&option);
		 std::string splitnode;
		 int var;

		 switch(option) {
		 case 1:
		 t->SRT_print();
		 break;
		 case 2:
		 printf("Enter the string for the node: ");
		 std::cin>>splitnode;
		 printf("Enter the variable: ");
		 scanf("%d",&var);
		 t->split(splitnode,var);
		 t->SRT_print();
		 break;
		 case 3:
		 printf("Enter the variable to merge on: ");
		 scanf("%d",&mergevar);
		 printf("Enter the threshold for merging: ");
		 scanf("%d",&threshold);
		 t->SRT_merge(mergevar,threshold);
		 t->SRT_print();
		 break;
		 case 4:
		 exit(0);
		 break;
		 default:
		 printf("Sorry, option unavailable\n");
		 }
		 }
		 return 0;*/
	if(argc != 2)
	{
		cout<<"Usage : "<<endl<<"\t"<<argv[0]<<" <filename>"<<endl;
		return 0;
	}
	bool res = QBF_Solve(argv[1]);
	if(res)
		cout<<"\t\t\t====================================\n"
			<<"\t\t\t========  Result is TRUE  ==========\n"
			<<"\t\t\t===================================="
			<<endl;
	else
		cout<<"\t\t\t====================================\n"
			<<"\t\t\t========  Result is FALSE  =========\n"
			<<"\t\t\t===================================="
			<<endl;
}


