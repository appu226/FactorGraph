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


#include "approx_merge.h"

#include <disjoint_set.h>
#include <max_heap.h>

#include <list>
#include <stdexcept>
#include <iostream>

namespace {

  using namespace parakram;
  using namespace blif_solve;

  struct AmNode;
  struct AmMerger;

  struct AmNode {
    enum NodeType { Func, Var };
    NodeType type;
    DdManager * manager;
    bdd_ptr node;
    bdd_ptr supportSet;
    std::list<AmNode *> neighbours;
    std::list<AmMerger *> mergers;

    AmNode(NodeType type, DdManager * manager, bdd_ptr node):
      type(type),
      manager(manager),
      node(bdd_dup(node)),
      supportSet(type == Func ? bdd_support(manager, node) : node),
      neighbours(),
      mergers()
    { }

    bool isConnectedTo(const AmNode & that) const
    {
      bdd_ptr supportSetIntersection = bdd_cube_intersection(manager, this->supportSet, that.supportSet);
      bool result = !bdd_is_one(manager,supportSetIntersection);
      bdd_free(manager, supportSetIntersection);
      return result;

    }

    ~AmNode()
    {
      bdd_free(manager, node);
      if (type == Func)
        bdd_free(manager, supportSet);
    }
  };

  struct AmMerger {
    
    typedef std::list<AmMerger *>::iterator MergerListEntry;
   
    AmNode * node1;
    AmNode * node2;
    MergerListEntry node1_entry;
    MergerListEntry node2_entry;
    
    MaxHeap<AmMerger *, double>::DataCellCptr heap_entry; 
    
    AmMerger(AmNode * node1, AmNode * node2):
      node1(node1),
      node2(node2),
      node1_entry(node1->mergers.insert(node1->mergers.end(), this)),
      node2_entry(node2->mergers.insert(node2->mergers.end(), this)),
      heap_entry()
    {
      if (node1->type != node2->type)
        throw std::runtime_error("AmMerger::AmMerger: node1 and node2 types must match");
    }

  };


  std::optional<double>
    getCompatibility(AmNode * f1, AmNode * f2, const int largestSupportSet)
  {
    auto manager = f1->manager;
    auto combinedSupportSet = bdd_dup(f1->supportSet);
    bdd_and_accumulate(manager, &combinedSupportSet, f2->supportSet);
    for (auto neigh: f1->neighbours)
      bdd_and_accumulate(manager, &combinedSupportSet, neigh->supportSet);
    for (auto neigh: f2->neighbours)
      bdd_and_accumulate(manager, &combinedSupportSet, neigh->supportSet);
    int unionSize = bdd_size(combinedSupportSet);
    bdd_free(manager, combinedSupportSet);
    if (unionSize > largestSupportSet)
    {
#ifdef DEBUG_MERGE
      std::cout << "cannot merge " << f1 << " and " << f2 << " because unionsize " << unionSize << " is larger than largestSupportSet " << largestSupportSet << std::endl;
#endif
      return std::optional<double>();
    }
#ifdef DEBUG_MERGE
    std::cout << "merging " << f1 << " and " << f2 << std::endl;
#endif
    auto commonSupportSet = bdd_cube_intersection(manager, f1->supportSet, f2->supportSet);
    double commonSize = bdd_size(commonSupportSet);
    bdd_free(manager, commonSupportSet);
    double f1Size = bdd_size(f1->supportSet);
    double f2Size = bdd_size(f2->supportSet);
    return commonSize / std::min(f1Size, f2Size);
  }

  AmNode *
    pullOutOtherNode(AmMerger * merger, AmNode * node1, AmNode * node2)
  {
    if (merger->node1 != node1 && merger->node1 != node2)
    {
      merger->node1->mergers.erase(merger->node1_entry);
      return merger->node1;
    }
    else if (merger->node2 != node1 && merger->node2 != node2)
    {
      merger->node2->mergers.erase(merger->node2_entry);
      return merger->node2;
    }
    else return NULL;
  }

} // end anonymous namespace

