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



#pragma once

#include <dd/bnet.h>
#include <dd/dd.h>

#include <vector>
#include <string>
#include <memory>

namespace blif_solve {

  class BlifFactors {
    public:

      typedef std::shared_ptr<std::vector<bdd_ptr>> FactorVec;
      typedef std::vector<std::shared_ptr<BlifFactors>> PtrVec;

      // ****** Constructor ******
      // parse the structures in a blif file
      // but don't create the bdd's yet
      BlifFactors(std::string const & fileName, int numNumLoVarsToQuantify, DdManager * ddm);

      // ****** Destructor ******
      // clean up
      ~BlifFactors();

      // ****** Function ******
      // create the bdds in the file
      // for each li circuit, create a new variable representing li,
      //   and store (li <-> circuit) as a factor
      // collect the pi variables
      // collect the non-pi variables (li, lo)
      // Note: this function is a pre-requisite for some of the
      //   accessor methods in this class.
      void createBdds();

      // ****** Function ******
      // partitions this problem into many disconnected
      //   sub-problems
      PtrVec partitionFactors() const;

      // ****** Accessor Function ******
      // return the factors described in the circuit
      // Pre-requisite: createBdds() must be called
      //   otherwise, this function will throw
      FactorVec getFactors() const;

      // ****** Accessor function ******
      // return a cube consisting of the primary input variables
      // Pre-requisite: createBdds() must be called
      //   otherwise, this function will throw
      bdd_ptr getPiVars() const;

      // ****** Accessor function ******
      // return a cube consisting of the latch input output variables
      // Pre-requisite: createBdds() must be called
      //   otherwise, this function will throw
      FactorVec getNonPiVars() const;

      // ****** Accessor function ******
      // returns the DdManager
      DdManager * getDdManager() const;


    private:
      BlifFactors(BnetNetwork * network,
                  DdManager * ddm,
                  FactorVec factors,
                  bdd_ptr piVars,
                  FactorVec nonPiVars);


      BnetNetwork * m_network;
      DdManager * m_ddm;
      FactorVec m_factors;
      bdd_ptr m_piVars;
      FactorVec m_nonPiVars;
      int m_numLoVarsToQuantify;
  }; // end class BlifFactors

} // end namespace blif_solve
