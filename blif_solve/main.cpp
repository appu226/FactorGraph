
// std includes
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <chrono>

// factor_graph includes
#include <dd.h>
#include <ntr.h>
#include <srt.h>
#include <cuddInt.h>

// blif_solve includes
#include "command_line_options.h"



// logging
std::shared_ptr<blif_solve::CommandLineOptions> clo;
#define blif_solve_log(level, msg) \
  if (clo->verbosity >= blif_solve::level) { \
    std::cout << "[" << #level << "] " << msg << std::endl; \
  }



// function declarations
int main(int argc, char ** argv);
NtrOptions* mainInit();
BnetNetwork_ptr parse_network(FILE * const fp, blif_solve::CommandLineOptions const & clo);
void create_bdds(BnetNetwork_ptr const network, DdManager * const ddm );
void apply_cudd(BnetNetwork_ptr const network, DdManager * const ddm);
void clean_up_network(BnetNetwork_ptr const network, DdManager * const ddm);



// ****** Function *******
// duration
// takes a start and end chrono time
// and returns the duration in seconds as a double
// ***********************
template<typename T> 
double duration(T const & start, T const & end)
{
  return std::chrono::duration<double>(end - start).count();
}




// *** Function ****
// main function:
//   parses inputs
//   reads blif file
//   creates bdds if either cudd or factor_graph needs to be applied
//   applies cudd to compute transition relation if required
//   applies factor_graph to compute transition relation if required
//   cleans up memory before exiting
//   logs useful information
// *****************
int main(int argc, char ** argv)
{
  
  // parse inputs
  clo = std::make_shared<blif_solve::CommandLineOptions>(argc, argv);

  try {

  
    
    
    // init cudd
    std::shared_ptr<SRT> srt = std::make_shared<SRT>();
    
    
    // read blif file
    FILE * fp = fopen(clo->blif_file_path.c_str(), "r");
    if (fp == NULL)
      throw std::invalid_argument(std::string("Could not open file '") + clo->blif_file_path + "'");


    
    // create blif network structure
    BnetNetwork_ptr network = parse_network(fp, *clo); 



    // create bdds
    if (clo->mustApplyCudd || clo->mustApplyFactorGraph)
      create_bdds(network, srt->ddm);



    // apply cudd based quantification
    if (clo->mustApplyCudd)
      apply_cudd(network, srt->ddm);

    
    
    // clean-up
    clean_up_network(network, srt->ddm); 
    
  
  } catch (std::exception const & e)
  {
    if (clo->verbosity >= blif_solve::ERROR)
      std::cerr << "Fatal error: " << e.what() << std::endl;
    exit(1);
  }
  return 0;
}





// *** Function ****************
// actually creates the bdds in a blif files.
// also logs the time taken.
// *****************************
void create_bdds(BnetNetwork_ptr const network, DdManager * const ddm)
{
  std::unique_ptr<NtrOptions> options(mainInit());
  if (network == NULL)
    throw std::logic_error("Unexpected error parsing blif file");
  auto start = std::chrono::system_clock::now();
  Ntr_buildDDs(network, ddm, options.get(), NULL);
  auto end = std::chrono::system_clock::now();
  blif_solve_log(INFO, "Created BDDs in the network in " << duration(start, end) << " sec");
}






// *** Function ****************
// parse_network
// Parses a blif file and creates a blif network.
// Note that the blif network is just a parsed data structure
//   that faithfully represents the file.
// In particular, no bdd's are created automatically.
// Also logs some info about the network.
// Returns the parsed BnetNetwork_ptr.
// *****************************
BnetNetwork_ptr parse_network(FILE * const fp, blif_solve::CommandLineOptions const & options)
{
  auto start = std::chrono::system_clock::now();
  BnetNetwork_ptr network = Bnet_ReadNetwork(fp, 0);
  auto end = std::chrono::system_clock::now();
  // print statistics about number of inputs
  int num_pi = 0, num_po = 0, num_li = 0, num_lo = 0;
  for (BnetNode_cptr node = network->nodes; node != NULL; node = node->next)
  {
    std::string name = node->name;
    if (name.find("pi") == 0)
      ++num_pi;
    else if (name.find("po") == 0)
      ++num_po;
    else if (name.find("li") == 0)
      ++num_li;
    else if (name.find("lo") == 0)
      ++num_lo;
  }
  blif_solve_log(INFO, "Parsed " << options.blif_file_path.substr(clo->blif_file_path.find_last_of('/') + 1) 
      << " with "
      << num_pi << " pi, " 
      << num_po << " po, "
      << num_li << " li, "
      << num_lo << " lo variables in "
      << duration(start, end) << " sec");
  return network;
}



