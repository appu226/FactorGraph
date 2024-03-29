#include "mustool/satSolvers/Dimacs.h"
#include "mustool/core/misc.h"
#include "mustool/satSolvers/MSHandle.h"
#include <sstream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <random>
#include <stdlib.h>   
#include <time.h>   
#include <cstdio>
#include <assert.h>

using namespace CustomMinisat;
using namespace std;

Lit itoLit(int i){
    bool sign = i < 0;
    int var = (sign)? -i-1 : i-1;
    return (sign) ? ~mkLit(var) : mkLit(var);
}

int Littoi(Lit l){
    return (var(l)+1) * (sign(l) ? -1 : 1);
}

vector<int>  convert_clause(string clause){
    vector<int> ret;
    istringstream is(clause);
    int n;
    while(is >> n)
        ret.push_back(n);
    ret.pop_back();
    return ret;
}

MSHandle::MSHandle(string filename):SatSolver(filename){
    solver = new Solver();
    vars = 0;
    parse(filename);
    dimension = clauses.size();
    srand (time(NULL));
    rotated_crits = 0;
    flip_edges_computed.resize(dimension, false);
    flip_edges.resize(dimension);
    flip_edges_flatten.resize(dimension);
}

MSHandle::MSHandle(int numVarsInput, const std::vector<std::vector<int> > & clausesInput):SatSolver("nofile") {
    solver = new Solver();
    vars = 0;
    parse(numVarsInput, clausesInput);
    dimension = clauses.size();
    srand (time(NULL));
    rotated_crits = 0;
    flip_edges_computed.resize(dimension, false);
    flip_edges.resize(dimension);
    flip_edges_flatten.resize(dimension);
}

MSHandle::~MSHandle(){
    delete solver;
}

void MSHandle::compute_flip_edges(int c){
    if(flip_edges_computed[c]) return;
    flip_edges_computed[c] = true;

    vector<bool> flatten(dimension, false);
    for(int l = 0; l < clauses[c].size() - 1; l++){
        auto lit = clauses[c][l];
        vector<int> edges;
        if(lit > 0){
            for(auto &h: hitmap_neg[lit - 1]){
                if(h != c){
                    edges.push_back(h);
                    flatten[h] = true;
                }
            }
        }
        else{
            for(auto &h: hitmap_pos[(-1 * lit) - 1]){
                if(h != c){
                    edges.push_back(h);            
                    flatten[h] = true;
                }
            }
        }
        flip_edges[c].push_back(edges);
    }
    for(int i = 0; i < dimension; i++)
        if(flatten[i])
            flip_edges_flatten[c].push_back(i);
}

bool MSHandle::add_clause(vector<int> cl){
    std::sort(cl.begin(), cl.end());
    vector<int> copy = cl; 
    copy.pop_back(); //get rid of control variable
    clauses.push_back(cl);
    clauses_map[copy] = clauses.size() - 1; //used for manipulation with single MUS extractions (muser2, dmuser)
    vec<Lit> msClause;
    for(auto &lit: cl)
        msClause.push(itoLit(lit));        

    for(auto &lit: copy){
        if(lit > 0)
            hitmap_pos[lit - 1].push_back(clauses.size() - 1);
        else
            hitmap_neg[(-1 * lit) - 1].push_back(clauses.size() - 1);
    }
    return solver->addClause(msClause);
}

bool MSHandle::add_unit(int lit){
    return solver->addClause(itoLit(lit));
}

bool MSHandle::parse(string path)
{
    ifstream infile(path, ifstream::in);
    if (!infile.is_open())
        print_err("wrong input file");

    string line;
    vector<int> clause;
    string pom;
    while (getline(infile, line))
    {
        if (line[0] == 'p')
        {
            istringstream is(line);
            is >> pom;    // p
            is >> pom;    // cnf
            is >> vars; //number of variables
        }
        else if (line[0] == 'c')
            continue;
        else if (clauses_unique_map.find(line) != clauses_unique_map.end())
        {
            cout << "a duplicate clause found in the input formula" << endl;
            continue;
        }
        else
        {
            clauses_str.push_back(line);
            clauses_unique_map[line] = clauses_str.size() - 1;
        }
    }
    hitmap_pos.resize(vars);
    hitmap_neg.resize(vars);
    for (int i = 0; i < vars; i++)
    {
        solver->newVar(); // clause variables
    }
    for (int i = 0; i < clauses_str.size(); i++)
        solver->newVar(lbool(uint8_t(1)), true); // control variables
    for (size_t i = 0; i < clauses_str.size(); i++)
    {
        clause = convert_clause(clauses_str[i]);
        clause.push_back(vars + i + 1); //control variable
        add_clause(clause);                //add clause to the solver
    }
    return true;
}

