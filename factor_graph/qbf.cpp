#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <sstream>
#include "qbf.h"

#define absolute(x) (x<0 ? -x : x)
#define absolute(x) (x<0 ? -x : x)
#define literal(x,sign) (sign=='1' ? x : -x)
#define transform(str) (str[0]=='n' ? transform_temp(str)-no_inputs+start_tempvar-1 : transform_var(str))

typedef std::vector<std::string> vs;

/* Functions on struct Quant */
Quant::Quant() {
	blocks.clear();
	ddm = NULL;
	vars.clear();
	dimacs_srt = NULL;
	srt_dimacs = NULL;
	return;
}

Quant::Quant(DdManager* ddm, int n) {
  this->ddm = ddm;
  vars.resize(n+1);
  dimacs_srt = new int[n+1];
  srt_dimacs = new int[n+1];
  for(int i=1;i<=n;i++)
  {
    vars[i] = bdd_new_var_with_index(ddm, i);
    dimacs_srt[i] = -1;
    srt_dimacs[i] = -1;
  }
  return;
}

void Quant::print()
{
  std::cout<<"Blocks : \n";
  for(int i = 0; i < blocks.size(); i++)
  {
    QBlock *q = blocks[i];
    std::cout<<(q->is_universal ? "for all " : "for some ")<<q->start<<" to "<<q->end-1<< "\n";
  }
  /*std::cout<<"dimacs_srt : ";
    for(int i = 1; i < vars.size(); i++)
    std::cout<<dimacs_srt[i]<<" ";
    std::cout<<"srt_dimacs : ";
    for(int i = 1; i < vars.size(); i++)
    std::cout<<srt_dimacs[i]<<" ";
   */
  /*for(int i = 1; i < vars.size(); i++)
    {
    assert(i == absolute(srt_dimacs[absolute(dimacs_srt[i])]));
    assert(i == absolute(dimacs_srt[absolute(srt_dimacs[i])]));
    }*/
  std::cout<<std::endl;
}

int Quant::no_vars() {
  return vars.size();
}

bool Quant::empty() {
//  DBG(std::cout<<"blocks has "<<blocks.size()<<" elements."<<std::endl);
  return (blocks.size() == 0);
}

QBlock* Quant::innermost() {
  return blocks.back();
}

void Quant::remove_innermost() {
  assert(blocks.size() > 0);
  QBlock* t = blocks.back();
  blocks.pop_back();
  free(t);
}

int Quant::is_universal_innermost() {
  assert(blocks.size() > 0);
  return blocks.back()->is_universal;
}

int Quant::is_existential_innermost() {
  assert(blocks.size() > 0);
  return 1 - blocks.back()->is_universal;
}

int Quant::is_universal(int v) {
  assert(v < vars.size());
  return (srt_dimacs[v]<0 ? 0 : 1);
}

int Quant::is_existential(int v) {
  assert(v < vars.size());
  return (srt_dimacs[v]<0 ? 1 : 0);
}

bdd_ptr Quant::get_var_quant_index (int quant_index) {
  assert(quant_index < vars.size() && quant_index > 0);
  return vars[quant_index];
}

Quant::~Quant() {
  int i;
  if(dimacs_srt) delete dimacs_srt;
  if(srt_dimacs) delete srt_dimacs;
  for(i = 1; i < vars.size(); i++)
    bdd_free(ddm, vars[i]);
}

/* Functions of MClause */
int MClause::no_vars() {
  return vars.size();
}

void MClause::print() {
	for(int i=0;i<vars.size();i++)
		std::cout<<vars[i]<<" ";
	std::cout<<std::endl;
}

void MClause::drop_vars(int start, int end) {
  for(Vector_Int::iterator iter = vars.begin() ; iter < vars.end(); ++iter) {
    int var = absolute(*iter);
    if(var >= start && var < end)
    {
      //DBG(std::cout<<"erasing var "<<var<<std::endl);
      vars.erase(iter);
      iter--;
    }
  }
  return;
}

/* Functions of CNF_vector */
CNF_vector::CNF_vector():clauses(0) {
  return;
}

