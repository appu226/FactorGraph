#include "log.h"

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

} // end namespace blif_solve