bool MSHandle::parse(int numVarsInput, const std::vector<std::vector<int> > & clausesInput)
{
    vars = numVarsInput;
    clauses_str.reserve(clausesInput.size());
    for (const auto & clauseInput: clausesInput)
    {
        std::stringstream ss;
        for (const auto var: clauseInput)
            ss << var << " ";
        ss << 0;
        std::string line = ss.str();
        clauses_str.push_back(line);
        if (clauses_unique_map.find(line) != clauses_unique_map.end())
        {
            cout << "a duplicate clause found in the input formula" << endl;
            continue;
        }
        clauses_unique_map[line] = clauses_str.size() - 1;
    }
    hitmap_pos.resize(vars);
    hitmap_neg.resize(vars);
    for (int i = 0; i < vars; ++i)
        solver->newVar(); // clause variables
    for (int i = 0; i < clauses_str.size(); i++)
        solver->newVar(lbool(uint8_t(1)), true); // control variables
    for (size_t i = 0; i < clausesInput.size(); ++i)
    {
        std::vector<int> clause = clausesInput[i];
        clause.push_back(vars + i + 1); //control variable
        add_clause(clause);                //add clause to the solver
    }
    return true;
}


vector<bool> MSHandle::model_extension(vector<bool> subset, vector<bool> model){
    int flipped = 0;
    vector<bool> extension = subset;
    for(int i = 0; i < extension.size(); i++){
        if(!extension[i]){
            for(int j = 0; j < clauses[i].size() - 1; j++){
                int lit = clauses[i][j];
                if(lit > 0 && model[lit - 1]){
                    extension[i] = true;
                    flipped++;
                    break;
                }
                else if(lit < 0 && !model[(-1 * lit) - 1]){
                    extension[i] = true;
                    flipped++;
                    break;
                }                
            }
        }
    }
    return extension;
}

void MSHandle::criticals_rotation(vector<bool>& criticals, vector<bool> subset){
    vector<int> criticals_int;
    for(int i = 0; i < dimension; i++)
        if(criticals[i] && subset[i]) criticals_int.push_back(i);

    for(int i = 0; i < criticals_int.size(); i++){
        int c = criticals_int[i];
        compute_flip_edges(c); //TODO: better encansulape
        for(auto &lit_group: flip_edges[c]){
            int count = 0;            
            int flip_c;    
            for(auto c2: lit_group){
                if(subset[c2]){ count++; flip_c = c2; }
            }
            if(count == 1 && !criticals[flip_c]){
                criticals_int.push_back(flip_c);
                criticals[flip_c] = true;
            }
        }
    }
}


// check formula for satisfiability using miniSAT
// the core and grow variables controls whether to return an unsat core or model extension, respectively
bool MSHandle::solve(vector<bool>& controls, bool unsat_improve, bool sat_improve){
    checks++;
    vec<Lit> lits;
    for(unsigned int i = 0; i < controls.size(); i++){
        if(controls[i])
            lits.push(itoLit((i + vars + 1) * (-1)));
        else
            lits.push(itoLit(i + vars + 1 ));
    }
    bool sat = solver->solve(lits);
    if(sat && sat_improve){ // extract model extension        
        for(int f = 0; f < controls.size(); f++){
            if(!controls[f]){
                for(int l = 0; l < clauses[f].size() - 1; l++){
                    if(clauses[f][l] > 0){
                        if(solver->model[clauses[f][l] - 1] == l_True){
                            controls[f] = true;
                            break;
                        }
                    }
                    else{
                        if(solver->model[-1 * clauses[f][l] - 1] == l_False){
                            controls[f] = true;
                            break;
                        }
                    }
                }
            }
        }
    }            
    else if(!sat && unsat_improve){ // extract unsat core
        vector<bool> core = vector<bool> (dimension, false);        
            for (int i = 0 ; i < solver->conflict.size() ; i++) 
            core[var(solver->conflict[i]) - vars] = true;
        controls = core;        

    }                
    return sat;
}

