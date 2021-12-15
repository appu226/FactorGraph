/*

Copyright 2021 Parakram Majumdar

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


#include <blif_solve_lib/clo.hpp>
#include <blif_solve_lib/log.h>
#include <memory>
#include <vector>
#include <string>




// struct to contain parse command line options
struct CommandLineOptions {
  int largestSupportSet;
  int maxMucSize;
  std::string inputFile;
};




// function declarations
CommandLineOptions parseClo(int argc, char const * const * const argv);
int main(int argc, char const * const * const argv);






// main
int main(int argc, char const * const * const argv)
{
  // parse command line options
  auto clo = parseClo(argc, argv);
  blif_solve_log(DEBUG, "Command line options parsed.");

  return 0;
}





// parse command line options
CommandLineOptions parseClo(int argc, char const * const * const argv)
{
  // set up the various options
  using blif_solve::CommandLineOption;
  auto largestSupportSet =
    std::make_shared<CommandLineOption<int> >(
        "--largestSupportSet",
        "largest allowed support set size while clumping cnf factors",
        false,
        50);
  auto maxMucSize =
    std::make_shared<CommandLineOption<int> >(
        "--maxMucSize",
        "max clauses allowed in an MUC",
        false,
        10);
  auto inputFile =
    std::make_shared<CommandLineOption<std::string> >(
        "--inputFile",
        "Input qdimacs file with exactly one quantifier which is existential",
        true);
  auto verbosity =
    std::make_shared<CommandLineOption<std::string> >(
        "--verbosity",
        "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)",
        false,
        std::string("ERROR"));
  
  
  // parse the command line
  blif_solve::parse(
      {  largestSupportSet, maxMucSize, inputFile, verbosity },
      argc,
      argv);

  
  // set log verbosity
  blif_solve::setVerbosity(blif_solve::parseVerbosity(*(verbosity->value)));

  // return the rest of the options
  return CommandLineOptions{
    *(largestSupportSet->value),
    *(maxMucSize->value),
    *(inputFile->value)
  };
}
