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


// std includes
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <fstream>

// blif_solve_lib includes
#include <log.h>

// verilog_to_bdd includes
#include <verilog_to_bdd.h>

// factor_graph includes
#include <srt.h>

// dd includes
#include <bnet.h>
#include <ntr.h>
#include <dd.h>

// cudd includes
#include <cuddInt.h>

// declarations for utility functions
namespace verilog_solve {

  // global variables
  std::string g_latch_input_prefix = "li";
  std::string g_latch_output_prefix = "lo";
  std::string g_primary_input_prefix = "pi";

  // ***** Struct CommandLineOptions *****
  // structure for capturing the command line options for this executable
  struct CommandLineOptions {
    CommandLineOptions(int argc, char ** argv);
    std::string blifFileName;
    std::string verilogFileName;
    static std::string help();
  }; 


  // ***** Struct BlifNetwork *****
  // wrapper around a parsed blif file
  struct BlifNetwork {
    BnetNetwork * network;
    DdManager * manager;
    BlifNetwork(const std::string & blifFileName, DdManager * ddm);
    ~BlifNetwork();

    verilog_to_bdd::BddVarMapPtr createNonPiVarMap() const;
    bdd_ptr getSubstitutedFactors(const verilog_to_bdd::BddVarMapPtr & bddVarMap) const;
    int getNumNonPiVars() const;
  };

  
} // end namespace verilog_solve


// main function
int main(int argc, char ** argv)
{
  // parse command line options and init cudd
  auto clo = std::make_shared<verilog_solve::CommandLineOptions>(argc, argv);
  auto srt = std::make_shared<SRT>();


  // parse blif network
  auto blifNetwork = std::make_shared<verilog_solve::BlifNetwork>(clo->blifFileName, srt->ddm);
  
  // create skolem functions
  auto bddVarMap = blifNetwork->createNonPiVarMap();
  std::ifstream verilogFileStream(clo->verilogFileName);
  verilog_to_bdd::VerilogToBdd::parse(&verilogFileStream,
                                      clo->verilogFileName,
                                      bddVarMap,
                                      srt->ddm);
  
  // substitute skolem functions into factors
  bdd_ptr result = blifNetwork->getSubstitutedFactors(bddVarMap);

  // count results
  size_t numSolutions = bdd_count_minterm(srt->ddm, result, blifNetwork->getNumNonPiVars());
  std::cout << "Number of solutions = " << numSolutions << std::endl;

  std::cout << "SUCCESS" << std::endl;

  return 0; 
} // end main unction



