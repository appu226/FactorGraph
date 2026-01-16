/*

Copyright 2025 Parakram Majumdar

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

#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

#include "oct_22_lib.h"
#include "ave2.h"


namespace ave_main {
    struct CommandLineOptions {
        std::string inputFile;
        std::string outputFile;
        size_t maxClauseTreeSize;
        size_t timeoutSeconds;
        
        CommandLineOptions(int argc, char const * const * argv);
        std::string toString() const {
            return "{ inputFile: " + inputFile +
                   ", outputFile: " + outputFile +
                   ", maxClauseTreeSize: " + std::to_string(maxClauseTreeSize) +
                   ", timeoutSeconds: " + std::to_string(timeoutSeconds) + " }";
        }

        typedef std::unique_ptr<CommandLineOptions const> UCPtr;
        static UCPtr parse(int argc, char const * const * argv);
    };

    oct_22::Oct22MucCallback::CnfPtr toCnf(const oct_22::Ave2ClauseSet& aveClauses) {
        oct_22::Oct22MucCallback::CnfPtr cnf = std::make_shared<oct_22::Oct22MucCallback::Cnf>();
        for (const auto& aveClause: *aveClauses) {
            cnf->emplace(aveClause.literals);
        }
        return cnf;
    }
}

ave_main::CommandLineOptions::CommandLineOptions(int argc, char const * const * argv)
{
    // set up the various options
    using blif_solve::CommandLineOption;
    auto inputFile =
      std::make_shared<CommandLineOption<std::string> >(
          "--inputFile",
          "path to input qdimacs file",
          true);
    auto outputFile =
      std::make_shared<CommandLineOption<std::string> >(
          "--outputFile",
          "path to output cnf file",
          true);
    auto maxClauseTreeSize =
      std::make_shared<CommandLineOption<size_t> >(
          "--maxClauseTreeSize",
          "maximum clause tree size for approx var elim",
          false,
          static_cast<size_t>(5));
    auto timeoutSeconds =
      std::make_shared<CommandLineOption<size_t> >(
          "--timeoutSeconds",
          "maximum time in seconds for approx var elim (0 = no timeout)",
          false,
          static_cast<size_t>(0)
      );
    auto logVerbosity =
      std::make_shared<CommandLineOption<std::string> >(
          "--logVerbosity",
          "log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)",
          false,
          std::string("ERROR")
      );
    // parse the command line
    blif_solve::parse(
        {  inputFile, outputFile, maxClauseTreeSize, timeoutSeconds, logVerbosity },
        argc,
        argv);
    // set member variables
    this->inputFile = *(inputFile->value);
    this->outputFile = *(outputFile->value);
    this->maxClauseTreeSize = *(maxClauseTreeSize->value);
    this->timeoutSeconds = *(timeoutSeconds->value);
    blif_solve::setVerbosity(blif_solve::parseVerbosity(*(logVerbosity->value)));
}

ave_main::CommandLineOptions::UCPtr ave_main::CommandLineOptions::parse(int argc, char const * const * argv)
{
    return std::make_unique<CommandLineOptions const>(argc, argv);
}

int main(int argc, char const * const * const argv) {
    auto clo = ave_main::CommandLineOptions::parse(argc, argv);
    blif_solve_log(DEBUG, "avemain CLO parsed: " + clo->toString());

    auto qdimacs = oct_22::parseQdimacs(clo->inputFile);
    blif_solve_log(INFO, "Parsed qdimacs file with "
                        << qdimacs->clauses.size() << " clauses and "
                        << qdimacs->numVariables << " variables.");

    auto ave = oct_22::Ave2::parseQdimacs(*qdimacs);
    auto results = ave->approximatelyEliminateAllVariables(clo->maxClauseTreeSize, clo->timeoutSeconds);
    blif_solve_log(INFO, "Finished ave_main on " + clo->inputFile);

    oct_22::writeResult(*ave_main::toCnf(results), *qdimacs, clo->outputFile);
    blif_solve_log(INFO, "Wrote result with " + std::to_string(results->size()) + " clauses to " + clo->outputFile);
    return 0;
}
