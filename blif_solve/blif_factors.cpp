#include "blif_factors.h"

#include "log.h"

#include <ntr.h>
#include <cuddInt.h>


// local function declarations
namespace {


  // ***** Function *****
  // inits the ntr options to some sensible default
  NtrOptions * mainInit();



  // ***** Function *****
  // computes the union and re-assigns to back to cube
  // i.e.:
  //    cube = cube \union var
  // also de-refs the previous value of the cube and the var
  void reassignToUnion(DdManager * ddm, bdd_ptr & cube, bdd_ptr var);




} // end anonymous namespace





// definitions for BlifFactors functions
namespace blif_solve {




  // static member variables
  std::string BlifFactors::primary_input_prefix = "pi";
  std::string BlifFactors::primary_output_prefix = "po";
  std::string BlifFactors::latch_input_prefix = "li";
  std::string BlifFactors::latch_output_prefix = "lo";





  // constructor
  // parses the network file, WITHOUT creating the actual bdds
  BlifFactors::BlifFactors(std::string const & fileName, DdManager * const ddm)
  {
    FILE * const fp = fopen(fileName.c_str(), "r");
    m_network.reset(Bnet_ReadNetwork(fp, 0));
    fclose(fp);
    m_ddm = ddm;
  }




  // destructor
  BlifFactors::~BlifFactors()
  {
    for (BnetNode_ptr node = m_network->nodes; node != NULL; node = node->next)
    {
      if (node->dd != NULL &&
          node->type != BNET_INPUT_NODE &&
          node->type != BNET_PRESENT_STATE_NODE) {
        Cudd_IterDerefBdd(m_ddm, node->dd);
        node->dd= NULL;
      }
    }
    // free network
    Bnet_FreeNetwork(m_network.get());

  }




  // createBdds
  // creates the bdds
  // extracts the factors (li <-> li_circuit)
  // stores the pi and non-pi (li & lo) variable cubes
  void BlifFactors::createBdds()
  {

    // build bdds in the blif file
    std::unique_ptr<NtrOptions> options(mainInit());
    if (m_network == NULL)
      throw std::logic_error("Unexpected error parsing blif file");
    Ntr_buildDDs(m_network.get(), m_ddm, options.get(), NULL);


    m_piVars = bdd_one(m_ddm);
    m_nonPiVars = bdd_one(m_ddm);
    m_factors.reset(new std::vector<bdd_ptr>());


    // loop over all variables
    for (BnetNode_cptr node = m_network->nodes; node != NULL; node = node->next)
    {
      if (NULL == node->dd)
        continue;
      std::string name = node->name;


      if (name.find(primary_input_prefix) == 0)
      {
        // if primary input variable
        // store in piVars
        reassignToUnion(m_ddm, m_piVars, bdd_new_var_with_index(m_ddm, node->var));
        
      }


      else if (name.find(latch_output_prefix) == 0)
      {
        // if lo variable
        // store in nonPiVars
        reassignToUnion(m_ddm, m_nonPiVars, bdd_new_var_with_index(m_ddm, node->var));
      }


      else if (name.find(latch_input_prefix) == 0)
      {
        // if li variable
        // store in nonPiVars
        // and create factor
        bdd_ptr li = bdd_new_var_with_index(m_ddm, -1);
        bdd_ptr li_circuit = node->dd;
        bdd_ptr li_and_lic = Cudd_bddAnd(m_ddm, li, li_circuit);
        bdd_ptr nli_and_nlic = Cudd_bddAnd(m_ddm, Cudd_Not(li), Cudd_Not(li));
        bdd_ptr factor = bdd_or(m_ddm, li_and_lic, nli_and_nlic);
        m_factors->push_back(factor);
        reassignToUnion(m_ddm, m_nonPiVars, li);
      }



    }
  }




  // accessors
  BlifFactors::FactorVec BlifFactors::getFactors() const
  {
    return m_factors;
  }
  bdd_ptr BlifFactors::getPiVars() const
  {
    return m_piVars;
  }
  bdd_ptr BlifFactors::getNonPiVars() const
  {
    return m_nonPiVars;
  }

} // end namespace blif_solve




namespace {

  // ***** Function *****
  // computes the union and re-assigns to back to cube
  // i.e.:
  //    cube = cube \union var
  // also de-refs the previous value of the cube and the var
  void reassignToUnion(DdManager * ddm, bdd_ptr & cube, bdd_ptr var)
  {
    bdd_ptr temp = bdd_cube_union(ddm, cube, var);
    bdd_free(ddm, cube);
    bdd_free(ddm, var);
    cube = temp;
  }






  // *** Function ***
  //
  //  @brief Allocates the option structure and initializes it.
  //
  //  @sideeffect none
  //
  //  @see ntrReadOptions
  //
  // ****************
  NtrOptions * mainInit()
  {
    NtrOptions	*option;

    // Initialize option structure. 
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

    return(option);
  }


} // end anonymous namespace

