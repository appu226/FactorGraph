
#pragma once

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

#define blif_log(verbosity, message) \
  if (blif_sove::getVerbosity() >= verbosity) \
  { \
    std::cout << "[" ## verbosity ## "] " << message << std::endl; \
  }

} // end namespace blif_solve

