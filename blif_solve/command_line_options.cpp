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



#include "command_line_options.h"

namespace blif_solve {

  // *** Constructor ***
  // Parses the command line arguments into a data structure
  // *******************
  CommandLineOptions::CommandLineOptions(int const argc, char const * const * const argv) :
    overApproximatingMethod("FactorGraphApprox"),
    underApproximatingMethod("AcyclicViaForAll"),
    verbosity(WARNING),
    diffOutputPath(),
    varNodeSize(0),
    funcNodeSize(1),
    seed(0),
    numConvergence(1),
    dotDumpPath(),
    blif_file_path()
  {

    if (argc < 1) usage("Blif file path not specified");

    for(int argi = 0; argi < argc; ++argi)
    {
      std::string arg(argv[argi]);
      if (arg == "--help")
        usage("");
      else if(arg == "--over_approximating_method")
      {
        ++argi;
        if (argi >= argc)
          usage("Solve method not specified after --overApproximatingMethod flag");
        overApproximatingMethod = argv[argi];
      }
      else if(arg == "--under_approximating_method")
      {
        ++argi;
        if (argi >= argc)
          usage("Solve method nto specified after --underApproximatingMethod flag");
        underApproximatingMethod = argv[argi];
      }
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
      else if(arg == "--diff_output_path")
      {
        ++argi;
        if (argi >= argc)
          usage("Output path not specified after --diff_output_path flag");
        diffOutputPath= argv[argi];
      }
      else if(arg == "--var_node_size")
      {
        ++argi;
        if (argi >= argc)
          usage("var node size missing after --var_node_size flag");
        varNodeSize = std::atoi(argv[argi]);
      }
      else if(arg == "--func_node_size")
      {
        ++argi;
        if (argi >= argc)
          usage("func node size missing after --func_node_size flag");
        funcNodeSize = std::atoi(argv[argi]);
      }
      else if (arg == "--seed")
      {
        ++argi;
        if (argi >= argc)
          usage("numeric seed missing after --seed flag");
        seed = std::atoi(argv[argi]);
      }
      else if (arg == "--num_convergence")
      {
        ++argi;
        if (argi >= argc)
          usage("number of convergences missing after --num_convergence flag");
        numConvergence = std::atoi(argv[argi]);
      }
      else if(arg == "--dot_dump_path")
      {
        ++argi;
        if (argi >= argc)
          usage("dot dump path missing after --dot_dump_path flag");
        dotDumpPath = argv[argi];
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
    std::cout << "blif_solve : Utility for solving a blif file using two methods, and compute the diff of the answers\n"
              << "Usage:\n"
              << "\tblif_solve [--help] [--over_approximating_method <solveMethod>] \n"
              << "\t           [--under_approximating_method <solveMethod>] [--verbosity <verbosity>] \n"
              << "\t           [--var_node_merge_limit <number>] <blif file path>\n"
              << "\t\t--help                       : print this usage information and exit\n"
              << "\t\t--over_approximating_method  : method to compute the upper limit\n"
              << "\t\t--under_approximating_method : method to compute the lower limit\n"
              << "\t\t--diff_output_path           : path to dump the diff bdd\n"
              << "\t\t                                 (upper_limit and not(lower_limit)) \n"
              << "\t\t                               in dimacs files (header and clauses separate)\n"
              << "\t\t--var_node_size              : maximum number of variables to merge into a single node \n"
              << "\t\t                               in the factor graph; default is 0 which means infinity\n"
              << "\t\t--func_node_sze              : maximum number of functions to merge into a single node \n"
              << "\t\t                               in the factor graph; default is 1 which means no merging\n"
              << "\t\t--seed                       : seed to use for randomized merging of var and func nodes\n"
              << "\t\t--num_convergence            : number of times to run message passing algorithm\n"
              << "\t\t--verbosity v                : set verbosity level to v;\n"
              << "\t\t                               must be one of QUIET/ERROR/WARNING/INFO/DEBUG\n"
              << "\t\t--dot_dump_path ddp          : path to dump dot files (for factor graph visualization\n"
              << "\tAvailable solve methods: ExactAndAccumulate/ExactAndAbstractMulti/FactorGraphApprox/\n"
              << "\t                         FactorGraphExact/AcyclicViaForAll/True/False"
              << std::endl;
    exit(error.empty());
  }




} // end namespace blif_solve
