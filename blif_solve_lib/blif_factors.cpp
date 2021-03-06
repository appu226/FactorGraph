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

#include "blif_factors.h"
#include "log.h"

#include <dd/bdd_partition.h>

#include <dd/ntr.h>
#include <cuddInt.h>

#include <set>

// local function definitions
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
  std::unique_ptr<NtrOptions> mainInit()
  {
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

    return(option);
  }


} // end anonymous namespace



// definitions for BlifFactors functions
namespace blif_solve {





  // constructor
  // parses the network file, WITHOUT creating the actual bdds
  BlifFactors::BlifFactors(std::string const & fileName, int numLoVarsToQuantify, DdManager * const ddm)
  {
    FILE * const fp = fopen(fileName.c_str(), "r");
    if (NULL == fp)
      throw std::invalid_argument("Could not open file '" + fileName + "'");
    m_network = Bnet_ReadNetwork(fp, 0);
    fclose(fp);
    m_ddm = ddm;
    m_numLoVarsToQuantify = numLoVarsToQuantify;
  }


  // private constructor
  BlifFactors::BlifFactors(BnetNetwork * network,
                           DdManager * ddm,
                           FactorVec factors,
                           bdd_ptr piVars,
                           FactorVec nonPiVars) :
    m_network(network),
    m_ddm(ddm),
    m_factors(factors),
    m_piVars(piVars),
    m_nonPiVars(nonPiVars)
  { }




  // destructor
  BlifFactors::~BlifFactors()
  {
    if (m_network)
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
      Bnet_FreeNetwork(m_network);
    }

    bdd_free(m_ddm, m_piVars);
    for (auto factor: *m_factors)
      bdd_free(m_ddm, factor);
    for (auto nonPiVar: *m_nonPiVars)
      bdd_free(m_ddm, nonPiVar);

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
    Ntr_buildDDs(m_network, m_ddm, options.get(), NULL);


    m_piVars = bdd_one(m_ddm);
    m_nonPiVars = std::make_shared<std::vector<bdd_ptr> >();
    m_factors.reset(new std::vector<bdd_ptr>());

    std::set<std::string> primaryInputs;
    std::set<std::string> latchInputs;
    std::set<std::string> latchOutputs;

    for (int ipi = 0; ipi < m_network->npis; ++ipi)
      primaryInputs.insert(m_network->inputs[ipi]);

    for (int ilatch = 0; ilatch < m_network->nlatches; ++ilatch)
    {
      latchInputs.insert(m_network->latches[ilatch][0]);
      latchOutputs.insert(m_network->latches[ilatch][1]);
    }


    // loop over all variables
    for (BnetNode_cptr node = m_network->nodes; node != NULL; node = node->next)
    {
      std::string nodeName = node->name;

      // if primary input
      if (primaryInputs.count(nodeName) > 0)
      {
        // count as pi var
        bdd_ptr temp = bdd_cube_union(m_ddm, m_piVars, node->dd);
        bdd_free(m_ddm, m_piVars);
        m_piVars = temp;
        blif_solve_log_bdd(DEBUG, "parsing var " << nodeName << " as:", m_ddm, node->dd);
      }

      // if latch input
      else if (latchInputs.count(nodeName) > 0)
      {
        // take the latch input circuit C
        // and the previously created latch input variable L
        // and add (L nxor C) as a factor
        auto L = bdd_new_var_with_index(m_ddm, -1);
        auto C = node->dd;
        m_factors->push_back(bdd_xnor(m_ddm, L, C));
        m_nonPiVars->push_back(L);
        blif_solve_log_bdd(DEBUG, "creating var " << nodeName << " as:", m_ddm, L);
        blif_solve_log_bdd(DEBUG, "parsing circuit for " << nodeName << " as:", m_ddm, C);
      }

      // if latch output
      else if (latchOutputs.count(nodeName) > 0)
      {
        // count as non pi var
        m_nonPiVars->push_back(bdd_dup(node->dd));
        blif_solve_log_bdd(DEBUG, "parsing var " << nodeName << " as:", m_ddm, node->dd);
      }
    } // end loop over all network nodes
  } //end BlifFactors::createBdds



  BlifFactors::PtrVec BlifFactors::partitionFactors() const
  {
    std::vector<std::vector<bdd_ptr>> partitions = bddPartition(m_ddm, *m_factors);
    BlifFactors::PtrVec result;
    for (auto partition: partitions)
    {
      auto partitionFactors = std::make_shared<std::vector<bdd_ptr> >(partition);
      bdd_ptr partitionSupport = bdd_one(m_ddm);
      for (auto factor: partition)
      {
        bdd_ptr factorSupport = bdd_support(m_ddm, factor);
        bdd_ptr unionSupport = bdd_cube_union(m_ddm, partitionSupport, factorSupport);
        bdd_free(m_ddm, factorSupport);
        bdd_free(m_ddm, partitionSupport);
        partitionSupport = unionSupport;
      }

      bdd_ptr partitionPiVars = bdd_cube_intersection(m_ddm, partitionSupport, m_piVars);

      auto partitionNonPiVars = std::make_shared<std::vector<bdd_ptr> >();
      bdd_ptr one = bdd_one(m_ddm);
      for (auto nonPiVar: *m_nonPiVars)
      {
        bdd_ptr partitionNonPiVar = bdd_cube_intersection(m_ddm, partitionSupport, nonPiVar);
        if (partitionNonPiVar == one)
          bdd_free(m_ddm, partitionNonPiVar);
        else
          partitionNonPiVars->push_back(partitionNonPiVar);
      }

      
      result.push_back(std::shared_ptr<BlifFactors>(new BlifFactors(NULL, m_ddm, partitionFactors, partitionPiVars, partitionNonPiVars)));
      blif_solve_log(DEBUG, "Added " << partitionFactors->size() << " factors to partition number " << result.size());
      for (auto factor: *partitionFactors)
        blif_solve_log_bdd(DEBUG, "Printing factor: ", m_ddm, factor);
      bdd_free(m_ddm, partitionSupport);
      bdd_free(m_ddm, one);
    }
    return result;
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
  BlifFactors::FactorVec BlifFactors::getNonPiVars() const
  {
    return m_nonPiVars;
  }
  DdManager * BlifFactors::getDdManager() const
  {
    return m_ddm;
  }

} // end namespace blif_solve