vector<bool> MSHandle::get_model(){
    vector<bool> model(vars, false);
    for(int i = 0; i < vars; i++){
        if(solver->model[i] == l_True)
            model[i] = true;
        else if(solver->model[i] == l_False)
            model[i] = false;
        else
            cout << "literal not evaluated " << i << endl;
    }
    return model;
}

string MSHandle::toString(vector<bool> &f){
    int formulas = std::count(f.begin(), f.end(), true);
    stringstream result;
    result << "p cnf " << vars << " " << formulas << "\n";
    for(int i = 0; i < f.size(); i++)
        if(f[i]){
            result << clauses_str[i] << "\n";
        }
    return result.str();
}

void MSHandle::processMuc(std::vector<bool> & mus)
{
    if (mucCallback)
    {
        std::vector<std::vector<int> > copy;
        for (int i = 0; i < mus.size() && i < clauses.size(); ++i)
        {
            if (mus[i])
            {
                copy.push_back(clauses[i]);
                copy.back().pop_back(); // remove the control variable
            }
        }
        mucCallback->processMuc(copy);
    }
}

void MSHandle::export_formula_crits(vector<bool> f, string filename, vector<bool> crits){
    int formulas = std::count(f.begin(), f.end(), true);


    FILE *file;
    file = fopen(filename.c_str(), "w");
    if(file == NULL) cout << "failed to open " << filename << ", err: " << strerror(errno) << endl;
    
    fprintf(file, "p gcnf %d %d %d\n", vars, formulas, formulas);
    for(int i = 0; i < f.size(); i++)
        if(f[i] && crits[i]){
            fprintf(file, "{0} %s\n", clauses_str[i].c_str());
        }
    int group = 1;
    for(int i = 0; i < f.size(); i++)
        if(f[i] && !crits[i]){
            fprintf(file, "{%d} %s\n", group++, clauses_str[i].c_str());
        }
    if (fclose(file) == EOF) {
        cout << "error closing file: " << strerror(errno) << endl;
    }    
}

vector<bool> MSHandle::import_formula_crits(string filename){
    vector<bool> f(dimension, false);
    vector<vector<int>> cls;
    ReMUS::parse_DIMACS(filename, cls);
    for(auto cl: cls){
        sort(cl.begin(), cl.end());
        if(clauses_map.count(cl))
            f[clauses_map[cl]] = true;
        else { assert(false); }        
    }    
    return f;
}

// implementation of the shrinking procedure
// based on the value of basic_shrink employes either muser2 or dmuser
vector<bool> MSHandle::shrink(std::vector<bool> &f, std::vector<bool> crits){
    shrinks++;
    if(shrink_alg == "custom"){
        return SatSolver::shrink(f, crits); //shrink with unsat cores
    }
#ifdef UMCSMUS
    if(shrink_alg == "default"){
        return shrink_mcsmus(f, crits);
    }
#endif
    stringstream exp;            
    exp << "./tmp/f_" << hash << ".cnf";            
    export_formula_crits(f, exp.str(), crits);    

    return shrink_muser(exp.str(), hash);
}

int muser_output(std::string filename){
    ifstream file(filename, ifstream::in);
    std::string line;
    while (getline(file, line))
    {
        if (line.find("c Calls") != std::string::npos){
            std::string calls = line.substr(line.find(":") + 2, line.size()); // token is "scott"
            return atoi(calls.c_str());
        }
    }    
    return 0;
}

vector<bool> MSHandle::shrink_muser(string input, int hash2){
    stringstream cmd, muser_out, imp;
    muser_out << "./tmp/f_" << hash << "_output";
    imp << "./tmp/f_" << hash << "_mus";
    cmd << "./muser2-para -grp -wf " << imp.str() << " " << input << " > " << muser_out.str();// */ " > /dev/null";
    int status = system(cmd.str().c_str());
    if(status < 0){
        std::cout << "Invalid muser return code" << std::endl; exit(0);
    }
    imp << ".gcnf";
    vector<bool> mus = import_formula_crits(imp.str());
    int sat_calls = muser_output(muser_out.str());    
//    checks += sat_calls;
    remove(imp.str().c_str());
    remove(muser_out.str().c_str());
    remove(input.c_str());
    return mus;
}

