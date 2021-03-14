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

#include <memory>

#include <dd/bdd_factory.h>

#include "var_score_graph_printer.h"

namespace var_score
{

  class VarScoreQuantification;

  class ApproximationMethod {
    public:
      typedef dd::BddWrapper BddWrapper;

      typedef std::shared_ptr<ApproximationMethod const> CPtr;

      static CPtr createExact();
      static CPtr createEarlyQuantification();
      static CPtr createFactorGraph(int largestSupportSet, GraphPrinter::CPtr const & graphPrinter);

      virtual void process(
          BddWrapper const & q, 
          BddWrapper const & f1, 
          BddWrapper const & f2, 
          var_score::VarScoreQuantification & vsq,
          DdManager * manager) const = 0;

      // unit test internal functionality
      static void runUnitTests(DdManager * manager);

      virtual ~ApproximationMethod() {}

  };


} // end namespace var_score