CNF_vector::CNF_vector(int F):clauses(F) {
  for(int i = 0; i < F; i++)
    clauses[i] = NULL;
  return;
}

int CNF_vector::no_clauses() {
  return clauses.size();
}

void CNF_vector::drop_vars(int start, int end) { // drops (universally quantifes) variables in vars
  int i, no_cl = no_clauses();
  for(i=0;i<no_cl;i++) {
    //DBG(std::cout<<"filtering clause "<<i<<std::endl);
    clauses[i]->drop_vars(start, end);
  }
  return;
}

/*CNF_vector* CNF_vector::filter(Vector_Int& inp_vars) {	// returns the set of clauses that depend on V, and DELETES THEM,
  int i,j,k,clause_found, inp_no_vars = inp_vars.size(), no_cl = no_clauses();
  CNF_vector* ret = new CNF_vector();
  for(i=0;i<no_cl;i++) {
  clause_found = 0;
  for(j=0;j<clauses[i]->no_vars() && !clause_found ;j++) {
  for(k=0;k<inp_no_vars;k++) {
  if(absolute(clauses[i]->vars[j]) == inp_vars[k]) {
  ret->clauses.push_back(clauses[i]);
  clause_found = 1;
  break;
  }
  }
  }
  }
  return ret;
  }*/

CNF_vector* CNF_vector::filter_innermost(Quant* q) {	// filters clauses depending variables from innermost quantifier block of q, ideally only this is required
  int j, temp, start = q->innermost()->start, end = q->innermost()->end;
  MClause_List::iterator i;
  CNF_vector* ret = new CNF_vector();
  int found;
  for(i = clauses.begin(); i != clauses.end(); )
  {
    found = 0;
    for(j=0;j<(*i)->no_vars();j++)
    {
      temp = absolute((*i)->vars[j]);
      //DBG(std::cout<<"comparing temp = "<<temp<<" with ["<<start<<", "<<end<<")"<<std::endl);
      if(temp >= start && temp < end)
      {
	ret->clauses.push_back(*i);
	i = clauses.erase(i);
	found = 1;
	break;
      }
    }
    if(!found)	// go to next clause
	i++;
  }
  return ret;
}

Vector_Int* CNF_vector::support_set() {				// returns variables in support set
  int i,j,no_cl = no_clauses();
  Set_Int ans;
  for(i=0;i<no_cl;i++) {
    for(j=0;j<clauses[i]->no_vars();j++) {
      ans.insert(absolute(clauses[i]->vars[j]));
    }
  }
  Vector_Int* ret = new Vector_Int(ans.begin(),ans.end());
  return ret;
}

void CNF_vector::print() {
	for(int i=0;i<clauses.size();i++)
		clauses[i]->print();
	std::cout<<std::endl;
}

/* BDD-CNF Conversions */

CNF_vector* BDD_to_CNF_helper (DdManager* d, Quant* q, bdd_ptr bdd, bool val) {
  if(Cudd_IsConstant(bdd)) {
  	bool actualval;
  	if(Cudd_IsComplement(bdd))
  		actualval = !Cudd_V(bdd);
  	else
  		actualval = Cudd_V(bdd);
  		
    if(val == actualval) {	// val should be the actual value of node
			CNF_vector* ans = new CNF_vector();
			ans->clauses.push_back(new MClause());
			return ans;
    }
    else{
			CNF_vector* ans = new CNF_vector();
			return ans;
    }
	}

  int var = Cudd_NodeReadIndex(bdd);//Cudd_ReadPerm(d,Cudd_NodeReadIndex(bdd));//bdd->index;
  bool iscomp = Cudd_IsComplement(bdd);
  bool newval = (iscomp ? !val : val);		// Find the opposite valued path if this node is complemented
  CNF_vector* t = BDD_to_CNF_helper(d, q,Cudd_T(bdd),newval);
  CNF_vector* e = BDD_to_CNF_helper(d, q,Cudd_E(bdd),newval);

  for(MClause_List::iterator it = t->clauses.begin(); it!=t->clauses.end(); ++it) {
    (*it)->vars.push_back(-var);
	}

  for(MClause_List::iterator it = e->clauses.begin(); it!=e->clauses.end(); ++it) {
    (*it)->vars.push_back(var);
  }

  CNF_vector* ans = new CNF_vector();
  ans->clauses.insert(ans->clauses.end(),t->clauses.begin(),t->clauses.end());
  ans->clauses.insert(ans->clauses.end(),e->clauses.begin(),e->clauses.end());

  delete t;
  delete e;

  return ans;
}

