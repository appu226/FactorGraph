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

#pragma once

#include <vector>
#include <memory>
#include <istream>

namespace dd {




  // struct to represent a quantified list of variables 
  struct Quantifier {

    enum QuantifierType{ ForAll, Exists };
    
    QuantifierType quantifierType; // the quantifier
    std::vector<int> variables;    // the variables
    
    
    bool operator==(const Quantifier& that) const 
    { 
      return this->quantifierType == that.quantifierType 
        && this->variables == that.variables; 
    }
  };

  



  // struct representing a Qdimacs file
  struct Qdimacs {
    
    using Clause = std::vector<int>;

    int numVariables;                      // number of vars
    std::vector<Quantifier> quantifiers;   // quantified variables, from outer to inner
    std::vector<Clause> clauses;           // clauses


    // static function to parse a Qdimacs file
    static std::shared_ptr<Qdimacs> parseQdimacs(std::istream& is);

  }; // end struct Qdimacs



} // end namespace dd
