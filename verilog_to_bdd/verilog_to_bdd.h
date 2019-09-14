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

#include "verilog_scanner.h"
#include "verilog_parser.hpp"
#include "location.hh"

#include <string>
#include <map>
#include <memory>
#include <dd.h>

namespace verilog_to_bdd {

  /** class BddVarMap
   * A class to represent a map from variable names to their bdds
   */
  class BddVarMap
  {
    public:
      // constructor
      BddVarMap(DdManager * manager);

      // destructor : calls bdd_free on all stored bdds
      ~BddVarMap();

      // getter : calls bdd_dup before returing result
      //          throws if varName not found
      bdd_ptr getBddPtr(const std::string & varName) const;
      
      // setter : stores the input bdd without calling bdd_dup
      //          throws if varName already exists
      void addBddPtr(const std::string & varName, bdd_ptr varBdd);

      // checker : checks whether varName is contained 
      //           in the var map
      bool containsBddPtr(const std::string & varName) const;
    
    private:
      DdManager * m_manager;
      std::map<std::string, bdd_ptr> m_data;
      
      // disable copying
      BddVarMap(const BddVarMap & that);
      BddVarMap & operator=(const BddVarMap & that);
  };
  typedef std::shared_ptr<BddVarMap> BddVarMapPtr;


  /** class VerilogToBdd
   * A class to encapsulate the functionality for
   *   parsing a verilog file containing skolem functions
   *   and substituting those functions into the main function.
   * See: http://mathworld.wolfram.com/SkolemFunction.html
   */
  class VerilogToBdd
  {
    // The main public api
    public:

      /** function VerilogToBdd::parse
       * A static function to encapsulate the class creation, stateful parsing, 
       *   and extraction of parsed structure.
       * input args:
       *   is: the input stream containing the text to be parsed
       *   filename: a filename used only for reporting errors during parsing
       *   vars: a BddVarMap mapping variable names to their bdd's.
       *         The input variables must already be set in the BddVarMap.
       *         The output variables will be set by the parse function.
       *         (Note that the argument is a 
       *           const ref, to a shared pointer, of a non-const BddVarMap.
       *           And hence, the contents of the BddVarMap can be modified.)
       *   manager: the cudd manager.
       * outputs:
       *   No explicit outputs, 
       *   but the resulting newly created variables
       *   will be present in the input argument "vars".
       */
      static
        void
        parse(std::istream * is, 
              const std::string & filename, 
              const BddVarMapPtr & vars,
              DdManager * manager);


    // private stateful api used only in the top level static functions
    private:
      VerilogToBdd(std::istream * is,
                   const std::string & filename, 
                   const BddVarMapPtr & vars,
                   DdManager * manager);
      void parse();

    // private api used by friend classes during parsing
    private:
      friend class VerilogParser;
      friend class VerilogScanner;
      void setModule(const ModulePtr & module);
      void columns(unsigned int offset);
      void lines(unsigned int offset);
      void step();
      void resetLocation();
      location getLocation() const;


    // private member variables
    private:
      VerilogScanner m_verilog_scanner;
      VerilogParser m_verilog_parser;
      std::string m_filename;
      location m_location;
      ModulePtr m_module;
      BddVarMapPtr m_vars;
      DdManager * m_manager;
  };

} // end namespace verilog_to_bdd
