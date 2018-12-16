#pragma once

#include <bnet.h>
#include <dd.h>

#include <vector>
#include <string>
#include <memory>

namespace blif_solve {

  class BlifFactors {
    public:

      typedef std::shared_ptr<std::vector<bdd_ptr>> FactorVec;

      // ****** Constructor ******
      // parse the structures in a blif file
      // but don't create the bdd's yet
      BlifFactors(std::string const & fileName, DdManager * ddm);

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
      bdd_ptr getNonPiVars() const;

      // ****** Accessor function ******
      // returns the DdManager
      DdManager * getDdManager() const;


      static std::string primary_input_prefix;
      static std::string primary_output_prefix;
      static std::string latch_input_prefix;
      static std::string latch_output_prefix;

    private:
      BnetNetwork * m_network;
      DdManager * m_ddm;
      FactorVec m_factors;
      bdd_ptr m_piVars;
      bdd_ptr m_nonPiVars; 
  }; // end class BlifFactors

} // end namespace blif_solve
