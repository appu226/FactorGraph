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



#include "log.h"
#include <stdexcept>

namespace {
  blif_solve::Verbosity g_verbosity = blif_solve::INFO;
} // end anonymous namespace

namespace blif_solve {

  Verbosity getVerbosity()
  {
    return g_verbosity;
  }

  void setVerbosity(Verbosity v)
  {
    g_verbosity = v;
  }

  Verbosity parseVerbosity(const std::string & verbosity_string)
  {
    if (verbosity_string == "QUIET")
      return QUIET;
    else if (verbosity_string == "ERROR")
      return ERROR;
    else if (verbosity_string == "WARNING")
      return WARNING;
    else if (verbosity_string == "INFO")
      return INFO;
    else if (verbosity_string == "DEBUG")
      return DEBUG;
    else
      throw std::invalid_argument("Unexpected verbosity string '"
                                  + verbosity_string
                                  + "', must be one of QUIET/ERROR/WARNING/INFO/DEBUG");
  }

} // end namespace blif_solve