CNF_vector* BDD_to_CNF (DdManager* d, Quant* q, bdd_ptr bdd) {
	return BDD_to_CNF_helper(d,q,bdd,false);
}

CNF_vector* BDD_List_to_CNF (DdManager* d, Quant* q, BDD_List* bdds) {
	CNF_vector* cnf = new CNF_vector();
	cnf->clauses.clear();
	
	for(int i=0;i<bdds->size();i++) {
		CNF_vector* temp = BDD_to_CNF(d,q,bdds->at(i));
		cnf->clauses.insert(cnf->clauses.end(),temp->clauses.begin(),temp->clauses.end());
		delete temp;
	}
	return cnf;
}

BDD_List* CNF_to_BDD_List(CNF_vector *cnf, DdManager *dd) {
  BDD_List *ans = new BDD_List(cnf->clauses.size());
  for(int i = 0; i < cnf->clauses.size(); i++)
  {
    bdd_ptr temp = bdd_zero(dd);
    MClause * mc = cnf->clauses[i];
    for(int j = 0; j < mc->vars.size(); j++)
    {
      bdd_ptr temp2 = bdd_new_var_with_index(dd, absolute(mc->vars[j]));
      bdd_ptr temp3;
      if(mc->vars[j] > 0) {
				temp3 = temp2;
			}
      else
      {
				temp3 = bdd_not(dd, temp2);
				bdd_free(dd, temp2);
      }
      bdd_or_accumulate(dd, &temp, temp3);
      bdd_free(dd, temp3);
    }
    (*ans)[i] = temp;
  }
  return ans;
}

bdd_ptr CNF_to_BDD (CNF_vector* cnf, DdManager *m) {
  bdd_ptr ans = bdd_one(m);
  for(int i = 0; i < cnf->clauses.size(); i++)
  {
    bdd_ptr temp = bdd_zero(m);
    MClause * mc = cnf->clauses[i];
    for(int j = 0; j < mc->vars.size(); j++)
    {
      bdd_ptr temp2 = bdd_new_var_with_index(m, absolute(mc->vars[j]));
      bdd_ptr temp3;
      if(mc->vars[j] > 0)
				temp3 = temp2;
      else
      {
				temp3 = bdd_not(m, temp2);
				bdd_free(m, temp2);
      }
      bdd_or_accumulate(m, &temp, temp3);
      bdd_free(m, temp3);
    }
    bdd_and_accumulate(m, &ans, temp);
    bdd_free(m, temp);
  }

  return ans;
}

void CNF_to_BLIF(CNF_vector* cnf, const char* filename) {
	std::ofstream fout(filename);
	
	fout<<"# Instance \"QBF_Solve\" generated by SRT"<<std::endl;
	fout<<".model QBF_Solve"<<std::endl;
	fout<<".inputs ";
	Vector_Int* ss = cnf->support_set();
	for(int i=0;i<ss->size();i++) {
		fout<<"var"<<ss->at(i)<<" ";
	}
	fout<<std::endl;
	fout<<".outputs f"<<std::endl;
	
	for(int i=0;i<cnf->clauses.size();i++) {
		fout<<".names ";
		
		for(int j=0;j<cnf->clauses[i]->vars.size();j++) {
			fout<<"var"<<absolute(cnf->clauses[i]->vars[j])<<" ";
		}
		fout<<"clause"<<i<<std::endl;
		
		for(int j=0;j<cnf->clauses[i]->vars.size();j++) {
			for(int k=0;k<j;k++)
				fout<<"-";
				
			if(cnf->clauses[i]->vars[j] > 0)
				fout<<1;
			else
				fout<<0;
				
			for(int k=j+1;k<cnf->clauses[i]->vars.size();k++)
				fout<<"-";
			
			fout<<" 1"<<std::endl;
		}
	}
	
	fout<<".names ";
	for(int i=0;i<cnf->clauses.size();i++) {
		fout<<"clause"<<i<<" ";
	}
	fout<<"f"<<std::endl;
	
	for(int i=0;i<cnf->clauses.size();i++) {
		fout<<"1";
	}
	fout<<" 1"<<std::endl;
	fout<<".end"<<std::endl;
	
	fout.close();
	return;
}

