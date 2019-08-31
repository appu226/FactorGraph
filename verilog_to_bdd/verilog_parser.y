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

%skeleton "lalr1.cc" /* use C++  */
%defines             /* generate verilog_bison.h */
%define parser_class_name {VerilogParser}
%define api.namespace {verilog_to_bdd}
%define api.prefix {vp}

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
  #include <sstream>
  #include <dd.h>

  namespace verilog_to_bdd {

    class VerilogScanner;
    class VerilogToBdd;

  } // end namespace verilog_to_bdd

}


%code top
{
  #include "verilog_scanner.h"
  #include "verilog_parser.hpp"
  #include "verilog_to_bdd.h"
  #include "location.hh"

static verilog_to_bdd::VerilogParser::symbol_type 
  vplex(verilog_to_bdd::VerilogScanner & verilog_scanner, 
        verilog_to_bdd::VerilogToBdd & verilogToBdd)
  {
    return verilog_scanner.get_next_token();
  }
}

%lex-param { verilog_to_bdd::VerilogScanner & verilog_scanner }
%lex-param { verilog_to_bdd::VerilogToBdd & verilogToBdd }
%parse-param { verilog_to_bdd::VerilogScanner & verilog_scanner }
%parse-param { verilog_to_bdd::VerilogToBdd & verilogToBdd }
%locations
%define parse.trace
%define parse.error verbose


%token END 0 "end of file";
%token MODULE "module";
%token ENDMODULE "endmodule";
%token WIRE "wire";
%token INPUT "input";
%token OUTPUT "output";
%token ASSIGN "assign";
%token LPAR "(";
%token RPAR ")";
%token SEMICOLON ";";
%token COMMA ",";
%token <std::string> IDENTIFIER;
%token EQUALS "=";
%token TILDA "~";
%token OR "|";
%token AND "&";

%left OR AND
%left TILDA

%type <bdd_ptr> expression;

%start module

%%

module : MODULE IDENTIFIER LPAR wire_name_list RPAR SEMICOLON
                INPUT wire_name_list SEMICOLON
                OUTPUT wire_name_list SEMICOLON
                WIRE wire_name_list SEMICOLON
                assignment_list
         ENDMODULE { }
         ;

wire_name_list : { }
               | IDENTIFIER
                 { }
               | wire_name_list COMMA IDENTIFIER
                 { }
               ;

assignment_list : { }
                |  assignment_list ASSIGN IDENTIFIER EQUALS expression SEMICOLON
                   {
                     verilogToBdd.addBddPtr($3, $5);
                   }
                ;

expression : IDENTIFIER
             {
               $$ = verilogToBdd.getBddPtr($1);
             }
             | TILDA expression
               {
                 $$ = verilogToBdd.bddUnaryAndClear($2, &bdd_not);
               }
             | expression AND expression
               {
                 $$ = verilogToBdd.bddBinaryAndClear($1, $3, &bdd_and);
               }
             | expression OR expression
               {
                 $$ = verilogToBdd.bddBinaryAndClear($1, $3, &bdd_or);
               }
             ;

%%

void verilog_to_bdd::VerilogParser::error(const location & loc, const std::string & message)
{
  std::stringstream ss;
  ss << "Error parsing verilog: " << message << "\nLocation: " << verilogToBdd.getLocation() << std::endl;
  throw std::runtime_error(ss.str());
}




