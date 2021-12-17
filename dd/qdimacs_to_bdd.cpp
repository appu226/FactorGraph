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

#include "qdimacs_to_bdd.h"
#include "bdd_factory.h"


namespace dd {


  // destructor
  QdimacsToBdd::~QdimacsToBdd()
  {
    for (auto & q: quantifications)
      bdd_free(ddManager, q->quantifiedVariables);
    for (auto & kv: clauses)
      bdd_free(ddManager, kv.second);
  }




  // constructor
  std::shared_ptr<QdimacsToBdd>
    QdimacsToBdd::createFromQdimacs(
        DdManager* ddManager,
        const Qdimacs& qdimacs)
  {

    auto result = std::make_shared<QdimacsToBdd>();
    result->ddManager = ddManager;


    // create variable bdds for easy access
    result->numVariables = qdimacs.numVariables;
    BddWrapper one(bdd_one(ddManager), ddManager);
    BddVectorWrapper vars({one.getCountedBdd()}, ddManager); // first element is just one
    for (int i = 1; i <= qdimacs.numVariables; ++i)
      vars.push_back({ bdd_new_var_with_index(ddManager, i), ddManager}); // add variable of bdd index i to vector index i



    // create quantifications
    for (const auto & qin: qdimacs.quantifiers)
    {
      BddWrapper quantifiedVariables = one; // start with True
      for (const auto vin: qin.variables)
        quantifiedVariables = quantifiedVariables.cubeUnion(vars.get(vin));
      result->quantifications.push_back(std::make_unique<BddQuantification>(BddQuantification{qin.quantifierType, quantifiedVariables.getCountedBdd()}));
    }



    // create factors
    for (const auto & clauseIn: qdimacs.clauses)
    {
      BddWrapper clauseOut = -one; // start with False
      for (auto vin: clauseIn)
        clauseOut = clauseOut + (vin > 0 ? vars.get(vin) : -vars.get(-vin));
      std::set<int> clauseKey(clauseIn.cbegin(), clauseIn.cend());
      result->clauses[clauseKey] = clauseOut.getCountedBdd();
    }


    // done
    return result;
  
  } // end QdimacsToBdd::createFromQdimacs




} // end namespace dd
