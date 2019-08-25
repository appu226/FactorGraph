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
%define parser_class_name {Parser}
%define api.namespace {verilog_to_bdd}
%define api.prefix {vp}

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
  #include "verilog_types.h"
  #include <sstream>

  namespace verilog_to_bdd {

    class Scanner;
    class Interpreter;

  } // end namespace verilog_to_bdd
}


%code top
{
  #include "scanner.h"
  #include "parser.hpp"
  #include "interpreter.h"
  #include "location.hh"

static verilog_to_bdd::Parser::symbol_type 
  vplex(verilog_to_bdd::Scanner & scanner, 
        verilog_to_bdd::Interpreter & driver)
  {
    return scanner.get_next_token();
  }
}

%lex-param { verilog_to_bdd::Scanner & scanner }
%lex-param { verilog_to_bdd::Interpreter & driver }
%parse-param { verilog_to_bdd::Scanner & scanner }
%parse-param { verilog_to_bdd::Interpreter & driver }
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

%type <std::shared_ptr<std::vector<std::string> > > wire_name_list;
%type <std::shared_ptr<std::vector<std::shared_ptr<verilog_to_bdd::Assignment> > > > assignment_list;
%type <std::shared_ptr<verilog_to_bdd::Expression> > expression;

%start module

%%

module : MODULE IDENTIFIER LPAR wire_name_list RPAR SEMICOLON
                INPUT wire_name_list SEMICOLON
                OUTPUT wire_name_list SEMICOLON
                WIRE wire_name_list SEMICOLON
                assignment_list
         ENDMODULE {
                     auto module = std::make_shared<Module>();
                     module->name = $2;
                     module->interface = $4;
                     module->inputs = $8;
                     module->outputs = $11;
                     module->wires = $14;
                     module->assignments = $16;
                     driver.setModule(module);
                   }
         ;

wire_name_list : {
                   $$ = std::make_shared<std::vector<std::string> >();
                 }
               | IDENTIFIER
                 {
                   $$ = std::make_shared<std::vector<std::string> >(1, $1);
                 }
               | wire_name_list COMMA IDENTIFIER
                 {
                   $1->push_back($3);
                   $$ = $1;
                 }
               ;

assignment_list : {
                    $$ = std::make_shared<std::vector<verilog_to_bdd::AssignmentPtr> >();
                  }
                |  assignment_list ASSIGN IDENTIFIER EQUALS expression SEMICOLON
                   {
                     auto assignment = std::make_shared<Assignment>();
                     assignment->lhs = $3;
                     assignment->rhs = $5;
                     $1->push_back(assignment);
                     $$ = $1;
                   }
                ;

expression : IDENTIFIER
             {
               auto ret = std::make_shared<WireExpression>();
               ret->wireName = $1;
               $$ = std::static_pointer_cast<Expression>(ret);
             }
             | TILDA expression
               {
                 auto ret = std::make_shared<NegExpression>();
                 ret->underlying = $2;
                 $$ = std::static_pointer_cast<Expression>(ret);
               }
             | expression AND expression
               {
                 auto ret = std::make_shared<AndExpression>();
                 ret->lhs = $1;
                 ret->rhs = $3;
                 $$ = std::static_pointer_cast<Expression>(ret);
               }
             | expression OR expression
               {
                 auto ret = std::make_shared<OrExpression>();
                 ret->lhs = $1;
                 ret->rhs = $3;
                 $$ = std::static_pointer_cast<Expression>(ret);
               }
             ;

%%

void verilog_to_bdd::Parser::error(const location & loc, const std::string & message)
{
  std::stringstream ss;
  ss << "Error parsing verilog: " << message << "\nLocation: " << driver.getLocation() << std::endl;
  throw std::runtime_error(ss.str());
}