namespace blif_solve
{

 
  MergeResults 
    merge(DdManager * manager,
          const std::vector<bdd_ptr> & factors, 
          const std::vector<bdd_ptr> & variables, 
          int largestSupportSet)
  {
    // create func nodes
    std::vector<std::unique_ptr<AmNode> > funcNodes;
    std::set<bdd_ptr> mergedFactors(factors.cbegin(), factors.cend());
    for (auto factor: factors)
      funcNodes.push_back(std::make_unique<AmNode>(AmNode::Func, manager, factor));

    // create var nodes
    std::vector<std::unique_ptr<AmNode> > varNodes;
    std::set<bdd_ptr> mergedVariables(variables.cbegin(), variables.cend());
    for (auto variable: variables)
      varNodes.push_back(std::make_unique<AmNode>(AmNode::Var, manager, variable));

    // create func-var connections
    for (auto & func: funcNodes) {
      for (auto & var: varNodes) {
        if (func < var && func->isConnectedTo(*var)) {
          func->neighbours.push_back(var.get());
          var->neighbours.push_back(func.get());
        }
      }
    }

    std::vector<std::unique_ptr<AmMerger> > mergers;
    MaxHeap<AmMerger*, double> heap;
    // create func-func connections
    for (auto & f1: funcNodes) {
      for (auto & f2: funcNodes) {
        if (f1 < f2 && f1->isConnectedTo(*f2)) {
          auto optPriority = getCompatibility(f1.get(), f2.get(), largestSupportSet);
          if (optPriority) {
            mergers.push_back(std::make_unique<AmMerger>(f1.get(), f2.get()));
            auto merger = mergers.back().get();
            merger->heap_entry = heap.insert(merger, *optPriority);
          } 
        }
      }
    }


    // create var-var connections
    for (auto & v1: varNodes) {
      for (auto & v2: varNodes) {
        if (v1 < v2) {
          auto optPriority = getCompatibility(v1.get(), v2.get(), largestSupportSet);
          if (optPriority) {
            mergers.push_back(std::make_unique<AmMerger>(v1.get(), v2.get()));
            auto merger = mergers.back().get();
            merger->heap_entry = heap.insert(merger, *optPriority);
          }
        }
      }
    }


    // execute the most promising merger
    // until you've exhausted all options
    while (heap.size() > 0)
    {
      auto merger = heap.top();
      heap.pop();
      if (merger->node1->type != merger->node2->type)
        throw std::runtime_error("Assertion failure: node1 and node2 in merger don't have consistent type");

      // create merged node
      bdd_ptr mergedBdd = bdd_and(manager, merger->node1->node, merger->node2->node);
      auto & nodeVec = merger->node1->type == AmNode::Func ? funcNodes : varNodes;
      nodeVec.push_back(std::make_unique<AmNode>(merger->node1->type, manager, mergedBdd));
      bdd_free(manager, mergedBdd);
      auto mergedNode = nodeVec.back().get();
      auto & mergeSet = merger->node1->type == AmNode::Func ? mergedFactors : mergedVariables;
      mergeSet.erase(merger->node1->node);
      mergeSet.erase(merger->node2->node);
      mergeSet.insert(mergedNode->node);

      // merge the neighbour lists
      {
        std::set<AmNode *> mergedNeighbourSet;
        for (auto neigh: merger->node1->neighbours)
          mergedNeighbourSet.insert(neigh);
        for (auto neigh: merger->node2->neighbours)
          mergedNeighbourSet.insert(neigh);
        for (auto neigh: mergedNeighbourSet)
          mergedNode->neighbours.push_back(neigh);
      }

      // refresh the list of mergers
      {
        std::set<AmNode *> oldMergerSet;
        std::list<AmMerger *> oldMergers;
        oldMergers.splice(oldMergers.end(), merger->node1->mergers);
        oldMergers.splice(oldMergers.end(), merger->node2->mergers);
        for (auto oldMerger: oldMergers)
        {
          AmNode * otherNode = pullOutOtherNode(oldMerger, merger->node1, merger->node2);
          if (otherNode == NULL)
            continue;
          heap.remove(oldMerger->heap_entry);
          if (oldMergerSet.count(otherNode) != 0)
            continue;
          oldMergerSet.insert(otherNode);
          auto optPriority = getCompatibility(mergedNode, otherNode, largestSupportSet);
          if (optPriority)
          {
            mergers.push_back(std::make_unique<AmMerger>(mergedNode, otherNode));
            auto newMerger = mergers.back().get();
            newMerger->heap_entry = heap.insert(newMerger, *optPriority);
          }
        }
      }
    }
    MergeResults result;
    result.factors = std::make_shared<std::vector<bdd_ptr> >(mergedFactors.cbegin(), mergedFactors.cend());
    result.variables = std::make_shared<std::vector<bdd_ptr> >(mergedVariables.cbegin(), mergedVariables.cend());
    for (auto factor: *result.factors) bdd_dup(factor);
    for (auto variable: *result.variables) bdd_dup(variable);
    return result;

  }

} // end namespace blif_solve
