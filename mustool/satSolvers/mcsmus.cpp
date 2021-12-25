#ifdef UMCSMUS

#include <errno.h>
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "mustool/satSolvers/MSHandle.h"
#include "mustool/mcsmus/mcsmus/minisat-wrapper.hh"
#include "mustool/mcsmus/mcsmus/glucose-wrapper.hh"
#include "mustool/mcsmus/mcsmus/minisolver.hh"
#include "mustool/mcsmus/mcsmus/parseutils.hh"
#include "mustool/mcsmus/mcsmus/options.hh"
#include "mustool/mcsmus/mcsmus/mcsmus.hh"
#include "mustool/mcsmus/mcsmus/dimacs.hh"
#include "mustool/mcsmus/mcsmus/system.hh"

using namespace mcsmus;

std::vector<Lit> intToLit(std::vector<int> cls){
	std::vector<Lit> lits;
	//we are taking the clasue from the MSHandle class, and each clause contain additional control literal
	//here we have to exclude the control literal
	//TODO: we should not store the control literals in MSHandle.clauses, get rid of them
	for(int i = 0; i < cls.size() - 1; i++){
		auto l = cls[i];
		int var = abs(l)-1;
		lits.push_back( mkLit(var, l<0) );
	}
	return lits;
}

std::vector<bool> MSHandle::shrink_mcsmus(std::vector<bool> &f, std::vector<bool> crits){
	setX86FPUPrecision();
	Wcnf wcnf;
	std::unique_ptr<BaseSolver> s;
	s = make_glucose_simp_solver();
	MUSSolver mussolver(wcnf, s.get());

	
	Control* control = getGlobalControl();;
	control->verbosity = 0;
	control->status_interval = 10;
	mussolver.single_mus = true;
	mussolver.initializeControl();
	control->force_exit_on_interrupt();

	//add the clauses
	std::vector<int> constraintGroupMap;
	int cnt = 0;
	for(int i = 0; i < dimension; i++){
		if(f[i]){
			if(crits[i]){
				wcnf.addClause(intToLit(clauses[i]), 0);
			}else{
				constraintGroupMap.push_back(i);
				cnt++;
				wcnf.addClause(intToLit(clauses[i]), cnt);
			}
		}
	}
	
	std::vector<Lit> mus_lits;
	wcnf.relax();
	mussolver.find_mus(mus_lits, false);

	std::vector<bool> mus(dimension, false);
	for(auto b : mus_lits){
		int gid = wcnf.bvarIdx(b);
		if(gid > 0){
			mus[constraintGroupMap[gid - 1]] = true;
		}	
	}
	for(int i = 0; i < dimension; i++){
		if(crits[i]){
			mus[i] = true;
		}
	}
	return mus;
}
#endif