int transform_temp(std::string str) {
	std::string newstr = str.substr(1,str.length());
	std::stringstream oss;
	oss<<newstr;
	int ans;
	oss>>ans;
	return ans;
}

int transform_var(std::string str) {
	std::string newstr = str.substr(3,str.length());
	std::stringstream oss;
	oss<<newstr;
	int ans;
	oss>>ans;
	return ans;
}

void add_clause(CNF_vector* ans, int a, int b=0, int c=0) {
	MClause* cl = new MClause();
	if(a) 
		cl->vars.push_back(a);
	if(b) 
		cl->vars.push_back(b);
	if(c)
		cl->vars.push_back(c);
	
	ans->clauses.push_back(cl);
	return;
}

CNF_vector* BLIF_to_CNF (const char* input, int start_tempvar) {
	if(!input)
		return NULL;
		
	std::string line;
	std::string word;
	vs local_inputs;
	int no_inputs = 0;	// all inputs
	int f,a,b,c;
	std::string input_rule;
	
	CNF_vector* ans = new CNF_vector();
	std::ifstream fin(input);
	
	getline(fin,line);	// comment
	getline(fin,line);	// .model
	fin>>word;
	assert(word == ".inputs");
	
	while(fin>>word) {
		if(word == ".outputs")
			break;
		no_inputs++;
	}
	fin>>word;
	assert(word=="f");
	
	// variable index of nX = X-no_inputs+start_tempvar-1
	
	fin>>word;
	
	while(1) {	// loop over set of rules
		assert(word==".names");
		local_inputs.clear();
		while(fin>>word) {	// loop over 1 rule
			if((int)word[0] > 57) {
				local_inputs.push_back(word);
			}
			else
				break;
		}
		
		// Now tempstr has the rule of inputs if no_inp_vars > 0 else just the value of output, we just have to dump those as they are
		if(local_inputs.size() > 0) {	// last was input rule if there were any input variables, now take output rule
			input_rule = word;
			fin>>word;
			assert(word=="1");
			assert(input_rule.length()<=2 && local_inputs.size() == input_rule.length()+1);
			if(input_rule.length() == 1) {	// f = a
				if(local_inputs[1]=="f")
					f = 0;			// In this case, only "a" will be added as clause
				else
					f = literal(transform(local_inputs[1]),'1');
				a = literal(transform(local_inputs[0]),input_rule[0]);
				// (f v ~a) ^ (~f v a)
				add_clause(ans,-f,a);
				if(f)		// if f is "f", just add the clause "a"
					add_clause(ans,f,-a);
			}
			else {	// f = a ^ b
				if(local_inputs[2]=="f")
					f = 0;
				else
					f = literal(transform(local_inputs[2]),'1');
				a = literal(transform(local_inputs[0]),input_rule[0]);
				b = literal(transform(local_inputs[1]),input_rule[1]);
				// (~f v a) ^ (~f v b) ^ (f v ~a v ~b)
				add_clause(ans,-f,a);
				add_clause(ans,-f,b);
				if(f)
					add_clause(ans,f,-a,-b);
			}
		}
		else {	// f = 1
			assert(word=="1");
			if(local_inputs[0]=="f")
				f = 0;
			else
				f = literal(transform(local_inputs[0]),'1');
			// f
			add_clause(ans,f);
		}
		fin>>word;
		if(word == ".end")	// end of input
			break;
	}
	
	return ans;	
}

void write_blif(DdManager* d, Quant* q, BDD_List* bdds, const char* filename) {
	if(!filename)
		return;
	
	CNF_vector* cnf = BDD_List_to_CNF(d,q,bdds);
	CNF_to_BLIF(cnf,filename);
	return;
}

