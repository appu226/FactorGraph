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
#include <string>
#include <memory>


// blif_solve_lib includes
#include <log.h>
#include <blif_factors.h>
#include <cnf_dump.h>



// cnf_dump includes
#include "cnf_dump_clo.h"



int main(int argc, char const * const * const argv)
{
  try 
  {

  auto clo = cnf_dump::CnfDumpClo::create(argc, argv);
  blif_solve::setVerbosity(clo->verbosity);

  // if --help flag is given, print help and exit
  if (clo->help)
  {
    cnf_dump::CnfDumpClo::printHelpMessage();
    return 0;
  }


  // parse blif bdds
  blif_solve_log(INFO, "Parsing blif bdds");
  DdManager *ddm = Cudd_Init(0, 0, 256, 262144, 0);
  if (ddm == NULL)
    throw std::runtime_error("Could not initialize DdManager.");
  auto blif_factors = std::make_shared<blif_solve::BlifFactors> (clo->blif_file, ddm);
  blif_factors->createBdds();

  // dump cnf
  blif_solve_log(INFO, "Creating cnf files");
  auto npv_vec = blif_factors->getNonPiVars();
  auto factor_vec = blif_factors->getFactors();
  bdd_ptr_set non_pi_vars(npv_vec->cbegin(), npv_vec->cend());
  bdd_ptr_set factors(factor_vec->cbegin(), factor_vec->cend());
  bdd_ptr_set lower_limit;
  lower_limit.insert(bdd_zero(ddm));
  blif_solve::dumpCnfForModelCounting(blif_factors->getDdManager(),
                                      non_pi_vars,
                                      factors,
                                      lower_limit,
                                      clo->output_cnf_file);


  blif_solve_log(INFO, "Done");
 
  }
  catch (const std::exception & e)
  {
    std::cerr << "ERROR: "
              << e.what() 
              << std::endl;;
    return 1;
  }

  return 0;
}



