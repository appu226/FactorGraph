/*

Copyright 2022 Parakram Majumdar

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

#include "oct_22_lib.h"

// main
int main(int argc, char const * const * const argv)
{

  // parse command line options
  auto clo = oct_22::parseClo(argc, argv);
  blif_solve_log(DEBUG, "Command line options parsed.");


  auto start = blif_solve::now();
  auto qdimacs = oct_22::parseQdimacs(clo.inputFile);                           // parse input file
  blif_solve_log(INFO, "Parsed qdimacs file in with " 
                        << qdimacs->clauses.size() << " clauses and " 
                        << qdimacs->numVariables << " variables in "
                        << blif_solve::duration(start) << " sec");

  start = blif_solve::now();
  auto ddm = oct_22::ddm_init();                                                // init cudd
  auto bdds = dd::QdimacsToBdd::createFromQdimacs(ddm.get(), *qdimacs); // create bdds
  blif_solve_log(INFO, "Created bdds in " << blif_solve::duration(start) << " sec");

  auto fg = oct_22::createFactorGraph(ddm.get(), *bdds, clo.largestSupportSet, clo.largestBddSize); // merge factors and create factor graph

  start = blif_solve::now();
  auto numIterations = fg->converge();                                  // converge factor graph
  blif_solve_log(INFO, "Factor graph converged after " 
      << numIterations << " iterations in "
      << blif_solve::duration(start) << " secs");

  start = blif_solve::now();                                            // factor graph result to CNF
  auto factorGraphResults = oct_22::getFactorGraphResults(ddm.get(), *fg, *bdds);
  auto factorGraphCnf = oct_22::convertToCnf(ddm.get(), bdds->numVariables + (2 * bdds->clauses.size()), factorGraphResults);
  blif_solve_log(INFO, "Factor graph result converted to cnf in "
      << blif_solve::duration(start) << " secs");

  if (clo.runMusTool)                                                     // run mustool
  {
    start = blif_solve::now();
    auto mustMaster = oct_22::createMustMaster(*qdimacs, factorGraphCnf);
    mustMaster->enumerate();
    blif_solve_log(INFO, "Must exploration finished in " << blif_solve::duration(start) << " sec");

    if (clo.outputFile.has_value())                                       // write results
      oct_22::writeResult(*factorGraphCnf, *qdimacs, clo.outputFile.value());
  }

  if (clo.computeExactUsingBdd)
  {
    auto exactResult = oct_22::computeExact(*bdds);
    auto fgMustResult = oct_22::cnfToBdd(*bdds, *factorGraphCnf);
    if (exactResult == fgMustResult) {
      blif_solve_log(INFO, "Factor Graph and/or Must result is EXACT.");
    } else {
      blif_solve_log(INFO, "Factor Graph and/or Must result is NOT EXACT.");
    }
    bdd_free(ddm.get(), fgMustResult);
    bdd_free(ddm.get(), exactResult);
  }
  blif_solve_log(INFO, "Done");
  return 0;
}
