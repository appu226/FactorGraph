
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
bdd_ptr         apply_factor_graph (BnetNetwork_ptr const network, DdManager * const ddm);
void            clean_up_network   (BnetNetwork_ptr const network, DdManager * const ddm);
bool            startsWith         (std::string const & str, std::string const & prefix);



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
  auto clo = std::make_shared<blif_solve::CommandLineOptions>(argc, argv);
  blif_solve::setVerbosity(clo->verbosity);

  try {
  
    
    
    // init cudd
    auto srt = std::make_shared<SRT>();
    
   

    // parse network
    auto blifFactors = std::make_shared<blif_solve::BlifFactors>(clo->blif_file_path, srt->ddm);


    
    // create bdds
    if (clo->mustApplyCudd || clo->mustApplyFactorGraph)
      blifFactors->createBdds();



    // apply cudd based quantification
    bdd_ptr cuddResult = NULL;
    if (clo->mustApplyCudd)
      cuddResult = apply_cudd(*blifFactors);


    /*
    
    // apply factor graph based quantification
    bdd_ptr factorGraphResult = NULL;
    if (clo->mustApplyFactorGraph)
      factorGraphResult = apply_factor_graph(network, srt->ddm);

    

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

    
    // clean-up
    if (cuddResult != NULL)        bdd_free(srt->ddm, cuddResult);
    if (factorGraphResult != NULL) bdd_free(srt->ddm, factorGraphResult);
    clean_up_network(network, srt->ddm);
    */
    
  
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
bdd_ptr apply_factor_graph(BnetNetwork_ptr const network, DdManager * const ddm)
{

  // collect from the network
  // the info required to create a factor graph
  std::vector<bdd_ptr> funcs;         // the set of functions
  bdd_ptr non_pi_vars = bdd_one(ddm); // the set of non_pi vars
  int num_non_pi_vars = 0;
  for (BnetNode_cptr node = network->nodes; node != NULL; node = node->next)
  {
    // collect the function
    if (node->dd != NULL)
      funcs.push_back(bdd_dup(node->dd));

    // collect the non-pi var
    if (std::string(node->name).find(primary_input_prefix) != 0)
    {
      bdd_ptr var = bdd_new_var_with_index(ddm, node->var);
      bdd_ptr temp = bdd_cube_union(ddm, var, non_pi_vars);
      bdd_free(ddm, non_pi_vars);
      non_pi_vars = temp;
      bdd_free(ddm, var);
      ++num_non_pi_vars;
    }
  }

  // create factor graph
  auto start = std::chrono::system_clock::now();
  factor_graph * fg = factor_graph_new(ddm, &funcs[0], funcs.size());
  blif_solve_log(INFO, "Created factor graph with "
                       << funcs.size() << " functions in "
                       << duration(start, std::chrono::system_clock::now()) << " secs");



  // group the non-pi variables in the factor graph
  factor_graph_group_vars(fg, non_pi_vars);
  start = std::chrono::system_clock::now();
  blif_solve_log(INFO, "Grouped " << num_non_pi_vars << " variables in "
                       << duration(start, std::chrono::system_clock::now()) << " secs");



  // pass messages till convergence
  start = std::chrono::system_clock::now();
  factor_graph_converge(fg);
  blif_solve_log(INFO, "Factor graph messages have converged in "
                       << duration(start, std::chrono::system_clock::now()) << " secs");



  // compute the result by conjoining all incoming messages
  start = std::chrono::system_clock::now();
  fgnode * V = factor_graph_get_varnode(fg, non_pi_vars);
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
                       << duration(start, std::chrono::system_clock::now()) << " secs");

  // clean-up and return
  factor_graph_delete(fg);
  bdd_free(ddm, non_pi_vars);
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
  auto conj = bdd_one(ddm);
  for (auto fit = factors->cbegin(); fit != factors->cend(); ++fit)
  {
    auto temp = bdd_and(ddm, *fit, conj);
    bdd_free(ddm, conj);
    conj = temp;
  }
  bdd_ptr result = bdd_forsome(ddm, conj, blif_factors.getPiVars());
  if (blif_solve::getVerbosity() >= blif_solve::DEBUG)
  {
    blif_solve_log(DEBUG, "printing conj from cudd_apply:");
    bdd_print_minterms(ddm, conj);
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
