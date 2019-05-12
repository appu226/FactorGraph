#include "approx_merge.h"

#include <disjoint_set.h>
#include <max_heap.h>

#include <list>
#include <stdexcept>

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
    { }

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
      return std::optional<double>();
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
    for (auto factor: factors)
      funcNodes.push_back(std::make_unique<AmNode>(AmNode::Func, manager, factor));

    // create var nodes
    std::vector<std::unique_ptr<AmNode> > varNodes;
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
        if (v1 < v2 && v1->isConnectedTo(*v2)) {
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
    // until you've exhausted all optional
    while (heap.size() > 0)
    {
      auto merger = heap.top();
      heap.pop();
      if (merger->node1->type != merger->node2->type)
        throw std::runtime_error("Assertion failure: node1 and node2 in merger don't have consistent type");

      // create merged node
      bdd_ptr mergedBdd = bdd_and(manager, merger->node1->node, merger->node2->node);
      auto & nodeVec = merger->node1->type == AmNode::Func ? funcNodes : varNodes;
      nodeVec.push_back(std::make_unique<AmNode>(AmNode::Func, manager, mergedBdd));
      bdd_free(manager, mergedBdd);
      auto mergedNode = nodeVec.back().get();

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
          heap.remove(oldMerger->heap_entry);
          if (otherNode == NULL)
            continue;
          if (oldMergerSet.count(otherNode) != 0)
            continue;
          oldMergerSet.insert(otherNode);
          auto optPriority = getCompatibility(mergedNode, otherNode, largestSupportSet);
          if (optPriority)
          {
            mergers.push_back(std::make_unique<AmMerger>(mergedNode, otherNode));
            auto newMerger = mergers.back().get();
            merger->heap_entry = heap.insert(merger, *optPriority);
          }
        }
      }
    }

  }

} // end namespace blif_solve
