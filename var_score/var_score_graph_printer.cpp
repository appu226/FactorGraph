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

#include "var_score_graph_printer.h"

#include <dd/dotty.h>
#include <blif_solve_lib/log.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>


namespace {

  using namespace var_score;

  class GraphPrinterNoneImpl: public GraphPrinter
  {
    public:
      void generateGraphs(const dd::BddWrapper & Q,
                          const std::vector<dd::BddWrapper> & originalFactors,
                          const dd::BddVectorWrapper & newFactors,
                          const dd::BddVectorWrapper & groupedVariables
                         ) const override
      { }
  };





  class GraphPrinterFileDumpImpl: public GraphPrinter
  {

    public:
      GraphPrinterFileDumpImpl(const std::string & outputFilePrefix):
        m_outputFilePrefix(outputFilePrefix)
      { }

      void generateGraphs(const dd::BddWrapper & Q,
                          const std::vector<dd::BddWrapper>& originalFactors,
                          const dd::BddVectorWrapper & newFactors,
                          const dd::BddVectorWrapper & groupedVariables
                         ) const override
      {
        // prepare the file to write to
        std::string fullFileName = m_outputFilePrefix + "_" + getTimeStamp() + ".dot";
        std::ofstream fout(fullFileName);
        blif_solve_log(INFO, "Writing var_score graphs to " << fullFileName);
        writeFirstGraph(Q, originalFactors, fout);
        writeSecondGraph(Q, newFactors, groupedVariables, fout);
        fout.close();
      }
    private: 

      static std::string getTimeStamp()
      {
        std::stringstream ss;
        ss << std::chrono::system_clock::now().time_since_epoch().count();
        return ss.str();
      }

      static void writeFirstGraph(const dd::BddWrapper & Q,
                                  const std::vector<dd::BddWrapper> & F,
                                  std::ostream & out)
      {
        using namespace dd;
        Dotty dotty("OriginalGraph");
        std::set<BddWrapper> l2Neigh; // Q's level 2 neighbors
                                      // (neighbors of neighbors of Q)
        for (const auto & f: F)
        {
          dotty.addFactor(f, false);
          auto fsup = f.support();
          const BddWrapper one(bdd_one(f.getManager()), f.getManager());
          bool fIsQNeigh = (fsup.cubeIntersection(Q) != one);
          if (fIsQNeigh)
            dotty.setFactorAttributes(f, "color=blue");
          while (fsup != one)
          {
            BddWrapper v = fsup.varWithLowestIndex();
            fsup = fsup.cubeDiff(v);
            dotty.addVariable(v, false);
            dotty.addEdge(f, v);
            dotty.setEdgeAttributes(f, v, "color=grey");
            if (fIsQNeigh && v != Q)
              dotty.setVariableAttributes(v, "color=green");
          }
        }
        dotty.setVariableAttributes(Q, "color=red");
        dotty.writeToDottyFile(out);
        out << std::endl;
      }

      static void writeSecondGraph(const dd::BddWrapper & Q,
                                   const dd::BddVectorWrapper & factors,
                                   const dd::BddVectorWrapper & groupedVariables,
                                   std::ostream & out)
      {
        using namespace dd;
        BddWrapper one(bdd_one(Q.getManager()), Q.getManager());
        Dotty dotty("ModifiedGraph");

        std::set<BddWrapper> addedVars; // set of vars that have already been added to the graph

        // add Q
        addedVars.insert(Q);
        dotty.addVariable(Q, false);
        dotty.setVariableAttributes(Q, "color=red");

        // add all grouped vars
        for (size_t igv = 0; igv < groupedVariables->size(); ++igv)
        {
          BddWrapper gv = groupedVariables.get(igv);
          dotty.addVariable(gv, false);
          dotty.setVariableAttributes(gv, "color=purple");
          while (gv != one)
          {
            auto v = gv.varWithLowestIndex();
            gv = gv.cubeDiff(v);
            addedVars.insert(v);
          }
        }

        // add each factor, and any ungrouped vars
        for (size_t ifactor = 0; ifactor < factors->size(); ++ifactor)
        {
          auto factor = factors.get(ifactor);
          auto fsup = factor.support();
          bool isQNeigh = (fsup.cubeIntersection(Q) != one);
          while (fsup != one)
          {
            auto v = fsup.varWithLowestIndex();
            fsup = fsup.cubeDiff(v);
            if (!addedVars.count(v))
            {
              addedVars.insert(v);
              dotty.addVariable(v, false);
              if (isQNeigh)
                dotty.setVariableAttributes(v, "color=green");
            }
          }
          dotty.addFactor(factor, true);
          if (isQNeigh)
            dotty.setFactorAttributes(factor, "color=blue");
        }

        // write to file
        dotty.writeToDottyFile(out);
        out << std::endl;
      }



      std::string m_outputFilePrefix;

  };




} // end anonymous namespace



namespace var_score {

  GraphPrinter::CPtr GraphPrinter::noneImpl()
  {
    return std::make_shared<GraphPrinterNoneImpl>();
  }

  GraphPrinter::CPtr GraphPrinter::fileDumpImpl(const std::string & outputFilePrefix)
  {
    return std::make_shared<GraphPrinterFileDumpImpl>(outputFilePrefix);
  }


} // end namespace var_score
