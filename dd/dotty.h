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



#pragma once



#include "bdd_factory.h"



#include <string>
#include <map>
#include <iostream>



namespace dd {

  class Dotty {

    public:

      void addFactor(const BddWrapper & factor, bool addEdges);
      void setFactorLabel(const BddWrapper & factor, const std::string & label);
      void setFactorAttributes(const BddWrapper & factor, const std::string & attributes);

      void addVariable(const BddWrapper & variable, bool addEdges);
      void setVariableLabel(const BddWrapper & variable, const std::string & label);
      void setVariableAttributes(const BddWrapper & variable, const std::string & attributes);

      void addEdge(const BddWrapper & factor, const BddWrapper & variable);
      void setEdgeLabel(const BddWrapper & factor, const BddWrapper & variable, const std::string & label);
      void setEdgeAttributes(const BddWrapper & factor, const BddWrapper & variable, const std::string & attributes);

      void writeToDottyFile(std::ostream & stream) const;

   private:

      struct Properties {
        std::string label;
        std::string attributes;
      };

      std::map<BddWrapper, Properties> m_factors;
      std::map<BddWrapper, Properties> m_variables;
      std::map<std::pair<BddWrapper, BddWrapper>, Properties> m_edges;

      


  }; // end class Dotty

} // end namespace dd
