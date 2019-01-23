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

#include "blif_factors.h"
#include "command_line_options.h"
#include <memory>

namespace blif_solve {

  class BlifSolveMethod
  {
    public:
      typedef std::shared_ptr<BlifSolveMethod const> Cptr;

      virtual bdd_ptr_set solve(BlifFactors const & blifFactors) const = 0;

      static Cptr createExactAndAccumulate();
      static Cptr createExactAndAbstractMulti();
      static Cptr createFactorGraphApprox(int varNodeSize,
                                          int funcNodeSize,
                                          int seed,
                                          int numConvergence,
                                          std::string const & dotDumpPath);
      static Cptr createAcyclicViaForAll();
      static Cptr createTrue();
      static Cptr createFalse();

  }; // end class BlifSolveMethod

  typedef BlifSolveMethod::Cptr BlifSolveMethodCptr;

} // end namespace blif_solve
