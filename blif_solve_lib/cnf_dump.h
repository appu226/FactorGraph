/*

Copyright 2019 Parakram Majumdar

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

#include <dd.h>
#include <string>

namespace blif_solve {

  // ------------------------- Function -----------------------------
  // dumpCnfForModelCounting:
  //   Creates a dimacs file in two parts (header and clauses)
  //     which can be fed to a model counter (in particular, ApproxMC)
  //     so that we can estimate the difference between the upper
  //     and the lower limits.
  //   The final result in the cnf file is 
  //     upperLimit && !lowerLimit
  //   This allows the model counter to count the number of
  //     satisfying assignments that are extra in upperLimit.
  //
  // Parameters:
  //   manager:    the cudd manager
  //   upperLimit: an OVER approximation of the 
  //               result of the quantification
  //   lowerLimit: an UNDER approximation of the
  //               result of the quantifiaction
  //   headerFile: the path to a file where the dimacs header
  //               should be written to
  //   clauseFile: the path to a file where the cnf clauses
  //               should be written to
  // ----------------------------------------------------------------
  void dumpCnfForModelCounting(DdManager * manager,
                               bdd_ptr_set const & allVars,
                               bdd_ptr_set const & upperLimit,
                               bdd_ptr_set const & lowerLimit,
                               std::string const & outputPath);

} // end namespace blif_solve
