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


// include corresponding header file
#include "cnf_dump_clo.h"



// std includes
#include <stdexcept>


namespace cnf_dump {


  //----------------------------------------------------------------
  // definitions of members of class CnfDumpClo
  //----------------------------------------------------------------
  
  // function CnfDumpClo::create
  std::shared_ptr<CnfDumpClo> CnfDumpClo::create(int argc, char const * const * const argv)
  {
    return std::make_shared<CnfDumpClo>(argc, argv);
  }


  // function CnfDumpClo::printHelpMessage
  //   prints help info
  void CnfDumpClo::printHelpMessage()
  {
    std::cout << "cnf_dump:\n"
              << "  Utility for converting a blif file into a cnf_dump\n"
              << "\n"
              << "Usage:\n"
              << "  cnf_dump <options>\n"
              << "\n"
              << "Options:\n"
              << "  --blif_file <input blif file> (MANDATORY)\n"
              << "  --cnf_file <output blif file> (MANDATORY)\n"
              << "  --num_lo_vars_to_quantify <number of latch output vars to quantify out>, defaults to 0\n"
              << "  --verbosity <verbosity> : one of QUIET/ERROR/WARNING/INFO/DEBUG, defaults to ERROR\n"
              << "  --help: prints this help message and exits\n"
              << std::endl;
    return;
  }


  // constructor CnfDumpClo::CnfDumpClo
  //   stub implementation
  CnfDumpClo::CnfDumpClo(int argc, char const * const * const argv) :
    blif_file(),
    output_cnf_file(),
    verbosity(blif_solve::ERROR),
    num_lo_vars_to_quantify(0),
    help(false)
  {
    char const * const * current_argv = argv + 1;
    for (int argnum = 1; argnum < argc; ++argnum, ++current_argv)
    {
      std::string current_arg(*current_argv);
      if (current_arg == "--blif_file")
      {
        ++argnum;
        ++current_argv;
        if (argnum >= argc)
          throw std::invalid_argument("Missing <input blif file> after argument --blif_file");
        blif_file = *current_argv;
      }
      else if (current_arg == "--cnf_file")
      {
        ++argnum;
        ++current_argv;
        if (argnum >= argc)
          throw std::invalid_argument("Missing <input cnf file> after argument --cnf_file");
        output_cnf_file = *current_argv;
      }
      else if (current_arg == "--verbosity")
      {
        ++argnum;
        ++current_argv;
        if (argnum >= argc)
          throw std::invalid_argument("Missing <verbosity> after argument --verbosity");
        verbosity = blif_solve::parseVerbosity(*current_argv);
      }
      else if (current_arg == "--help")
        help = true;
      else if (current_arg == "--num_lo_vars_to_quantify")
      {
        ++argnum;
        ++current_argv;
        if (argnum >= argc)
          throw std::invalid_argument("Missing <number> after argument --num_lo_vars_to_quantify");
        num_lo_vars_to_quantify = atoi(*current_argv);
      }

      else
        throw std::invalid_argument(std::string("Unexpected argument '") + *current_argv + "'");
    }
    if (help)
      return;
    if (blif_file == "")
      throw std::invalid_argument("Missing mandatory argument --blif_file");
    if (output_cnf_file == "")
      throw std::invalid_argument("Missing mandatory argument --cnf_file");

    return;
  }





} // end namespace cnf_dump
