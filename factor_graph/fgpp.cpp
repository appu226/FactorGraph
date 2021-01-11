/*

Copyright 2020 Parakram Majumdar

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

#include "fgpp.h"

#include <stdexcept>
#include <map>
#include <cassert>

namespace {

  struct FGNode;
  struct FGEdge;
  struct FGVariableNode;
  struct FGFactorNode;
  typedef std::shared_ptr<FGNode> FGNodePtr;
  typedef std::set<FGNodePtr> FGNodePtrSet;
  typedef std::weak_ptr<FGVariableNode> FGVariableNodeWeakPtr;
  typedef std::shared_ptr<FGVariableNode> FGVariableNodePtr;
  typedef std::weak_ptr<FGFactorNode> FGFactorNodeWeakPtr;
  typedef std::shared_ptr<FGFactorNode> FGFactorNodePtr;
  typedef std::shared_ptr<FGEdge> FGEdgePtr;
  typedef std::set<FGEdgePtr> FGEdgePtrSet;

  struct FGNode {
    FGEdgePtrSet edges;
    dd::BddWrapper nodeBdd;
    virtual ~FGNode() {}
    FGNode(const dd::BddWrapper & v_nodeBdd) : edges(), nodeBdd(v_nodeBdd) {}
    virtual void passMessages(FGNodePtrSet & updatedNodes) = 0;
  };

  struct FGEdge {
    FGVariableNodeWeakPtr variableNode;
    FGFactorNodeWeakPtr factorNode;
    dd::BddWrapper variableToFactorMessage;
    dd::BddWrapper factorToVariableMessage;
    FGVariableNodePtr getVariableNode()
    {
      FGVariableNodePtr result = variableNode.lock();
      if (!result) throw std::runtime_error("Invalid weak pointer in FGEdge::getVariableNode()");
      return result;
    }
    FGFactorNodePtr getFactorNode()
    {
      FGFactorNodePtr result = factorNode.lock();
      if (!result) throw std::runtime_error("Invalid weak pointer in FGEdge::getFactorNode()");
      return result;
    }
    FGEdge(const FGVariableNodePtr & variableNode, const FGFactorNodePtr & factorNode);
  };

  struct FGVariableNode : public FGNode {
    FGVariableNode(const dd::BddWrapper & v_nodeBdd): FGNode(v_nodeBdd) {}
    virtual void passMessages(FGNodePtrSet & updatedNodes) override;
  };

  struct FGFactorNode : public FGNode {
    FGFactorNode(const dd::BddWrapper & v_nodeBdd): FGNode(v_nodeBdd), supportBdd(v_nodeBdd.support()) {}
    virtual void passMessages(FGNodePtrSet & updatedNodes) override;
    dd::BddWrapper supportBdd;
  };


  dd::BddWrapper project(const dd::BddWrapper & factor, const dd::BddWrapper & cube)
  {
    return factor.existentialQuantification(factor.support().cubeDiff(cube));
  }


  class FactorGraphImpl: public fgpp::FactorGraph
  {
    public:

      typedef dd::BddWrapper BddWrapper;

      FactorGraphImpl(const std::vector<BddWrapper> & factors);

      void groupVariables(const BddWrapper & variableCube) override;
      std::vector<BddWrapper> getIncomingMessages(const BddWrapper & variableCube) const override;
      int converge() override;

      static void test(DdManager *);

    private:
      FGEdgePtrSet m_edges;
      FGNodePtrSet m_factorNodes;
      FGNodePtrSet m_variableNodes;

  };




  FGEdge::FGEdge(const FGVariableNodePtr & v_variableNode, const FGFactorNodePtr & v_factorNode):
    variableNode(v_variableNode),
    factorNode(v_factorNode),
    variableToFactorMessage(v_variableNode->nodeBdd.one()),
    factorToVariableMessage(v_variableNode->nodeBdd.one())
  { }






  void FGVariableNode::passMessages(FGNodePtrSet & updatedNodes)
  {
    using namespace dd;
    // compute message
    BddWrapper message =  nodeBdd.one();
    for (const auto & edge: edges)
      message = message * edge->factorToVariableMessage;

    // update message for each edge
    for (const auto & edge: edges)
    {
      // project the message
      FGFactorNodePtr factorNode = edge->getFactorNode();
      BddWrapper factorMessage = project(message, factorNode->supportBdd);

      
      // if message is already updated, skip
      if (edge->variableToFactorMessage == factorMessage)
        continue;

      // update the message
      edge->variableToFactorMessage = factorMessage;
      updatedNodes.insert(factorNode);
    }

  }



  void FGFactorNode::passMessages(FGNodePtrSet & updatedNodes)
  {
    using namespace dd;
    // compute conjoined message
    BddWrapper conjoined = nodeBdd;
    for (const auto & edge: edges)
      conjoined = conjoined * edge->variableToFactorMessage;

    // update message for each edge
    for (const auto & edge: edges)
    {
      // project the message
      FGVariableNodePtr variableNode = edge->getVariableNode();
      BddWrapper variableMessage = project(conjoined, variableNode->nodeBdd);

      // if message is already updated, skip
      if (edge->factorToVariableMessage == variableMessage)
        continue;

      // update the message
      edge->factorToVariableMessage = variableMessage;
      updatedNodes.insert(variableNode);
    }
  }



  FactorGraphImpl::FactorGraphImpl(const std::vector<BddWrapper> & factors)
  {
    if (factors.empty())
      return;

    std::map<bdd_ptr, FGVariableNodePtr> varNodeMap;
    // for each factor
    for (const auto & factor: factors)
    {
      // create a factor node
      FGFactorNodePtr fnode = std::make_shared<FGFactorNode>(factor);
      m_factorNodes.insert(fnode);
      BddWrapper fsup = fnode->supportBdd;
      // for each var in factor
      while(!fsup.isOne())
      {
        auto v = fsup.varWithLowestIndex();
        fsup = fsup.cubeDiff(v);
        auto vnmit = varNodeMap.find(v.getUncountedBdd());
        if (vnmit == varNodeMap.end())
        {
          // if var node doesn't already exist, create it
          auto new_vn = std::make_shared<FGVariableNode>(v);
          m_variableNodes.insert(new_vn);
          vnmit = varNodeMap.insert(std::make_pair(v.getUncountedBdd(), new_vn)).first;
        }
        // create an edge
        auto vnode = vnmit->second;
        FGEdgePtr edge = std::make_shared<FGEdge>(vnode, fnode);
        m_edges.insert(edge);
        vnode->edges.insert(edge);
        fnode->edges.insert(edge);
      }
    }
  }



  int FactorGraphImpl::converge()
  {
    // reset all messages
    if (m_factorNodes.empty()) return 0;
    auto one = (*m_factorNodes.cbegin())->nodeBdd.one();
    for (const auto & edge: m_edges)
    {
      edge->variableToFactorMessage = one;
      edge->factorToVariableMessage = one;
    }

    // set up factor nodes for message passing
    FGNodePtrSet pendingSet(m_factorNodes.cbegin(), m_factorNodes.cend());
    
    // pass messages and collect nodes for next iteration
    int numIterations = 0;
    while(!pendingSet.empty())
    {
      ++numIterations;
      FGNodePtrSet updatedNodes;
      for (const auto & node: pendingSet)
        node->passMessages(updatedNodes);

      pendingSet.swap(updatedNodes);
    }

    return numIterations;
  }




  std::vector<dd::BddWrapper> FactorGraphImpl::getIncomingMessages(const BddWrapper & variableCube) const
  {
    std::vector<BddWrapper> result;
    result.reserve(m_edges.size());
    for (const auto & vnode: m_variableNodes)
    {
      if (vnode->nodeBdd.cubeIntersection(variableCube).isOne())
        continue;
      for (const auto & e: vnode->edges)
        result.push_back(e->factorToVariableMessage);
    }
    return result;
  }





  void FactorGraphImpl::groupVariables(const BddWrapper & variableCube)
  {
    if (m_variableNodes.empty())
      return;
    FGVariableNodePtr new_vnode = std::make_shared<FGVariableNode>((*m_variableNodes.cbegin())->nodeBdd.one());
    FGNodePtrSet neighbors;
    for (auto vit = m_variableNodes.begin(); vit != m_variableNodes.end();)
    {
      FGNodePtr old_vnode = *vit;
      if (old_vnode->nodeBdd.cubeIntersection(variableCube).isOne())
      {
        ++vit;
        continue;
      }

      new_vnode->nodeBdd = new_vnode->nodeBdd.cubeUnion(old_vnode->nodeBdd);
      for (const auto & old_edge: old_vnode->edges)
      {
        const auto & old_fnode = old_edge->getFactorNode();
        m_edges.erase(old_edge);
        old_fnode->edges.erase(old_edge);
        if (neighbors.count(old_fnode))
          continue;
        neighbors.insert(old_fnode);
        FGEdgePtr new_edge = std::make_shared<FGEdge>(new_vnode, old_fnode);
        m_edges.insert(new_edge);
        old_fnode->edges.insert(new_edge);
        new_vnode->edges.insert(new_edge);
      }
      vit = m_variableNodes.erase(vit);
    }
    m_variableNodes.insert(new_vnode);
  }

} // end anonymous namespace












namespace fgpp
{

  
  FactorGraph::Ptr FactorGraph::createFactorGraph(const std::vector<BddWrapper> & factors)
  {
    return std::make_shared<FactorGraphImpl>(factors);
  }


  void FactorGraph::testFactorGraphImpl(DdManager * manager)
  {
    FactorGraphImpl::test(manager);
  }

} // end namespace fgpp









//-----------------------------------------------
// Tests
//-----------------------------------------------
namespace {

  using namespace dd;

  void FactorGraphImpl::test(DdManager * manager)
  {
    // f0 -- v1 -- f1 -- v2 -- f2 -- v4
    // |     |     |
    // v0    f4    v3
    // |     |
    // f3    v5 -- f5 -- v6 -- f6 -- v10
    // |           |           |
    // + -- v7     v9 -- f7 -- v11
    // |
    // + -- v8

    using namespace dd;
    std::vector<BddWrapper> V; // variables
    const int numVars = 12;
    std::vector<BddWrapper> F; // factors
    const int numFactors = 8;

    // generate variables
    for (int i = 1; i <= numVars; ++i)
      V.emplace_back(bdd_new_var_with_index(manager, i), manager);

    F.push_back(V[0] * -V[1]);                      // f0 = v0 * -v1
    F.push_back((-V[1] * V[3]) + (V[1] * V[2]));    // f1 = (-v1 * v3) + (v1 * v2)
    F.push_back(-V[2] * V[4]);                      // f2 = -v2 * v4
    F.push_back(-V[0] + (V[7] * V[8]));             // f3 = v0 -> (v7 * v8)
    F.push_back(V[1] + V[5]);                       // f4 = -v1 -> v5

    F.push_back((V[5] * V[6]) + (-V[5] + V[9]));    // f5 = (v5 * v6) + (-v5 * v9)
    F.push_back((V[6] * V[11]) + (-V[6] * -V[10])); // f6 = (v6 * v11) + (-v6 * -v10)
    F.push_back(-V[11] + V[9]);                     // f7 = v11 -> v9

    // test factor graph creation
    FactorGraphImpl fg1(F);
    assert(fg1.m_factorNodes.size() == numFactors);
    assert(fg1.m_variableNodes.size() == numVars);
    assert(fg1.m_edges.size() == 20);

    // all factors nodes should be FGFactorNodes
    std::vector<FGFactorNodePtr> fnodes(F.size());
    for (const auto & fn: fg1.m_factorNodes)
      for (size_t iF = 0; iF < F.size(); ++iF)
        if (fn->nodeBdd == F[iF])
          fnodes[iF] = std::dynamic_pointer_cast<FGFactorNode>(fn);
    for (const auto & fnode: fnodes)
      assert(fnode);
    
    // f5 should have 3 edges to v5, v6, and v9
    {
      FGFactorNodePtr f5Node = fnodes[5];
      assert(f5Node->supportBdd == F[5].support());
      assert(f5Node->edges.size() == 3);
      std::set<BddWrapper> f5Neigh, expectedF5Neigh;
      for (const auto & e5: f5Node->edges)
        f5Neigh.insert(e5->getVariableNode()->nodeBdd);
      expectedF5Neigh.insert(V[5]);
      expectedF5Neigh.insert(V[6]);
      expectedF5Neigh.insert(V[9]);
      assert(f5Neigh == expectedF5Neigh);
    }

    // all variable nodes should be FGVariableNodes
    std::vector<FGVariableNodePtr> vnodes(V.size());
    for (const auto & vn: fg1.m_variableNodes)
      for (size_t iV = 0; iV < V.size(); ++iV)
        if (vn->nodeBdd == V[iV])
          vnodes[iV] = std::dynamic_pointer_cast<FGVariableNode>(vn);
    for (const auto & vnode: vnodes)
      assert(vnode);

    // v10 should have 1 edge to f6
    {
      FGNodePtr v10Node;
      for (const auto & vn: fg1.m_variableNodes)
        if (vn->nodeBdd == V[10]) v10Node = vn;
      assert(v10Node);
      assert(std::dynamic_pointer_cast<FGVariableNode>(v10Node));
      assert(v10Node->edges.size() == 1);
      assert((*v10Node->edges.begin())->getFactorNode()->nodeBdd == F[6]);
    }

    // manual function and variable message passing
    {
      std::set<FGNodePtr> updatedSet;
      fnodes[2]->passMessages(updatedSet);
      assert(updatedSet.size() == 2);
      for (const auto & en: fnodes[2]->edges)
        assert(en->factorToVariableMessage == project(F[2], en->getVariableNode()->nodeBdd));

      fnodes[4]->passMessages(updatedSet);
      fnodes[0]->passMessages(updatedSet);
      updatedSet.erase(updatedSet.begin(), updatedSet.end());
      vnodes[1]->passMessages(updatedSet);
      assert(updatedSet.size() == 3);
      for (const auto & en: vnodes[1]->edges)
        assert(en->variableToFactorMessage == -V[1]);
    }

    // convergence should give over approximations
    BddWrapper FAnd = F[0].one();
    for (const auto & factor: F)
      FAnd = FAnd * factor;
    fg1.converge();
    for (const auto & variable: V)
    {
      auto messages = fg1.getIncomingMessages(variable);
      auto messagesAnd = V[0].one();
      for (const auto & m: messages)
        messagesAnd = messagesAnd * m;
      auto FAndProjected = project(FAnd, variable);
      assert(-FAndProjected + messagesAnd == V[0].one()); // assert(FAndProjected => messagesAnd)
    }

    // convergence on acyclic graph should give exact answers
    {
      FactorGraphImpl fg2(F);
      BddWrapper v_6_9_11 = V[6].cubeUnion(V[9]).cubeUnion(V[11]);
      fg2.groupVariables(v_6_9_11);
      assert(fg2.m_factorNodes.size() == F.size());
      assert(fg2.m_variableNodes.size() == V.size() - 3 + 1);
      assert(fg2.m_edges.size() == fg1.m_edges.size() - 3);
      fg2.converge();
      std::vector<BddWrapper> groupedVars{V[0], V[1], V[2], V[3], V[4], V[5], v_6_9_11, V[7], V[8], V[10]};
      for (const auto & gv: groupedVars)
      {
        auto messages = fg2.getIncomingMessages(gv);
        auto messagesAnd = V[0].one();
        for (const auto & m: messages)
          messagesAnd = messagesAnd * m;
        auto FAndProjected = project(FAnd, gv);
        assert(messagesAnd == FAndProjected);
      }
    }


  }




} // end anonymous namespace
