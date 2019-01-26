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

#include <iostream>
#include <chrono>

namespace blif_solve {

  // Verbosity for logging
  enum Verbosity {
    QUIET,
    ERROR,
    WARNING,
    INFO,
    DEBUG
  };

  Verbosity getVerbosity();
  void setVerbosity(Verbosity verbosity);

#define blif_solve_log(verbosity, message) \
  if (blif_solve::getVerbosity() >= blif_solve::verbosity) \
  { \
    std::cout << "[" << #verbosity << "] " << message << std::endl; \
  }

#define blif_solve_log_bdd(verbosity, message, ddm, bdd) \
  if (blif_solve::getVerbosity() >= blif_solve::verbosity) \
  { \
    std::cout << "[" << #verbosity << "] " << message << std::endl; \
    bdd_print_minterms(ddm, bdd); \
  }

  // ****** Function *******
  // duration
  // takes a start and end chrono time
  // and returns the duration in seconds as a double
  // ***********************
  template<typename T> 
  double duration(T const & start)
  {
    return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
  }

  inline auto now()
  {
    return std::chrono::high_resolution_clock::now();
  }


} // end namespace blif_solve
