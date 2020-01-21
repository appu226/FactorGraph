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

// blif_solve includes
#include "blif_solve_method.h"

// blif_solve_lib includes
#include <approx_merge.h>

// FactorGraph includes
#include <dd.h>
#include <factor_graph.h>
#include <cnf_dump.h>

// std includes
#include <memory>
#include <queue>
#include <map>
#include <random>
#include <algorithm>
#include <sstream>

namespace {

  using namespace blif_solve;

  // ***** Class *****
  // ClippingAndAbstract
  // An implementation of BlifSolveMethod
  // Computes an under approximation using cudd's Clip logic
  class ClippingAndAbstract:
    public BlifSolveMethod
  {
    public:
      ClippingAndAbstract(int maxDepth, bool isOverApprox) :
        m_maxDepth(maxDepth),
        m_isOverApprox(isOverApprox)
    { }

      bdd_ptr_set solve(BlifFactors const & blifFactors) const override
      {
        int direction = m_isOverApprox ? dd_constants::Clip_Up : dd_constants::Clip_Down;
        auto funcs = blifFactors.getFactors();
        bdd_ptr_set funcSet(funcs->cbegin(), funcs->cend());
        auto ddm = blifFactors.getDdManager();
        auto cube = blifFactors.getPiVars();
        auto result = bdd_clipping_and_exists_multi(ddm, funcSet, cube, m_maxDepth, direction);
        bdd_ptr_set resultSet;
        resultSet.insert(result);
        return resultSet;
      }

    private:
      int m_maxDepth;
      bool m_isOverApprox;
  }; // end class ClippingAndAbstract

