
// std includes
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// factor_graph includes
#include <dd.h>
#include <ntr.h>
#include <srt.h>
#include <cuddInt.h>


// logging
#define blif_solve_log(level, msg) \
  if (blif_solve_verbosity >= level) { \
    std::cout << "[" << #level << "] " << msg << std::endl; \
  }




// typedefs



// function declarations
int main(int argc, char ** argv);
void usage();
NtrOptions* mainInit();



// globals
enum Verbosity {
  QUIET,
  ERROR,
  WARNING,
  INFO,
  DEBUG
};
Verbosity blif_solve_verbosity = INFO;





// *** Function ****
// main function:
//   parses inputs
//   reads blif file
//   creates bdds
// *****************
int main(int argc, char ** argv)
{
  
  // parse inputs
  if (argc != 2)
  {
    usage();
    exit(1);
  }


  try {

  
    
    
    // init cudd
    std::shared_ptr<SRT> srt = std::make_shared<SRT>();
    
    
    
    
    // read blif file
    FILE * fp = fopen(argv[1], "r");
    if (fp == NULL)
      throw std::invalid_argument(std::string("Could not open file '") + argv[0] + "'");


    
    
    
    // create blif network structure and read bdds
    BnetNetwork_ptr network = Bnet_ReadNetwork(fp, 0);
    if (blif_solve_verbosity >= INFO)
    {
      blif_solve_log(INFO, "Parsed network with " << network->npis << " primary inputs, "
                                                  << network->ninputs << " inputs, "
                                                  << network->npos << " primary outputs, "
                                                  << network->noutputs << " outputs and "
                                                  << network->nlatches << " latches.");
    }
    auto options = std::unique_ptr<NtrOptions>(mainInit());
    if (network == NULL)
      throw std::logic_error("Unexpected error parsing blif file");
    Ntr_buildDDs(network, srt->ddm, options.get(), NULL);




    // compute the conjunction of all functions
    // and collect all primary input variables
    bdd_ptr conj = bdd_one(srt->ddm);
    bdd_ptr pi_vars = bdd_one(srt->ddm);
    for (BnetNode_cptr node = network->nodes; node != NULL; node = node->next)
    {
      bdd_ptr temp;
      if (node->dd != NULL)
      {
        temp = bdd_and(srt->ddm, conj, node->dd);
        bdd_free(srt->ddm, conj);
        conj = temp;
        if (node->name[0] = 'p' && node->name[1] == 'i')
        {
          bdd_ptr var = bdd_new_var_with_index(srt->ddm, node->var);
          temp = bdd_cube_union(srt->ddm, pi_vars, var);
          bdd_free(srt->ddm, pi_vars);
          pi_vars = temp;
        }
      }
    }
    blif_solve_log(INFO, "Created conjunction of all functions");





    // quantify out the primary variables
    bdd_ptr result = bdd_forsome(srt->ddm, conj, pi_vars);
    blif_solve_log(INFO, "Quantified out primary inputs to get transition relation");


    
    
    // clean-up
    // Clean ptrs
    bdd_free(srt->ddm, result);
    bdd_free(srt->ddm, conj);
    bdd_free(srt->ddm, pi_vars);
    // Dispose of node BDDs
    for( BnetNode_ptr node = network->nodes; node != NULL; node = node->next)
    {
      if (node->dd != NULL &&
          node->type != BNET_INPUT_NODE &&
          node->type != BNET_PRESENT_STATE_NODE) {
        Cudd_IterDerefBdd(srt->ddm, node->dd);
        node->dd = NULL;
      }
    }
    // free network
    Bnet_FreeNetwork(network);

  
  
  
  
  } catch (std::exception const & e)
  {
    if (blif_solve_verbosity >= ERROR)
      std::cerr << "Fatal error: " << e.what() << std::endl;
    exit(1);
  }
  return 0;
}






// *** Function ******
// prints the usage information for the executable
// *******************
void usage()
{
  std::cout << "blif_solve : Utility for solving a blif file using various methods\n"
            << "Usage:\n"
            << "\tblif_solve <blif file path>" << std::endl;
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


