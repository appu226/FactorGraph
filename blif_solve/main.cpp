
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
#include "blif_factors.h"


// constants
std::string const primary_input_prefix = "pi";
std::string const primary_output_prefix = "po";
std::string const latch_input_prefix = "li";
std::string const latch_output_prefix = "lo";
std::string const quantification_answer_prefix = "ans_qpi";



// function declarations
int             main               (int argc, char ** argv);
NtrOptions*     mainInit           ();
bdd_ptr         apply_cudd         (blif_solve::BlifFactors const & blifFactors);
bdd_ptr         apply_factor_graph (blif_solve::BlifFactors const & blifFactors);
void            clean_up_network   (BnetNetwork_ptr const network, DdManager * const ddm);
bool            startsWith         (std::string const & str, std::string const & prefix);



// ****** Function *******
// duration
// takes a start and end chrono time
// and returns the duration in seconds as a double
// ***********************
template<typename T> 
double duration(T const & start)
{
  return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
}

inline auto now()
{
  return std::chrono::high_resolution_clock::now();
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
  auto clo = std::make_shared<blif_solve::CommandLineOptions>(argc, argv);
  blif_solve::setVerbosity(clo->verbosity);

  try {
  
    
    
    // init cudd
    auto srt = std::make_shared<SRT>();
    
   

    // parse network
    auto start = now();
    auto blifFactors = std::make_shared<blif_solve::BlifFactors>(clo->blif_file_path, srt->ddm);
    blif_solve_log(INFO, "parsed blif file in " << duration(start) << " sec");


    
    // create bdds
    if (clo->mustApplyCudd || clo->mustApplyFactorGraph)
    {
      start = now();
      blifFactors->createBdds();
      blif_solve_log(INFO, "create factor bdds in " << duration(start) << " sec");
    }



    // apply cudd based quantification
    bdd_ptr cuddResult = NULL;
    if (clo->mustApplyCudd)
      cuddResult = apply_cudd(*blifFactors);


    
    // apply factor graph based quantification
    bdd_ptr factorGraphResult = NULL;
    if (clo->mustApplyFactorGraph)
      factorGraphResult = apply_factor_graph(*blifFactors);

    
    /*

    // confirm that the cudd result implies the factor graph result
    if (clo->mustApplyCudd && clo->mustApplyFactorGraph)
    {
      // (a implies b) = (!a or b)
      bdd_ptr notCuddResult = bdd_not(srt->ddm, cuddResult);
      bdd_ptr cuddImpliesFactorGraph = bdd_or(srt->ddm, notCuddResult, factorGraphResult);
      bdd_free(srt->ddm, notCuddResult);
      int implicationIsTrue = bdd_is_one(srt->ddm, cuddImpliesFactorGraph);
      bdd_free(srt->ddm, cuddImpliesFactorGraph);
      blif_solve_log(INFO, "The cudd result does "
                           << (implicationIsTrue ? "indeed" : "NOT")
                           << " imply the factor graph result."
                           << std::endl);
      if (!implicationIsTrue)
        blif_solve_log(ERROR, "The cudd result does NOT imply the factor graph result");
    }

    */
    
    // clean-up
    if (cuddResult != NULL)        bdd_free(srt->ddm, cuddResult);
    if (factorGraphResult != NULL) bdd_free(srt->ddm, factorGraphResult);
  
  } catch (std::exception const & e)
  {
    if (clo->verbosity >= blif_solve::ERROR)
      std::cerr << "Fatal error: " << e.what() << std::endl;
    exit(1);
  }


  blif_solve_log(INFO, "SUCCESS " << clo->blif_file_path.substr(clo->blif_file_path.find_last_of("/") + 1));

  return 0;
}




// ***** Function ***********
// Apply the factor graph algorithm to compute the transition relation
//   - create the factor_graph using the set of nodes in the network
//   - merge all the var nodes that are not pi<nnn> (primary inputs) into a single node R
//   - pass messages, collect the conjunction of messages coming into R
// **************************
bdd_ptr apply_factor_graph(blif_solve::BlifFactors const & blifFactors)
{

  // collect from the network
  // the info required to create a factor graph
  auto funcs = blifFactors.getFactors();      // the set of functions
  auto ddm = blifFactors.getDdManager();

  // create factor graph
  auto start = now();
  factor_graph * fg = factor_graph_new(ddm, &(funcs->front()), funcs->size());
  blif_solve_log(INFO, "Created factor graph with "
                       << funcs->size() << " functions in "
                       << duration(start) << " secs");



  // group the non-pi variables in the factor graph
  factor_graph_group_vars(fg, blifFactors.getNonPiVars());
  start = now();
  blif_solve_log(INFO, "Grouped non-pi variables in "
                       << duration(start) << " secs");



  // pass messages till convergence
  start = now();
  factor_graph_converge(fg);
  blif_solve_log(INFO, "Factor graph messages have converged in "
                       << duration(start) << " secs");



  // compute the result by conjoining all incoming messages
  start = now();
  fgnode * V = factor_graph_get_varnode(fg, blifFactors.getNonPiVars());
  int num_messages;
  bdd_ptr *messages = factor_graph_incoming_messages(fg, V, &num_messages);
  bdd_ptr result = bdd_one(ddm);
  for (int mi = 0; mi < num_messages; ++mi)
  {
    bdd_ptr temp = bdd_and(ddm, result, messages[mi]);
    bdd_free(ddm, result);
    bdd_free(ddm, messages[mi]);
    result = temp;
  }
  free(messages);
  blif_solve_log(INFO, "Computed final factor graph result in "
                       << duration(start) << " secs");

  // clean-up and return
  factor_graph_delete(fg);
  return result;
} // end factor graph based quantification





// **** Function ************
// Compute the transition relation using cudd quantification
//  by first computing
//    the conjunction of all the bdds in the network,
//  and then
//    existentially quantifying out the primary input (pi_<nnn>) variables
// **************************
bdd_ptr apply_cudd(blif_solve::BlifFactors const & blif_factors)
{
  auto factors = blif_factors.getFactors();
  auto ddm = blif_factors.getDdManager();
  auto start = now();
  auto conj = bdd_one(ddm);
  auto result = bdd_and_exists_multi(ddm, std::set<DdNode*>(factors->cbegin(), factors->cend()), blif_factors.getPiVars());
  blif_solve_log(INFO, "computed existential quantification in " << duration(start) << " secs");
  if (blif_solve::getVerbosity() >= blif_solve::DEBUG)
  {
    blif_solve_log(DEBUG, "printing result from cudd_apply:");
    bdd_print_minterms(ddm, result);
  }
  bdd_free(ddm, conj);
  return result;
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



// ******* Function *******
// startsWith: determines whether str starts with prefix
bool startsWith(std::string const & str, std::string const & prefix)
{
  return str.find(prefix) == 0;
}
