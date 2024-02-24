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

#include <dd/disjoint_set.h>
#include <dd/max_heap.h>

#include <list>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <unordered_map>

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
    std::string name;

    AmNode(NodeType v_type, DdManager * v_manager, bdd_ptr v_node, const std::string& v_name):
      type(v_type),
      manager(v_manager),
      node(bdd_dup(v_node)),
      supportSet(v_type == Func ? bdd_support(v_manager, v_node) : v_node),
      neighbours(),
      mergers(),
      name(v_name)
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

  struct NodeData {
    std::string name;
    int count;
  };

  struct NodeCounterSet {

    std::unordered_map<bdd_ptr, NodeData> data;

    void addNode(bdd_ptr node, const std::string& name)
    {
      if (data.count(node)) 
        data[node].count++;
      else
        data[node] = {name, 1};
    }

    void removeNode(bdd_ptr node)
    {
      auto nit = data.find(node);
      if (nit == data.end())
        throw std::runtime_error("Error: Attempting to erase non-existent node");
      nit->second.count--;
      if (nit->second.count == 0)
        data.erase(nit);
    }

    void getAllData(std::vector<bdd_ptr> & nodes, std::vector<std::string> & names) const
    {
      nodes.reserve(data.size());
      names.reserve(data.size());
      for (const auto& dit: data)
      {
        nodes.push_back(bdd_dup(dit.first));
        names.push_back(dit.second.name);
      }
    }

  };


  std::optional<double>
    getCompatibility(AmNode * f1, AmNode * f2, const int largestSupportSet, const int largestBddSize, const double hint, const std::set<bdd_ptr>& quantifiedVariables)
  {
    bool isF1Quantified = quantifiedVariables.count(f1->supportSet);
    bool isF2Quantified = quantifiedVariables.count(f2->supportSet);
    if (isF1Quantified != isF2Quantified)
      return std::optional<double>();
    auto combinedBddSize = bdd_size(f1->node) + bdd_size(f2->node);
    if (combinedBddSize > largestBddSize)
    {
#ifdef DEBUG_MERGE
      std::cout << "cannot merge " << f1 << " and " << f2 << " because combinedBddSize " << combinedBddSize << " is larger than largestBddSize " << largestBddSize << std::endl;
#endif
      return std::optional<double>();
    }
    auto manager = f1->manager;
    auto combinedSupportSet = bdd_cube_union(manager, f1->supportSet, f2->supportSet);
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
    return commonSize / std::min(f1Size, f2Size) + hint;
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

  blif_solve::MergeHints::FactorPair makeFactorPair(bdd_ptr f1, bdd_ptr f2, DdManager* manager) {
    typedef dd::BddWrapper bw;
    return blif_solve::MergeHints::FactorPair{bw(bdd_dup(f1), manager), bw(bdd_dup(f2), manager)};
  }


  std::vector<std::string> checkNamesInput(const std::vector<std::string>& names, size_t numberOfNodes, char const * const nodeType)
  {
    if (nodeType == nullptr || nodeType[0] == '\0')
      throw std::runtime_error("blif_solve::checkNamesInput: nodeType can neither be null nor empty string");
    auto numberOfNames = names.size();
    if (numberOfNames > 0 && numberOfNames != numberOfNodes)
      throw std::runtime_error("blif_solve::merge : mismatching inputs " + std::to_string(numberOfNodes) + " bdds and " + std::to_string(numberOfNames) + " names for " + nodeType + " nodes.");
    if (numberOfNames > 0)
      return names;
    std::vector<std::string> result;
    result.reserve(numberOfNodes);
    char namePrefix = nodeType[0];
    for (size_t i = 1; i <= numberOfNodes; ++i)
      result.push_back(namePrefix + std::to_string(i));
    return result;
  }

  std::string mergedName(const std::string& name1, const std::string& name2)
  {
#ifdef DEBUG_MERGE
     return name1 + " && " + name2;
#else
     return "";
#endif
  }

} // end anonymous namespace






namespace blif_solve
{


  void MergeHints::addWeight(bdd_ptr func1, bdd_ptr func2, double weight)
  {
    if (func2 < func1) return addWeight(func2, func1, weight);
    if (func1 == func2) return;

    m_weights.insert(std::make_pair(makeFactorPair(func1, func2, m_manager), weight));
  }

  double MergeHints::getWeight(bdd_ptr func1, bdd_ptr func2) const
  {
    if (func2 < func1) return getWeight(func2, func1);
    if (func1 == func2) return 0;

    auto it = m_weights.find(makeFactorPair(func1, func2, m_manager));
    if (it != m_weights.end())
      return it->second;
    else
      return 0;
  }

