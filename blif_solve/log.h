#pragma once

#include <iostream>

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

} // end namespace blif_solve

