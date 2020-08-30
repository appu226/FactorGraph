/*

Copyright 2020 Parakram Majumdar

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



#include <log.h>
#include <command_line_options.h>
#include <blif_factors.h>
#include <srt.h>

#include <memory>



//////////////////
// declarations //
//////////////////

// structure and functions to parse and store command line options
struct VarScoreCommandLineOptions
{
  std::string verbosity;
  std::string blif;
  int largestSupportSet;
};
template<typename TValue>
std::shared_ptr<blif_solve::CommandLineOptionValue<TValue> > 
  addCommandLineOption(std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > & commandLineOptions,
                       const std::string & name,
                       const std::string & help,
                       const TValue & defaultValue);
VarScoreCommandLineOptions parseClo(int argc, char const * const * const argv);

// main
int main(int argc, char const * const * const argv);

// quantification algorithms
std::vector<bdd_ptr>
  varScoreQuantification(const std::vector<bdd_ptr> & F, 
                         bdd_ptr Q, 
                         int largestSupportSet, 
                         DdManager * ddm);











///////////////////
// main function //
///////////////////
int main(int argc, char const * const * const argv)
{
  auto clo = parseClo(argc, argv); // parse the command line options
  auto srt = std::make_shared<SRT>(); // initialize BDD
  
  // parse the blif file
  blif_solve::BlifFactors bf(clo.blif, 0, srt->ddm);
  bf.createBdds();
  auto subProblems = bf.partitionFactors();
  blif_solve_log(INFO, "Parsed " << subProblems.size() << " sub problems");

  // solve each sub problem
  for (auto sp: subProblems)
  {
    auto resultVec = varScoreQuantification(*(sp->getFactors()), sp->getPiVars(), clo.largestSupportSet, sp->getDdManager());
    blif_solve_log(INFO, "Computed sub result");
    for (auto result: resultVec)
      blif_solve_log_bdd(DEBUG, "result", sp->getDdManager(), result);
  }

  return 0;
}









///////////////////////////////////////////////////////
// functions to parse and store command line options //
///////////////////////////////////////////////////////
template<typename TValue>
std::shared_ptr<blif_solve::CommandLineOptionValue<TValue> >
  addCommandLineOption(std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > & commandLineOptions,
                       const std::string & name,
                       const std::string & help,
                       const TValue & defaultValue)
{
  auto result = blif_solve::CommandLineOptionValue<TValue>::create(name, help, defaultValue);
  commandLineOptions.push_back(result);
  return result;
}

//------------------------------------
VarScoreCommandLineOptions
  parseClo(int argc, char const * const * const argv)
{
  VarScoreCommandLineOptions result;
  std::vector<std::shared_ptr<blif_solve::ICommandLineOption> > clo;
  auto verbosity = addCommandLineOption<std::string>(clo, "--verbosity", "verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)", "WARNING");
  auto blif = addCommandLineOption<std::string>(clo, "--blif", "blif file name", "");
  auto largestSupportSet = addCommandLineOption<int>(clo, "--largest_support_set", "limit on the bdd size", 100);


  parseCommandLineOptions(argc - 1, argv + 1, clo);
  blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity->getValue()));
  if (blif->getValue() == "")
  {
    blif_solve_log(ERROR, "Expecting value for command line argument --blif");
    blif_solve::printHelp(clo);
    exit(1);
  }
  blif_solve_log(DEBUG, "blif file: " + blif->getValue());
  blif_solve_log(DEBUG, "largest support set: " << largestSupportSet->getValue());
  result.verbosity = verbosity->getValue();
  result.blif = blif->getValue();
  result.largestSupportSet = largestSupportSet->getValue();
  return result;
}









///////////////////////////////
// Quantification algorithms //
///////////////////////////////

// declare utility structs, functions
struct VarScoreQuantification {
  public:

#define NYI(fname) { throw std::runtime_error(#fname " not yet implemented"); }

    VarScoreQuantification(const std::vector<bdd_ptr> & F, bdd_ptr Q, int largestSupportSet, DdManager * ddm) NYI(VarScoreQuantification)
    bdd_ptr findVarWithOnlyOneFactor() NYI(findVarWithOnlyOneFactor)
    std::vector<bdd_ptr> neighboringFactors(bdd_ptr var) NYI(neighboringFactors)
    void removeFactor(bdd_ptr factor) NYI(removeFactor)
    void addFactor(bdd_ptr factor) NYI(addFactor)
    void removeVar(bdd_ptr var) NYI(removeVar)
    bdd_ptr varWithLowestScore() NYI(varWithLowestScore)
    std::pair<bdd_ptr, bdd_ptr> smallestTwoNeighbors(bdd_ptr var) NYI(smallestTwoNeighbors)
    bool isFinished() NYI(isFinished)
    std::vector<bdd_ptr> getFactorCopies() const NYI(getFactorCopies)
    ~VarScoreQuantification() { blif_solve_log(ERROR, "Destructor ~VarScoreQuantification() not yet implemented"); }
  private:
    std::vector<bdd_ptr> m_factors;
    std::map<bdd_ptr, std::vector<bdd_ptr> > m_vars;
    int m_largestSupportSet;
    DdManager * m_ddm;
};


std::vector<bdd_ptr> varScoreQuantification(const std::vector<bdd_ptr> & F, bdd_ptr Q, int largestSupportSet, DdManager * ddm)
{
  VarScoreQuantification vsq(F, Q, largestSupportSet, ddm);
  while(!vsq.isFinished())
  {
    auto q = vsq.findVarWithOnlyOneFactor();
    if (q != NULL)
    {
      auto tv = vsq.neighboringFactors(q);
      assert(tv.size() == 1);
      auto t = tv.front();
      auto t_without_q = bdd_forsome(ddm, t, q);
      vsq.addFactor(t_without_q);
      bdd_free(ddm, t_without_q);
      vsq.removeFactor(t);
      vsq.removeVar(q);
    }
    else
    {
      auto q = vsq.varWithLowestScore();
      auto t1t2 = vsq.smallestTwoNeighbors(q);
      auto t1 = t1t2.first;
      auto t2 = t1t2.second;
      if (vsq.neighboringFactors(q).size() == 2)
      {
        auto t = bdd_and_exists(ddm, t1, t2, q);
        vsq.addFactor(t);
        bdd_free(ddm, t);
        vsq.removeFactor(t1);
        vsq.removeFactor(t2);
        vsq.removeVar(q);
      }
      else
      {
        auto t = bdd_and(ddm, t1, t2);
        vsq.addFactor(t);
        bdd_free(ddm, t);
        vsq.removeFactor(t1);
        vsq.removeFactor(t2);
      }
    }
  }
  return vsq.getFactorCopies();
}












