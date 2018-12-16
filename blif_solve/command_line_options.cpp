#include "command_line_options.h"

namespace blif_solve {

  // *** Constructor ***
  // Parses the command line arguments into a data structure
  // *******************
  CommandLineOptions::CommandLineOptions(int const argc, char const * const * const argv) :
    mustApplyFactorGraph(false),
    mustApplyCudd(false),
    verbosity(WARNING),
    mustDumpBdds(false),
    varNodeMergeLimit(0),
    blif_file_path()
  {

    if (argc < 1) usage("Blif file path not specified");

    for(int argi = 0; argi < argc; ++argi)
    {
      std::string arg(argv[argi]);
      if (arg == "--help")
        usage("");
      else if(arg == "--factor_graph")
        mustApplyFactorGraph = true;
      else if(arg == "--cudd")
        mustApplyCudd = true;
      else if(arg == "--verbosity")
      {
        ++argi;
        if (argi >= argc)
          usage("verbosity missing after --verbosity flag");
        std::string v(argv[argi]);
        if (v == "QUIET")
          verbosity = QUIET;
        else if(v == "ERROR")
          verbosity = ERROR;
        else if(v == "WARNING")
          verbosity = WARNING;
        else if(v == "INFO")
          verbosity = INFO;
        else if(v == "DEBUG")
          verbosity = DEBUG;
        else
          usage("Unknown verbosity '" + v + "', expecting one of QUIET/ERROR/WARNING/INFO/DEBUG");
      }
      else if(arg == "--dump_bdds")
      {
        mustDumpBdds = true;
      }
      else if(arg == "--var_node_merge_limit")
      {
        ++argi;
        if (argi >= argc)
          usage("var node merge limit missing after --var_node_merge_limit flag");
        varNodeMergeLimit = std::atoi(argv[argi]);
      }
      else blif_file_path = arg;

      if(blif_file_path.empty())
        usage("blif file path not provided");
    }
  }

  // *** Function ******
  // prints the usage information for the executable and exits
  // *******************
  void CommandLineOptions::usage(std::string const & error)
  {
    if (!error.empty())
      std::cerr << "Error: " << error << std::endl;
    std::cout << "blif_solve : Utility for solving a blif file using various methods\n"
              << "Usage:\n"
              << "\tblif_solve [--help] [--factor_graph] [--cudd] [--verbosity <verbosity>] \n"
              << "\t           [--var_node_merge_limit <number>] <blif file path>\n"
              << "\t\t--help          : print this usage information and exit\n"
              << "\t\t--factor_graph  : apply the factor_graph algorithm for existential quantification\n"
              << "\t\t--cudd          : use Cudd_bddExistAbstract for existential quantification\n"
              << "\t\t--dump_bdds     : whether to print bdds to stdout\n"
              << "\t\t--var_node_merge_limit : maximum number of variables to merge into a single node in the factor graph\n"
              << "\t\t                         default is 0 which means infinity\n"
              << "\t\t--verbosity v   : set verbosity level to v (one of QUIET/ERROR/WARNING/INFO/DEBUG)\n"
              << std::endl;
    exit(error.empty());
  }



} // end namespace blif_solve
