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

#include <string>
#include <iostream>
#include <log.h>

namespace blif_solve {
  

  // command line options
  struct CommandLineOptions
  {

    // overApproximatingMethod
    std::string overApproximatingMethod;
    // underApproximatingMethod
    std::string underApproximatingMethod;
    // verbosity for logging
    Verbosity verbosity;
    // path to dump the diff cnf and header files
    std::string diffOutputPath;
    // maximum number of variable nodes to merge into a single node in the factor graph
    int varNodeSize;
    // maximum number of function nodes to merge into a single node in the factor graph
    int funcNodeSize;
    // seed for randomized message passing
    int seed;
    // number of convergences to perform
    int numConvergence;
    // maximum depth to use while clipping
    int clippingDepth;

    // path to dump dot files (for factor graph visualization)
    std::string dotDumpPath;

    // whether to count and print the number of solutions
    bool mustCountSolutions;

    std::string blif_file_path;

    // constructor to parse the command line options
    CommandLineOptions(int argc, char const * const * const argv);

    // print usage information and exit
    static void usage(std::string const & error);
  };

  
} // end namespace blif_solve
