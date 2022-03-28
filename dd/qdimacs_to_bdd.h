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

#include "qdimacs.h"
#include "dd.h"
#include "bdd_factory.h"

#include <vector>
#include <set>
#include <map>
#include <memory>


namespace dd {


  // quantification clause in bdd form
  struct BddQuantification {
    Quantifier::QuantifierType quantifierType;
    bdd_ptr quantifiedVariables;
    bool operator == (const BddQuantification& that) const { 
      return quantifierType == that.quantifierType 
        && quantifiedVariables == that.quantifiedVariables; 
    }
  };
  typedef std::unique_ptr<BddQuantification> BddQuantificationUPtr;




  // qdimacs file in bdd form
  struct QdimacsToBdd {


    int numVariables;                                    // number of variables
    std::vector<BddQuantificationUPtr> quantifications;  // quantification clauses ordered from outer to inner
    std::map<std::set<int>, bdd_ptr> clauses;            // cnf factors, mapped from original int-set clause to bdd clause
    DdManager* ddManager;                                // the bdd manager

    typedef std::shared_ptr<QdimacsToBdd> Ptr;

    static
      Ptr
      createFromQdimacs(DdManager* ddManager, const Qdimacs& qdimacs); // static constructor function

    BddWrapper getBdd(int v) const;
    BddWrapper getBdd(const std::set<int>& clause) const;
    ~QdimacsToBdd(); // destructor, calls free on all bdds stored in the structure
  };


} // end namespace dd
