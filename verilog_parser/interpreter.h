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

#include "scanner.h"
#include "verilog_types.h"
#include "parser.hpp"
#include "location.hh"

namespace verilog_parser {

 class Interpreter
 {
   public:
     Interpreter();

     int parse();

     void clear();

     void switchInputStream(std::istream * is, const std::string & filename);

     friend class Parser;
     friend class Scanner;

   public:
     void setModule(const std::shared_ptr<Module> & module);
     void columns(unsigned int offset);
     void lines(unsigned int offset);
     void step();
     void resetLocation();

     location getLocation() const;

   private:
     Scanner m_scanner;
     Parser m_parser;
     std::shared_ptr<Module> m_module;
     std::string m_filename;
     location m_location;
 };

} // end namespace verilog_parser