  // ***** Class *****
  // AcyclicViaForAll
  // An implementation for BlifSolveMethod
  // Commpute an underapproximation of the and-exists operator
  //   - create the factor graph
  //   - do a breadth first traversal, and collect all back-edges
  //   - delete each back-edge by universally quantifying out
  //       the variable(s) from the function
  //   - do an acyclic-message passing using the remaining graph
  // *****************
  class AcyclicViaForAll:
    public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blifFactors) const override
      {
        // create factor graph
        auto funcs = blifFactors.getFactors();
        auto ddm = blifFactors.getDdManager();
        auto fg = factor_graph_new(ddm, &(funcs->front()), funcs->size());


        // group non-pi vars
        auto nonPiVarVec = blifFactors.getNonPiVars();
        auto nonPiVars = bdd_one(ddm);
        for (auto npv: (*nonPiVarVec))
          bdd_and_accumulate(ddm, &nonPiVars, npv);
        factor_graph_group_vars(fg, nonPiVars);
        auto rootNode = factor_graph_get_varnode(fg, nonPiVars);

        

        // initialize the colors for bfs
        const int COLOR_UNVISITED = 1, COLOR_QUEUED = 2, COLOR_VISITED = 3;
        for_each_list(fg->fl, 
                      [&](fgnode_list* fl)
                      { 
                        fl->n->color = COLOR_UNVISITED; 
                      });
        for_each_list(fg->vl,
                      [&](fgnode_list* vl)
                      {
                        vl->n->color = COLOR_UNVISITED;
                      });


        // add the root node to the var queue
        std::queue<fgnode *> q;
        q.push(rootNode);
        rootNode->color = COLOR_QUEUED;


        // set of back edges
        // maps the functions to the variables
        std::map<bdd_ptr, std::set<bdd_ptr>> backEdges;


        // BFS: loop until queue is empty
        while(!q.empty())
        {
          fgnode* curNode = q.front();
          q.pop();
          curNode->color = COLOR_VISITED;
          for_each_list(curNode->neigh,
                       [&](fgedge_list* el)
                       {
                         fgnode * nextNode = (curNode->type == VAR_NODE ? el->e->fn : el->e->vn);
                         if (curNode->parent == nextNode)
                           return;
                         else if (nextNode->color != COLOR_UNVISITED)
                         {
                           // found a back edge, record it
                           fgnode * fn = curNode->type == FUNC_NODE ? curNode : nextNode;
                           fgnode * vn = curNode->type == VAR_NODE ? curNode : nextNode;
                           for (int fidx = 0; fidx < fn->fs; ++fidx)
                           for (int vidx = 0; vidx < vn->fs; ++vidx)
                               backEdges[fn->f[fidx]].insert(vn->f[vidx]);
                         }
                         else
                         {
                           // no back edge, add to queue
                           nextNode->parent = curNode;
                           nextNode->color = COLOR_QUEUED;
                           q.push(nextNode);
                         }
                       });


        } // end of BFS loop



        // create functions which would lead to an acyclic graph
        std::vector<bdd_ptr> acyclicFuncs;
        for (auto func: *funcs)
        {
          auto bei = backEdges.find(func);
          if (bei != backEdges.end())
          {
            // found back edge for this function, quantify out all vars
            bdd_ptr vars = bdd_one(ddm);
            for (auto vi = bei->second.cbegin(); vi != bei->second.cend(); ++vi)
              bdd_and_accumulate(ddm, &vars, *vi);
            acyclicFuncs.push_back(bdd_forall(ddm, func, vars));
            bdd_free(ddm, vars);
          }
          else
          {
            // no back edges, keep as it is
            acyclicFuncs.push_back(bdd_dup(func));
          }
        }



        // pass acyclic messages and collect results
        auto acyclicFg = factor_graph_new(ddm, &acyclicFuncs.front(), acyclicFuncs.size());
        factor_graph_group_vars(acyclicFg, nonPiVars);
        auto acyclicRootNode = factor_graph_get_varnode(acyclicFg, nonPiVars);
        factor_graph_acyclic_messages(acyclicFg, acyclicRootNode);
        int resultSize;
        bdd_ptr* resultArray = factor_graph_incoming_messages(acyclicFg, acyclicRootNode, &resultSize);


        // clean-up
        for(auto acyclicFunc: acyclicFuncs)
          bdd_free(ddm, acyclicFunc);
        factor_graph_delete(fg);
        factor_graph_delete(acyclicFg);
        bdd_free(ddm, nonPiVars);



        return bdd_ptr_set(resultArray, resultArray + resultSize);
      }

  }; // end class AcyclicViaForAll





  // ***** Class *****
  // FactorGraphApprox
  // An implementation for BlifSolveMethod
  // Apply the factor graph algorithm to compute the transition relation
  //   - create the factor_graph using the set of nodes in the network
  //   - merge all the var nodes that are not pi<nnn> (primary inputs) into a single node R
  //   - pass messages, collect the conjunction of messages coming into R
  // *****************
  class FactorGraphApprox:
    public BlifSolveMethod
  {
    public:
      FactorGraphApprox(int largestSupportSet,
                        int numConvergence, 
                        std::string dotDumpPath):
        m_largestSupportSet(largestSupportSet),
        m_numConvergence(numConvergence),
        m_dotDumpPath(dotDumpPath)
      { }

      bdd_ptr_set solve(BlifFactors const & blifFactors) const override
      {
        // setup
        blif_solve_log(INFO, "largestSupportSet " << m_largestSupportSet << ", numConvergence:" << m_numConvergence);
        auto funcs = blifFactors.getFactors();      // the set of functions
        auto nonPiVars = blifFactors.getNonPiVars();
        auto ddm = blifFactors.getDdManager();
        bdd_ptr_set result; // the set of results
        int last_result_size = -1; // do not iterate if number of results is not increasing


        // run message passing multiple times and collect results
        int nc = 0;
        for ( ; nc < m_numConvergence && static_cast<int>(result.size()) > last_result_size; ++nc)
        {
          // group the funcs in the factor graph
          auto start = now();
          auto mergeResults = merge(ddm, *funcs, *nonPiVars, m_largestSupportSet);
          auto & funcGroups = *mergeResults.factors;
          blif_solve_log(INFO, "Grouped func nodes in " << duration(start) << " secs");
          start = now();
          factor_graph * fg = factor_graph_new(ddm, &funcGroups.front(), funcGroups.size()); // the factor graph
          blif_solve_log(INFO, "Created factor graph with "
              << funcGroups.size() << " functions in "
              << duration(start) << " secs");
          for (auto funcGroup: funcGroups)
            bdd_free(ddm, funcGroup);



          // dump original factor graph in the first iteration
          if (0 == nc && m_dotDumpPath.size() > 0)
          {
            std::string original_fg_path = m_dotDumpPath + "/original_factor_graph.dot";
            factor_graph_print(fg, original_fg_path.c_str(), "/dev/null");
          }


          // group the non-pi variables in the factor graph
          auto & nonPiVarGroups = *mergeResults.variables;
          for (auto varGroup: nonPiVarGroups)
            factor_graph_group_vars(fg, varGroup);
          blif_solve_log(INFO, "Grouped non-pi variables in "
              << duration(start) << " secs"); 


          // dump factor graph with grouped var nodes
          if (m_dotDumpPath.size() > 0 && blif_solve::getVerbosity() >= blif_solve::DEBUG)
          {
            std::stringstream groupedFgPathSs;
            groupedFgPathSs << m_dotDumpPath << "/factor_graph_approx_" << nc << ".dot";
            std::string groupedFgPath = groupedFgPathSs.str();
            factor_graph_print(fg, groupedFgPath.c_str(), "/dev/null");
          }




          // pass messages till convergence
          start = now();
          blif_solve_log(INFO, "Initiating message passing");
          int numIterations = factor_graph_converge(fg);
          blif_solve_log(INFO, "Factor graph messages have converged in "
              << numIterations << " iterations");
          blif_solve_log(INFO, "Factor graph messages have converged in "
              << duration(start) << " secs");




          // compute the result by conjoining all incoming messages
          last_result_size = result.size();
          for (auto nonPiVarCube: nonPiVarGroups)
          {
            fgnode * V = factor_graph_get_varnode(fg, nonPiVarCube);
            int num_messages;
            bdd_ptr *messages = factor_graph_incoming_messages(fg, V, &num_messages);
            for (int mi = 0; mi < num_messages; ++mi)
            {
              if (result.count(messages[mi]) == 0)
                result.insert(messages[mi]);
              else 
                bdd_free(ddm, messages[mi]);
            }
            free(messages);
          }
          factor_graph_delete(fg);


          for (auto nonPiVarGroup: nonPiVarGroups)
            bdd_free(ddm, nonPiVarGroup);

          // dump results for debugging
          if (m_dotDumpPath.size() > 0 && blif_solve::getVerbosity() >= blif_solve::DEBUG)
          {
            bdd_ptr_set zero;
            zero.insert(bdd_zero(ddm));
            //bdd_ptr_set allVars(nonPiVars->cbegin(), nonPiVars->cend());
            bdd_ptr_set allVars;
            std::stringstream outputPath;
            outputPath << m_dotDumpPath << "/result_iter_" << nc << ".dimacs";
            blif_solve::dumpCnfForModelCounting(ddm, allVars, result, zero, outputPath.str());
          }



        } // end loop on number of iterations
        blif_solve_log(INFO, "Ran " << nc << " FactorGraph convergences");


        return result;
      } // end 

    private:
      int m_largestSupportSet;
      int m_numConvergence;
      std::string m_dotDumpPath;
  }; // end of class FactorGraphApprox



  

  // ***** Class *****
  // ExactAndAccumulate
  // An implementation for BlifSolveMethod
  // Uses cudd to conjoin all the factors and then
  // quantify out the variables
  // *****************
  class ExactAndAccumulate: public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override 
      {
        auto manager = blif_factors.getDdManager();
        auto func = bdd_one(manager);
        bdd_ptr_set result;
        {
          auto factors = blif_factors.getFactors();
          for (auto fi = factors->cbegin(); fi != factors->cend(); ++fi)
            bdd_and_accumulate(manager, &func, *fi);
          blif_solve_log_bdd(DEBUG, "Conjunction of factors in partition", manager, func);
          auto piVars = blif_factors.getPiVars();
          auto forsome = bdd_forsome(manager, func, piVars);
          blif_solve_log_bdd(DEBUG, "Quantified result from partition", manager, forsome);
          result.insert(forsome);
        }
        bdd_free(manager, func);
        return result;
      }
  }; // end class ExactAndAccumulate





  // ***** Class *****
  // ExactAndAbstractMulti
  // An implementation for BlifSolveMethod
  // Uses the newly implemented cudd method Cudd_bddAndAbstractMulti
  //   to conjoin all the factors
  //   and abstract away all primary input variables
  //   in a single pass
  // *****************
  class ExactAndAbstractMulti
    : public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override
      {
        auto manager = blif_factors.getDdManager();
        auto factors = blif_factors.getFactors();
        bdd_ptr_set factor_set(factors->cbegin(), factors->cend());
        auto cube = blif_factors.getPiVars();
        bdd_ptr_set result;
        result.insert(bdd_and_exists_multi(manager, factor_set, cube));
        return result;
      }
  };




  // ***** Class *****
  // True
  // An implementation for BlifSolveMethod
  // Always returns true
  // *****************
  class True
    : public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override
      {
        bdd_ptr_set result;
        result.insert(bdd_one(blif_factors.getDdManager()));
        return result;
      }
  };

  // ***** Class *****
  // False
  // An implementation for BlifSolveMethod
  // Always returns false
  // *****************
  class False
    : public BlifSolveMethod
  {
    public:
      bdd_ptr_set solve(BlifFactors const & blif_factors) const override
      {
        bdd_ptr_set result;
        result.insert(bdd_zero(blif_factors.getDdManager()));
        return result;
      }
  };

}// end anonymous namespace



namespace blif_solve
{

  BlifSolveMethodCptr BlifSolveMethod::createExactAndAccumulate()
  {
    return std::make_shared<ExactAndAccumulate>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createExactAndAbstractMulti()
  {
    return std::make_shared<ExactAndAbstractMulti>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createFactorGraphApprox(
      int largestSupportSet,
      int numConvergence,
      std::string const & dotDumpPath)
  {
    return std::make_shared<FactorGraphApprox>(largestSupportSet, numConvergence, dotDumpPath);
  }

  BlifSolveMethodCptr BlifSolveMethod::createAcyclicViaForAll()
  {
    return std::make_shared<AcyclicViaForAll>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createTrue()
  {
    return std::make_shared<True>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createFalse()
  {
    return std::make_shared<False>();
  }

  BlifSolveMethodCptr BlifSolveMethod::createClippingAndAbstract(int clippingDepth, bool isClippingOverApproximated)
  {
    return std::make_shared<ClippingAndAbstract>(clippingDepth, isClippingOverApproximated);
  }


} // end namespace blif_solve