  void MergeHints::merge(bdd_ptr func1, bdd_ptr func2, bdd_ptr newFunc)
  {
    if (func2 < func1) return merge(func2, func1, newFunc);
    if (func1 == func2) return;

    std::map<BddWrapper, double> stuffToAdd;
    std::vector<FactorPair> stuffToDelete;

    auto markForAddition = [&](BddWrapper const & g, double w) {
      auto staIt = stuffToAdd.find(g);
      if (staIt == stuffToAdd.end())
        stuffToAdd[g] = w;
      else
        staIt->second = std::max(w, staIt->second);
    };

    for (auto wit = m_weights.cbegin(); wit != m_weights.cend(); ++wit) {
      auto g1 = wit->first.first;
      auto g2 = wit->first.second;
      auto w = wit->second;
      if (func1 == g1.getUncountedBdd() && func2 == g2.getUncountedBdd()) {
        stuffToDelete.push_back(makeFactorPair(func1, func2, m_manager));
      } else if(func1 == g1.getUncountedBdd())
      {
        stuffToDelete.push_back(FactorPair{g1, g2});
        markForAddition(g2, w);
      } else if (func1 == g2.getUncountedBdd())
      {
        stuffToDelete.push_back(FactorPair{g1, g2});
        markForAddition(g1, w);
      } else if (func2 == g1.getUncountedBdd())
      {
        stuffToDelete.push_back(FactorPair{g1, g2});
        markForAddition(g2, w);
      } else if (func2 == g2.getUncountedBdd())
      {
        stuffToDelete.push_back(FactorPair{g1, g2});
        markForAddition(g1, w);
      }
    }
    for (const auto & toDelete: stuffToDelete)
      m_weights.erase(toDelete);
    for (const auto & toAdd: stuffToAdd)
      addWeight(toAdd.first.getUncountedBdd(), newFunc, toAdd.second);
  }

 
  MergeResults 
    merge(DdManager * manager,
          const std::vector<bdd_ptr> & factors, 
          const std::vector<bdd_ptr> & variables, 
          int largestSupportSet,
          int largestBddSize,
          const MergeHints& hintsInput,
          const std::set<bdd_ptr>& quantifiedVariables,
          const std::vector<std::string> & v_factorNames,
          const std::vector<std::string> & v_variableNames)
  {
    auto factorNames = checkNamesInput(v_factorNames, factors.size(), "Factor");
    auto variableNames = checkNamesInput(v_variableNames, variables.size(), "Variable");

    MergeHints hints = hintsInput;
    // create func nodes
    std::vector<std::unique_ptr<AmNode> > funcNodes;
    NodeCounterSet mergedFactors;
    for (size_t fidx = 0; fidx < factors.size(); ++fidx)
    {
      funcNodes.push_back(std::make_unique<AmNode>(AmNode::Func, manager, factors[fidx], factorNames[fidx]));
      mergedFactors.addNode(factors[fidx], factorNames[fidx]);
    }

    // create var nodes
    std::vector<std::unique_ptr<AmNode> > varNodes;
    NodeCounterSet mergedVariables;
    for (size_t vidx = 0; vidx < variables.size(); ++vidx)
    {
      varNodes.push_back(std::make_unique<AmNode>(AmNode::Var, manager, variables[vidx], variableNames[vidx]));
      mergedVariables.addNode(variables[vidx], variableNames[vidx]);
    }

    // copy quantifiedVariables set
    std::set<bdd_ptr> qv;
    for (auto v: quantifiedVariables)
      qv.insert(bdd_dup(v));
    std::set<bdd_ptr> qf; // for functions
    

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
          auto optPriority = getCompatibility(f1.get(), f2.get(), largestSupportSet, largestBddSize, hints.getWeight(f1->node, f2->node), qf);
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
          auto optPriority = getCompatibility(v1.get(), v2.get(), largestSupportSet, largestBddSize, hints.getWeight(v1->node, v2->node), qv);
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
      std::set<bdd_ptr> &quantified = (merger->node1->type == AmNode::Var ? qv : qf);
      bool isQuantified = quantified.count(merger->node1->supportSet);

      // create merged node
      bdd_ptr mergedBdd = bdd_and(manager, merger->node1->node, merger->node2->node);
      if (isQuantified && quantified.count(mergedBdd) == 0) quantified.insert(bdd_dup(mergedBdd));
      hints.merge(merger->node1->node, merger->node2->node, mergedBdd);
      auto & nodeVec = merger->node1->type == AmNode::Func ? funcNodes : varNodes;
      nodeVec.push_back(std::make_unique<AmNode>(merger->node1->type, manager, mergedBdd, mergedName(merger->node1->name, merger->node2->name)));
      bdd_free(manager, mergedBdd);
      auto mergedNode = nodeVec.back().get();
      auto & mergeSet = merger->node1->type == AmNode::Func ? mergedFactors : mergedVariables;
      mergeSet.removeNode(merger->node1->node);
      mergeSet.removeNode(merger->node2->node);
      mergeSet.addNode(mergedNode->node, mergedNode->name);

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
          auto optPriority = getCompatibility(mergedNode, otherNode, largestSupportSet, largestBddSize, hints.getWeight(mergedNode->node, otherNode->node), quantified);
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
    result.factors = std::make_shared<std::vector<bdd_ptr> >();
    result.factorNames = std::make_shared<std::vector<std::string>>();
    mergedFactors.getAllData(*result.factors, *result.factorNames);
    result.variables = std::make_shared<std::vector<bdd_ptr> >();
    result.variableNames = std::make_shared<std::vector<std::string>>();
    mergedVariables.getAllData(*result.variables, *result.variableNames);
    for (auto q: qv) bdd_free(manager, q);
    return result;

  }

} // end namespace blif_solve