// ----------------------------
// definitions for verilog_solve::BlifNetwork
verilog_solve::BlifNetwork::BlifNetwork(const std::string & fileName, DdManager * ddm)
{
  auto start = blif_solve::now();
  FILE * const fp = fopen(fileName.c_str(), "r");
  if (NULL == fp)
    throw std::invalid_argument("Could not open file '" + fileName + "'");
  network = Bnet_ReadNetwork(fp, 0);
  fclose(fp);
  manager = ddm;
  blif_solve_log(DEBUG, "parsed blif file in " << blif_solve::duration(start) << " sec");
  start = blif_solve::now();

  auto option = std::make_unique<NtrOptions>();
  // Initialize option structure. 
  option->initialTime    = util_cpu_time();
  option->verify         = FALSE;
  option->second         = FALSE;
  option->file1          = NULL;
  option->file2          = NULL;
  option->traverse       = FALSE;
  option->depend         = FALSE;
  option->image          = NTR_IMAGE_MONO;
  option->imageClip      = 1.0;
  option->approx         = NTR_UNDER_APPROX;
  option->threshold      = -1;
  option->from	   = NTR_FROM_NEW;
  option->groupnsps      = NTR_GROUP_NONE;
  option->closure        = FALSE;
  option->closureClip    = 1.0;
  option->envelope       = FALSE;
  option->scc            = FALSE;
  option->maxflow        = FALSE;
  option->shortPath      = NTR_SHORT_NONE;
  option->selectiveTrace = FALSE;
  option->zddtest        = FALSE;
  option->printcover     = FALSE;
  option->sinkfile       = NULL;
  option->partition      = FALSE;
  option->char2vect      = FALSE;
  option->density        = FALSE;
  option->quality        = 1.0;
  option->decomp         = FALSE;
  option->cofest         = FALSE;
  option->clip           = -1.0;
  option->dontcares      = FALSE;
  option->closestCube    = FALSE;
  option->clauses        = FALSE;
  option->noBuild        = FALSE;
  option->stateOnly      = FALSE;
  option->node           = NULL;
  option->locGlob        = BNET_GLOBAL_DD;
  option->progress       = FALSE;
  option->cacheSize      = 32768;
  option->maxMemory      = 0;	// set automatically 
  option->maxMemHard     = 0; // don't set 
  option->maxLive        = ~0U; // very large number 
  option->slots          = CUDD_UNIQUE_SLOTS;
  option->ordering       = PI_PS_FROM_FILE;
  option->orderPiPs      = NULL;
  option->reordering     = CUDD_REORDER_NONE;
  option->autoMethod     = CUDD_REORDER_SIFT;
  option->autoDyn        = 0;
  option->treefile       = NULL;
  option->firstReorder   = DD_FIRST_REORDER;
  option->countDead      = FALSE;
  option->maxGrowth      = 20;
  option->groupcheck     = CUDD_GROUP_CHECK7;
  option->arcviolation   = 10;
  option->symmviolation  = 10;
  option->recomb         = DD_DEFAULT_RECOMB;
  option->nodrop         = TRUE;
  option->signatures     = FALSE;
  option->verb           = 0;
  option->gaOnOff        = 0;
  option->populationSize = 0;	// use default
  option->numberXovers   = 0;	// use default 
  option->bdddump	   = FALSE;
  option->dumpFmt	   = 0;	// dot
  option->dumpfile	   = NULL;
  option->store          = -1; // do not store 
  option->storefile      = NULL;
  option->load           = FALSE;
  option->loadfile       = NULL;
  option->seed           = 1;

  Ntr_buildDDs(network, manager, option.get(), NULL);

}

verilog_solve::BlifNetwork::~BlifNetwork()
{
  if (network)
  {
    for (BnetNode_ptr node = network->nodes; node != NULL; node = node->next)
    {
      if (node->dd != NULL &&
          node->type != BNET_INPUT_NODE &&
          node->type != BNET_PRESENT_STATE_NODE) {
        Cudd_IterDerefBdd(manager, node->dd);
        node->dd= NULL;
      }
    }
    // free network
    Bnet_FreeNetwork(network);
  }
}

verilog_to_bdd::BddVarMapPtr verilog_solve::BlifNetwork::createNonPiVarMap() const
{
  auto result = std::make_shared<verilog_to_bdd::BddVarMap>(manager);
  for (auto node = network->nodes; node != NULL; node = node->next)
  {
    if (NULL == node->dd)
      continue;
    std::string name = node->name;

    // add non-pi variables
    if (name.find(g_latch_output_prefix) == 0 
        || name.find(g_latch_input_prefix) == 0)
    {
      bdd_ptr nonPiVar = bdd_new_var_with_index(manager, node->var);
      result->addBddPtr(name, nonPiVar);
    }

  }
  return result;
}

