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


    // create quantifications
    for (const auto & qin: qdimacs.quantifiers)
    {
      BddWrapper quantifiedVariables = one; // start with True
      for (const auto vin: qin.variables)
        quantifiedVariables = quantifiedVariables.cubeUnion(result->getBdd(vin));
      result->quantifications.push_back(std::make_unique<BddQuantification>(BddQuantification{qin.quantifierType, quantifiedVariables.getCountedBdd()}));
    }



    // create factors
    for (const auto & clauseIn: qdimacs.clauses)
    {
      BddWrapper clauseOut = -one; // start with False
      for (auto vin: clauseIn)
        clauseOut = clauseOut + result->getBdd(vin);
      std::set<int> clauseKey(clauseIn.cbegin(), clauseIn.cend());
      result->clauses[clauseKey] = clauseOut.getCountedBdd();
    }


    // done
    return result;
  
  } // end QdimacsToBdd::createFromQdimacs


  BddWrapper
    QdimacsToBdd::getBdd(int v) const
  {
    BddWrapper result(bdd_new_var_with_index(ddManager, v > 0 ? v : -v), ddManager);
    if (v < 0) result = -result;
    return result;
  }


  BddWrapper
    QdimacsToBdd::getBdd(const std::set<int>& clause) const
  {
    auto cit = clauses.find(clause);
    if (cit != clauses.end())
      return BddWrapper(bdd_dup(cit->second), ddManager);
    BddWrapper result(bdd_zero(ddManager), ddManager);
    for (const auto l: clause)
    {
      result = result + getBdd(l);
    }
    return result;
  }




} // end namespace dd
