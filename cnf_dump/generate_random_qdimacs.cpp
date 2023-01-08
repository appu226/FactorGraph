/*

Copyright 2023 Parakram Majumdar

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

#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>



// struct to contain parse command line options
struct CommandLineOptions {
  int numVars;
  int numQuantifiedVars;
  int randomSeed;
  int numClauses;
  int avgClauseSize;
  std::string outputFile;
};



// class to manage random clause generation
class Randomizer {
  public:
    Randomizer(int seed, int numVars, int avgClauseSize);
    int generateVar();
    int generateLiteral();
    void generateClause(std::vector<int>& result);
  private:
    std::mt19937_64 m_engine;
    std::bernoulli_distribution m_varSign;
    std::uniform_int_distribution<int> m_var;
    std::poisson_distribution<int> m_numVarsInClause;
};



// class to create and manage output stream
class FileStreamParser {
  public:
    FileStreamParser(const std::string& filePath);
    std::ostream& operator*() { return m_ostream; }
  private:
    std::unique_ptr<std::ostream> m_fileStream;
    std::ostream& m_ostream;
};



// high level function declarations
CommandLineOptions parseClo(int argc, char const * const * const argv);
void writeHeader(const CommandLineOptions& clo, std::ostream& fout);
void writeClauses(const CommandLineOptions& clo, std::ostream& fout);
int main(int argc, char const * const * argv);




// main entry point
int main(int argc, char const * const * argv)
{
  auto clo = parseClo(argc, argv);
  FileStreamParser fout(clo.outputFile);  
  writeHeader(clo, *fout);
  writeClauses(clo, *fout);

  return 0;
}



// Randomizer functions
Randomizer::Randomizer(int seed, int numVars, int avgClauseSize) :
  m_engine(seed),
  m_varSign(),
  m_var(1, numVars),
  m_numVarsInClause(avgClauseSize - 1)
{ }
int Randomizer::generateVar() 
{ 
    return m_var(m_engine); 
}
int Randomizer::generateLiteral()
{
    return generateVar() * (m_varSign(m_engine) ? 1 : -1);
}
void Randomizer::generateClause(std::vector<int>& result)
{
    int numVarsInClause = m_numVarsInClause(m_engine) + 1;
    if (result.size() < numVarsInClause + 1)
        result.resize(numVarsInClause + 1, 0);
    result[numVarsInClause] = 0;
    for (int iv = 0; iv < numVarsInClause; ++iv)
        result[iv] = generateVar();
    return;
}



// Create FileStream
FileStreamParser::FileStreamParser(const std::string& filePath) :
  m_fileStream(filePath == "stdout" ? NULL : std::make_unique<std::ofstream>(filePath)),
  m_ostream(m_fileStream ? *m_fileStream : std::cout)
{ }



// write clauses into fout
void writeClauses(const CommandLineOptions& clo, std::ostream& fout)
{
  Randomizer r(clo.randomSeed, clo.numVars, clo.avgClauseSize);
  std::vector<int> clause;
  for (int iclause = 0; iclause < clo.numClauses; ++iclause)
  {
    r.generateClause(clause);
    for (int ilit = 0; ilit < clause.size(); ++ilit)
    {
      int lit = clause[ilit];
      fout << lit;
      if (lit == 0) {
        fout << '\n';
        ilit = clause.size() - 1;
      }
      else fout << ' ';
    }
  }
}




// write header into fout
void writeHeader(const CommandLineOptions& clo, std::ostream& fout)
{
    fout << "p cnf " << clo.numVars << ' ' << clo.numClauses << '\n';

    fout << "e ";
    for (int v = 1; v <= clo.numQuantifiedVars; ++v)
      fout << v << ' ';
    fout << "0\n";
}




// parse command line options
CommandLineOptions parseClo(int argc, char const * const * const argv)
{
  // set up the various options
  using blif_solve::CommandLineOption;
  auto numVars =
    std::make_shared<CommandLineOption<int> >(
        "--numVars",
        "total number of variables",
        false,
        50);
  auto numQuantifiedVars =
    std::make_shared<CommandLineOption<int> >(
      "--numQuantifiedVars",
      "number of variables to be quantified",
      false
    );
  auto randomSeed =
    std::make_shared<CommandLineOption<int> >(
        "--randomSeed",
        "Seed for pseudo random generator",
        false,
        0);
  auto verbosity =
    std::make_shared<CommandLineOption<std::string> >(
        "--verbosity",
        "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)",
        false,
        std::string("ERROR"));
  auto numClauses =
    std::make_shared<CommandLineOption<int> >(
        "--numClauses",
        "Number of clauses",
        false,
        30
    );
  auto avgClauseSize =
    std::make_shared<CommandLineOption<int> >(
        "--avgClauseSize",
        "Average clause size",
        false
    );
  auto outputFile =
    std::make_shared<CommandLineOption<std::string> >(
        "--outputFile",
        "Output qdimacs file to write results to",
        true,
        std::optional<std::string>()
    );
  
  // parse the command line
  blif_solve::parse(
      { numVars, numQuantifiedVars, randomSeed, verbosity, numClauses, avgClauseSize, outputFile },
      argc,
      argv);

  
  // set log verbosity
  blif_solve::setVerbosity(blif_solve::parseVerbosity(*(verbosity->value)));

  // return the rest of the options
  return CommandLineOptions{
    numVars->value.value(),
    numQuantifiedVars->value.value_or(numVars->value.value() / 2),
    randomSeed->value.value(),
    numClauses->value.value(),
    avgClauseSize->value.value_or(std::min(4, numVars->value.value())),
    outputFile->value.value()
  };
}
