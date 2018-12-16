#pragma once

#include <string>
#include <iostream>
#include "log.h"

namespace blif_solve {
  

  // command line options
  struct CommandLineOptions
  {
    // whether factor graph algorithm should be applied or not
    bool mustApplyFactorGraph;
    // whether Cudd_ExistsAbstract should be applied or not
    bool mustApplyCudd;
    // verbosity for logging
    Verbosity verbosity;
    // path to dump bdds
    bool mustDumpBdds;
    // maximum number of variable nodes to merge into a single node in the factor graph
    int varNodeMergeLimit;

    std::string blif_file_path;

    // constructor to parse the command line options
    CommandLineOptions(int argc, char const * const * const argv);

    // print usage information and exit
    static void usage(std::string const & error);
  };

  
} // end namespace blif_solve
