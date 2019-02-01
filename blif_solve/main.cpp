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




// std includes
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// factor_graph includes
#include <dd.h>
#include <ntr.h>
#include <srt.h>
#include <cuddInt.h>

// blif_solve includes
#include <cnf_dump.h>
#include "command_line_options.h"
#include "blif_factors.h"
#include "blif_solve_method.h"


// constants
std::string const primary_input_prefix = "pi";
std::string const primary_output_prefix = "po";
std::string const latch_input_prefix = "li";
std::string const latch_output_prefix = "lo";
std::string const quantification_answer_prefix = "ans_qpi";



// function declarations
int                             main                 (int argc, char ** argv);
blif_solve::BlifSolveMethodCptr createBlifSolveMethod(std::string const & bsmStr,
                                                      blif_solve::CommandLineOptions const & clo);
int                             getNumSolutions      (DdManager * ddm,
                                                      bdd_ptr_set const & bdd,
                                                      int numVars);


using blif_solve::now;
using blif_solve::duration;



// *** Function ****
// main function:
//   parses inputs
//   reads blif file
//   creates bdds if either cudd or factor_graph needs to be applied
//   applies cudd to compute transition relation if required
//   applies factor_graph to compute transition relation if required
//   cleans up memory before exiting
//   logs useful information
// *****************
int main(int argc, char ** argv)
{
 
  // parse inputs
  auto clo = std::make_shared<blif_solve::CommandLineOptions>(argc, argv);
  blif_solve::setVerbosity(clo->verbosity);

  try {
  
    
    
    // init cudd
    auto srt = std::make_shared<SRT>();
    
   

    // parse network
    auto start = now();
    auto blifFactors = std::make_shared<blif_solve::BlifFactors>(clo->blif_file_path, srt->ddm);
    blif_solve_log(DEBUG, "parsed blif file in " << duration(start) << " sec");
    start = now();
    blifFactors->createBdds();
    int const numPiVars = Cudd_SupportSize(blifFactors->getDdManager(), blifFactors->getPiVars());
    int const numNonPiVars = blifFactors->getNonPiVars()->size();
    blif_solve_log(INFO, "created " << blifFactors->getFactors()->size() << " factors with "
                          << (numPiVars + numNonPiVars) << " vars ("
                          << numPiVars << " primary inputs + " << numNonPiVars << " others) in "
                          << duration(start) << " sec");



    // compute upper limit
    bdd_ptr_set upperLimit;
    if (clo->overApproximatingMethod != "Skip")
    {
      blif_solve_log(INFO, "Executing over approximating method " << clo->overApproximatingMethod);
      start = now();
      auto upperMethod = createBlifSolveMethod(clo->overApproximatingMethod, *clo);
      upperLimit = upperMethod->solve(*blifFactors);
      blif_solve_log(INFO, "Finised over approximating method " 
                           << clo->overApproximatingMethod << " in " 
                           << duration(start) << " sec");
      if(clo->mustCountSolutions)
        blif_solve_log(INFO, "Over approximating method " << clo->overApproximatingMethod
                             << " finished with " << getNumSolutions(srt->ddm, upperLimit, numNonPiVars)
                             << " solutions.");
    }



    // compute lower limit
    bdd_ptr_set lowerLimit;
    if (clo->underApproximatingMethod != "Skip")
    {
      blif_solve_log(INFO, "Executing under approximating method " << clo->underApproximatingMethod);
      start = now();
      auto lowerMethod = createBlifSolveMethod(clo->underApproximatingMethod, *clo);
      lowerLimit = lowerMethod->solve(*blifFactors);
      blif_solve_log(INFO, "Finished under approximating method " << clo->underApproximatingMethod
                           << " in " << duration(start) << " sec");
      if (clo->mustCountSolutions)
        blif_solve_log(INFO, "Under approximating method " << clo->underApproximatingMethod
                             << " finished with " << getNumSolutions(srt->ddm, lowerLimit, numNonPiVars)
                             << " solutions.");
    }



    // dump the diff
    if (upperLimit.size() > 0 && lowerLimit.size() > 0)
    {
      blif_solve_log(DEBUG, "Writing diff to " << clo->diffOutputPath);
      auto nonPiVars = blifFactors->getNonPiVars();
      bdd_ptr_set allVars(nonPiVars->cbegin(), nonPiVars->cend());
      blif_solve::dumpCnfForModelCounting(blifFactors->getDdManager(),
                                          allVars,
                                          upperLimit,
                                          lowerLimit,
                                          clo->diffOutputPath);
    }



    // free bdds
    for (auto uli = upperLimit.cbegin(); uli != upperLimit.cend(); ++uli)
      bdd_free(blifFactors->getDdManager(), *uli);
    for (auto lli = lowerLimit.cbegin(); lli != lowerLimit.cend(); ++lli)
      bdd_free(blifFactors->getDdManager(), *lli);
   
  } catch (std::exception const & e)
  {
    if (clo->verbosity >= blif_solve::ERROR)
      std::cerr << "Fatal error: " << e.what() << std::endl;
    exit(1);
  }


  blif_solve_log(INFO, "SUCCESS " << clo->blif_file_path.substr(clo->blif_file_path.find_last_of("/") + 1));

  return 0;
}



// ***** Function *****
// BlifSolveMethod :: createBlifSolveMethod
// Takes a string and creates an impl of the BlifSolveMethod class
// ********************
blif_solve::BlifSolveMethodCptr createBlifSolveMethod(std::string const & bsmStr, blif_solve::CommandLineOptions const & clo)
{
  if ("ExactAndAccumulate" == bsmStr)
    return blif_solve::BlifSolveMethod::createExactAndAccumulate();
  else if ("ExactAndAbstractMulti" == bsmStr)
    return blif_solve::BlifSolveMethod::createExactAndAbstractMulti();
  else if ("FactorGraphApprox" == bsmStr)
    return blif_solve::BlifSolveMethod::createFactorGraphApprox(
        clo.varNodeSize,
        clo.funcNodeSize,
        clo.seed,
        clo.numConvergence,
        clo.dotDumpPath);
  else if ("AcyclicViaForAll" == bsmStr)
    return blif_solve::BlifSolveMethod::createAcyclicViaForAll();
  else if ("True" == bsmStr)
    return blif_solve::BlifSolveMethod::createTrue();
  else if ("False" == bsmStr)
    return blif_solve::BlifSolveMethod::createFalse();
  else if ("ClippingOverApprox" == bsmStr)
    return blif_solve::BlifSolveMethod::createClippingAndAbstract(clo.clippingDepth, true);
  else if ("ClippingUnderApprox" == bsmStr)
    return blif_solve::BlifSolveMethod::createClippingAndAbstract(clo.clippingDepth, false);
  else if (bsmStr == "FactorGraphExact")
    throw std::runtime_error("BlifSolveMethod for '" + bsmStr + "' not yet implemented.");
  else
    throw std::runtime_error("Invalid BlifSolveMethod '" + bsmStr + "', "
        "expecting one of ExactAndAccumulate/ExactAndAbstractMulti/"
        "FactorGraphApprox/FactorGraphExact/AcyclicViaForAll/Skip");
}


int getNumSolutions(DdManager * manager, bdd_ptr_set const & bdds, int numVars)
{
  auto conj = bdd_and_multi(manager, bdds);
  auto numSln = bdd_count_minterm(manager, conj, numVars);
  bdd_free(manager, conj);
  return numSln;
}
