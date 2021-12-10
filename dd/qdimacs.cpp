/*

Copyright 2021 Parakram Majumdar

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

#include "qdimacs.h"

#include <fstream>
#include <sstream>
#include <stdexcept>


namespace {

  inline bool startsWith(const std::string& s1, const char * s2, int len) {
    return s1.compare(0, len, s2, len) == 0;
  }

} // end anonymous namespace

namespace dd {


    std::shared_ptr<Qdimacs> Qdimacs::parseQdimacs(std::istream& is)
    {
      auto q = std::make_shared<Qdimacs>();

      std::string line;
      int numClauses;
      while (getline(is, line))
      {
        // skip comments and empty lines
        if (line.empty() || line[0] == 'c')
          continue;
        else if (startsWith(line, "p cnf ", 6))
        {
          sscanf(line.c_str(), "p cnf %d %d", &(q->numVariables), &numClauses);
        }
        else if (line[0] == 'a' || line[0] == 'e')
        {
          // add empty quantifier clause
          q->quantifiers.emplace_back();
          auto & quantifier = q->quantifiers.back();

          // parse quantifier type
          char c;
          std::stringstream lss(line);
          lss >> c;
          if (c == 'a')
            quantifier.quantifierType = Quantifier::ForAll;
          else if (c == 'e')
            quantifier.quantifierType = Quantifier::Exists;
          else
            throw std::invalid_argument(std::string("Unexpected quantifier. Expected: 'a'/'e', Actual: '") + c + "'");
          
          // parse vars into the quantifier clause until 0 var
          int var;
          lss >> var;
          while(var != 0) {
            quantifier.variables.push_back(var);
            lss >> var;
          }
        }
        else
        {
          // add empty clause
          q->clauses.emplace_back();
          auto & clause = q->clauses.back();

          // parse vars from line to clause until 0 var
          int var;
          std::stringstream lss(line);
          lss >> var;
          while (var != 0)
          {
            clause.push_back(var);
            lss >> var;
          }
        }
      }

      return q;
    }




    void Qdimacs::print(std::ostream& os) const
    {
      os << "p cnf " << numVariables << ' '<< clauses.size() << "\n";
      for (const auto & quantifier: quantifiers)
      {
        os << (quantifier.quantifierType == Quantifier::ForAll ? 'a' : 'e');
        for (auto v: quantifier.variables)
          os << ' ' << v;
        os << " 0\n";
      }
      for (const auto & clause: clauses)
      {
        for (auto v: clause)
          os << v << ' ';
        os << "0\n";
      }
    }


} // end namespace dd
