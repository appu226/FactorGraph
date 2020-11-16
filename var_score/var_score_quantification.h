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

#include <dd.h>



namespace var_score {

  ///////////////////////////////
  // Quantification algorithms //
  ///////////////////////////////

  // declare utility struct
  struct VarScoreQuantification {
    public:


      VarScoreQuantification(const std::vector<bdd_ptr> & F, bdd_ptr Q, int largestSupportSet, DdManager * ddm);
      ~VarScoreQuantification();
      
      void addFactor(bdd_ptr factor);
      void removeFactor(bdd_ptr factor);
      void removeVar(bdd_ptr var);
      
      bdd_ptr varWithLowestScore() const;
      bdd_ptr findVarWithOnlyOneFactor() const;
      std::pair<bdd_ptr, bdd_ptr> smallestTwoNeighbors(bdd_ptr var) const;
      const std::set<bdd_ptr> & neighboringFactors(bdd_ptr var) const;
      
      bool isFinished() const;
      std::vector<bdd_ptr> getFactorCopies() const;

      void printState() const;

      static
        std::vector<bdd_ptr>
        varScoreQuantification(const std::vector<bdd_ptr> & F, 
                               bdd_ptr Q, 
                               int largestSupportSet, 
                               DdManager * ddm);

    private:
      std::set<bdd_ptr> m_factors;
      std::map<bdd_ptr, std::set<bdd_ptr> > m_vars;
      int m_largestSupportSet;
      DdManager * m_ddm;

      bool isNeighbor(bdd_ptr f, bdd_ptr v) const;
  }; // end struct VarScoreQuantification


} // end nmespace var_score
