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

#include <dd/qdimacs.h>

#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <set>

int main(int argc, char const * const * const argv)
{
  // check cli usage
  if (argc != 4)
  {
    std::cout << "Program to take a qdimacs file and create two output files:\n"
      << "  - a qdimacs file with just one quantifier clause,\n"
      << "      which was the innermost existential quantification\n"
      << "      of the original problem"
      << "  - a cnf file, created by removing all the existentially \n"
      << "      quantified variables from the previous problem (along with\n"
      << "      any resulting empty clauses)\n"
      << "Usage:\n\t" << argv[0] << " <input qdimacs file> "
      << "<output qdimacs file> <output cnf file>"
      << std::endl;
    return 0;
  }




  // parse input file
  std::string inputFile = argv[1];
  std::shared_ptr<dd::Qdimacs> qdimacs;
  {
    std::ifstream inputReader(inputFile);
    qdimacs = dd::Qdimacs::parseQdimacs(inputReader);
  }
  



  // remove all except innermost quantifier
  if (qdimacs->quantifiers.empty())
    throw std::runtime_error("File '" + inputFile + "' did not have any quantifiers.");
  else if (qdimacs->quantifiers.back().quantifierType != dd::Quantifier::Exists)
    throw std::runtime_error("Innermost quantifier in file '" 
        + inputFile + "' is not an existential quantification.");
  else if (qdimacs->quantifiers.size() > 1)
    qdimacs->quantifiers = std::vector<dd::Quantifier>(1, qdimacs->quantifiers.back());

  // write qdimacs file
  {
    std::ofstream qos(argv[2]);
    qdimacs->print(qos);
    qos << std::endl;
  }




  // create a set of the quantified variables
  std::set<int> varsToRemove;
  for (auto v: qdimacs->quantifiers[0].variables)
  {
    varsToRemove.insert(v);
    varsToRemove.insert(-v);
  }
  // remove all collected vars from all clauses
  // and all empty clauses hence created
  std::vector<dd::Qdimacs::Clause> newClauses;
  for (const auto & clause: qdimacs->clauses)
  {
    dd::Qdimacs::Clause newClause;
    for (auto v: clause)
      if (varsToRemove.count(v) == 0)
        newClause.push_back(v);
    if (!newClause.empty())
      newClauses.push_back(newClause);
  }
  // complete the modification, and print
  qdimacs->quantifiers.clear();
  qdimacs->clauses = newClauses;
  {
    std::ofstream cnfos(argv[3]);
    qdimacs->print(cnfos);
    cnfos << std::endl;
  }
  
  
  
  return 0;
}
