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


// namespace cnf_dump
//   contains all functionality for this application
namespace cnf_dump {


  // class CommandLineOptions
  //   structure to parse all the command line options
  class CnfDumpClo {
    public:

      // factory method to parse the command line options
      static std::shared_ptr<CnfDumpClo> create(int argc, char const * const * const argv);


      std::string blif_file;           // the input blif file path
      std::string output_cnf_file;     // the output cnf file path
      blif_solve::Verbosity verbosity; // log verbosity
      int num_lo_vars_to_quantify;
      bool help;                       // whether the help flag was mentioned or not

      static 
        void printHelpMessage();       // print the help message
      
    
    public:
      // constructor to parse command line options
      CnfDumpClo(int argc, char const * const * const argv);
  }; // end of class CommandLineOptions


  
  
} // end namespace cnf_dump


