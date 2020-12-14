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


#pragma once

#include <map>
#include <set>
#include <vector>
#include <optional>

#include <bdd_factory.h>

#include "var_score_approximation.h"



namespace var_score {


  ///////////////////////////////
  // Quantification algorithms //
  ///////////////////////////////

  // declare utility struct
  struct VarScoreQuantification {
    public:
      typedef dd::BddWrapper BddWrapper;
 
      static
        std::vector<BddWrapper>
        varScoreQuantification(const std::vector<BddWrapper> & F, 
                               const BddWrapper & Q, 
                               DdManager * ddm,
                               const int maxBddSize,
                               const ApproximationMethod::CPtr & approximationMethod);



      VarScoreQuantification(const std::vector<BddWrapper> & F, const BddWrapper & Q, DdManager * ddm);
      
      void addFactor(const BddWrapper & factor);
      void removeFactor(const BddWrapper & factor);
      void removeVar(const BddWrapper & var);
      
      BddWrapper varWithLowestScore() const;
      std::optional<BddWrapper> findVarWithOnlyOneFactor() const;
      std::pair<BddWrapper, BddWrapper> smallestTwoNeighbors(const BddWrapper & var) const;
      const std::set<BddWrapper> & neighboringFactors(const BddWrapper & var) const;
      
      bool isFinished() const;
      std::vector<BddWrapper> getFactorCopies() const;
      void printState() const;

    private:
      std::set<BddWrapper> m_factors;
      std::map<BddWrapper, std::set<BddWrapper> > m_vars;
      DdManager * m_ddm;

      bool isNeighbor(const BddWrapper & f, const BddWrapper & v) const;
  }; // end struct VarScoreQuantification


} // end nmespace var_score