//**** Function ************
// Compute the transition relation using cudd quantification
//  by first computing
//    the conjunction of all the bdds in the network,
//  and then
//    existentially quantifying out the primary input (pi_<nnn>) variables
//**************************
void apply_cudd(BnetNetwork_ptr const network, DdManager * const ddm)
{


  // compute the conjunction of all functions
  // and collect all primary input variables
  bdd_ptr conj = bdd_one(ddm);
  bdd_ptr pi_vars = bdd_one(ddm);
  auto conj_start = std::chrono::system_clock::now();
  for (BnetNode_cptr node = network->nodes; node != NULL; node = node->next)
  {
    bdd_ptr temp;
    if (node->dd != NULL)
    {
      temp = bdd_and(ddm, conj, node->dd);
      bdd_free(ddm, conj);
      conj = temp;
      if (node->name[0] = 'p' && node->name[1] == 'i')
      {
        bdd_ptr var = bdd_new_var_with_index(ddm, node->var);
        temp = bdd_cube_union(ddm, pi_vars, var);
        bdd_free(ddm, pi_vars);
        pi_vars = temp;
      }
    }
  }
  auto conj_end = std::chrono::system_clock::now();
  blif_solve_log(INFO, "Created conjunction of all functions in " 
      << duration(conj_start, conj_end)
      << " sec");





  // quantify out the primary variables
  auto quant_start = std::chrono::system_clock::now();
  bdd_ptr result = bdd_forsome(ddm, conj, pi_vars);
  auto quant_end = std::chrono::system_clock::now();
  blif_solve_log(INFO, "Quantified out primary inputs to get transition relation in "
      << duration(quant_start, quant_end)
      << " sec");




  // clean up cudd specific stuff
  // Clean ptrs
  bdd_free(ddm, result);
  bdd_free(ddm, conj);
  bdd_free(ddm, pi_vars);


} // end cudd based quantification




// ******* Function *******
// Clear the memory captured by a network
// ************************
void clean_up_network(BnetNetwork_ptr const network, DdManager * const ddm)
{
  for( BnetNode_ptr node = network->nodes; node != NULL; node = node->next)
  {
    if (node->dd != NULL &&
        node->type != BNET_INPUT_NODE &&
        node->type != BNET_PRESENT_STATE_NODE) {
      Cudd_IterDerefBdd(ddm, node->dd);
      node->dd = NULL;
    }
  }
  // free network
  Bnet_FreeNetwork(network);

}




//*** Function ***
/**
  @brief Allocates the option structure and initializes it.

  @sideeffect none

  @see ntrReadOptions

*/
NtrOptions *
mainInit(
   )
{
    NtrOptions	*option;

    /* Initialize option structure. */
    option = ALLOC(NtrOptions,1);
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
    option->maxMemory      = 0;	/* set automatically */
    option->maxMemHard     = 0; /* don't set */
    option->maxLive        = ~0U; /* very large number */
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
    option->populationSize = 0;	/* use default */
    option->numberXovers   = 0;	/* use default */
    option->bdddump	   = FALSE;
    option->dumpFmt	   = 0;	/* dot */
    option->dumpfile	   = NULL;
    option->store          = -1; /* do not store */
    option->storefile      = NULL;
    option->load           = FALSE;
    option->loadfile       = NULL;
    option->seed           = 1;

    return(option);

} /* end of mainInit */


