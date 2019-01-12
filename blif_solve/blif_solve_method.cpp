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

// FactorGraph includes
#include <dd.h>
#include <factor_graph.h>

// std includes
#include <memory>
#include <queue>
#include <map>

namespace {

  using namespace blif_solve;

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
      FactorGraphApprox(int varNodeSize):
        m_varNodeSize(varNodeSize)
      { }

      bdd_ptr_set solve(BlifFactors const & blifFactors) const override
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
        int varNodeSize = m_varNodeSize;
        if (0 >= varNodeSize)
          varNodeSize = fg->num_vars;
        auto nonPiVars = blifFactors.getNonPiVars();
        std::vector<bdd_ptr> nonPiVarGroups;
        int lastSize = varNodeSize;
        for (auto npv: *nonPiVars)
        {
          if (lastSize >= varNodeSize)
          {
            nonPiVarGroups.push_back(bdd_one(ddm));
            lastSize = 0;
          }
          bdd_and_accumulate(ddm, &nonPiVarGroups.back(), npv);
          ++lastSize;
        }

        for (auto nonPiVarGroup: nonPiVarGroups)
          factor_graph_group_vars(fg, nonPiVarGroup);

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
        bdd_ptr_set result;
        for (auto nonPiVarCube: nonPiVarGroups)
        {
          fgnode * V = factor_graph_get_varnode(fg, nonPiVarCube);
          int num_messages;
          bdd_ptr *messages = factor_graph_incoming_messages(fg, V, &num_messages);
          for (int mi = 0; mi < num_messages; ++mi)
          {
            result.insert(messages[mi]);
          }
          free(messages);
        }
        blif_solve_log(INFO, "Computed final factor graph result in "
                              << duration(start) << " secs");

        // clean-up and return
        factor_graph_delete(fg);
        return result;

      }

    private:
      int m_varNodeSize;
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
          auto piVars = blif_factors.getPiVars();
          result.insert(bdd_forsome(manager, func, piVars));
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

  BlifSolveMethodCptr BlifSolveMethod::createFactorGraphApprox(int varNodeMergeLimit)
  {
    return std::make_shared<FactorGraphApprox>(varNodeMergeLimit);
  }

  BlifSolveMethodCptr BlifSolveMethod::createAcyclicViaForAll()
  {
    return std::make_shared<AcyclicViaForAll>();
  }


} // end namespace blif_solve