bdd_ptr verilog_solve::BlifNetwork::getSubstitutedFactors(const verilog_to_bdd::BddVarMapPtr & bddVarMap) const
{
  // vector to store all primary variables, and their skolem functions
  std::vector<std::pair<int, bdd_ptr> > varAssignments;
  // vector to store all the original un-substituted factors
  std::vector<bdd_ptr> factors;


  // collect factors and primary input variables
  for (auto node = network->nodes; node != NULL; node = node->next)
  {
    if (NULL == node->dd)
      continue;
    std::string name = node->name;
    // if primary input, collect it, else count it
    if (name.find(g_primary_input_prefix) == 0)
      varAssignments.push_back(std::make_pair(node->var, bddVarMap->getBddPtr(name)));
    // if latch input, create factor from it and store it
    else if(name.find(g_latch_input_prefix) == 0)
    {
      bdd_ptr li = bdd_new_var_with_index(manager, -1);
      bdd_ptr li_circuit = node->dd;
      bdd_ptr factor = bdd_xnor(manager, li, li_circuit);
      factors.push_back(factor);
      bdd_free(manager, li);
    }
  }

  // create result
  bdd_ptr result = bdd_one(manager);


  // for each factor
  for (auto factor: factors)
  {

    // compute subst factor
    auto substitutedFactor = bdd_dup(factor);
    for (auto va: varAssignments) // for each varAssignment, subst into factor
    {
      auto subst_temp = bdd_compose(manager, substitutedFactor, va.second, va.first);
      bdd_free(manager, substitutedFactor);
      substitutedFactor = subst_temp;
    }

    // conjoin subst factor to result
    auto and_temp = bdd_and(manager, substitutedFactor, result);
    bdd_free(manager, result);
    result = and_temp;
    bdd_free(manager, substitutedFactor);
  }

  // clean up and return result
  for (auto va: varAssignments) 
    bdd_free(manager, va.second);
  for (auto factor: factors)
    bdd_free(manager, factor);
  return result;
}

int verilog_solve::BlifNetwork::getNumNonPiVars() const
{
  int numNonPiVars = 0;
  for (auto node = network->nodes; node != NULL; node = node->next)
  {
    if (NULL == node->dd)
      continue;
    std::string name = node->name;
    // if primary input, collect it, else count it
    if (name.find(g_primary_input_prefix) != 0)
      ++numNonPiVars;
  }
  return numNonPiVars;
}

// -------------------------------------------------
// definitions for verilog_solve::CommandLineOptions
verilog_solve::CommandLineOptions::CommandLineOptions(int argc, char ** argv)
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h")
    {
      std::cout << help() << std::endl;
      exit(0);
    }
    else if (arg == "--blif_file_name")
    {
      ++i;
      if (i < argc)
        blifFileName = argv[i];
      else
        throw std::runtime_error("Please provide blif file name after"
                                 " command line option --blif_file_name");
    } 
    else if (arg == "--verilog_file_name")
    {
      ++i;
      if (i < argc)
        verilogFileName = argv[i];
      else
        throw std::runtime_error("Please provide verilog file name after"
                                 " command line option --verilog_file_name");
    } 
    else if (arg == "--verbosity")
    {
      ++i;
      if (i >= argc)
        throw std::runtime_error("Please provide verbosity after"
                                 " command line option --verbosity");
      std::string verbosity = argv[i];
      using namespace blif_solve;
      if (verbosity == "QUIET") setVerbosity(QUIET);
      else if (verbosity == "ERROR") setVerbosity(ERROR);
      else if (verbosity == "WARNING") setVerbosity(WARNING);
      else if (verbosity == "INFO") setVerbosity(INFO);
      else if (verbosity == "DEBUG") setVerbosity(DEBUG);
      else throw std::runtime_error("verbosity must be one of { QUIET, ERROR, WARNING, INFO, DEBUG }.");
    }
  }
  if (blifFileName.empty() || verilogFileName.empty())
  {
    std::cout << help() << std::endl;
    throw std::runtime_error("Mandatory arguments missing.");
  }
}


std::string verilog_solve::CommandLineOptions::help()
{
  return "verilog_solve: Program for using skolem functions in verilog format \n"
         "               for solving existential quantification problems in blif format!\n"
         "               (It's a long story...)\n"
         "Command line arguments:\n"
         "   --help | -h : print this help and exit.\n"
         "   --blif_file_name <path> : path to the blif file, from which the primary input variables\n"
         "                             (eg pi123) are to be quantified out.\n"
         "   --verilog_file_name <path> : path to the verilog file, which contains skolem functions\n"
         "                                for the primary input variables, which can be substituted\n"
         "                                into the blif factors, for existential quantification.\n"
         "   --verbosity <verbosity> : one of { QUIET, ERROR, WARNING, INFO, DEBUG }.";
}
