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

%{

#include "interpreter.h"
#include "parser.hpp"
#include "location.hh"

#define vpterminate() verilog_parser::Parser::make_END(m_driver.getLocation());

#define YY_USER_ACTION { m_driver.step(); \
                         m_driver.columns(yyleng); \
                       }
                         


%}

%option nodefault
%option noyywrap
%option c++
%option yyclass="Scanner"
%option prefix="vp"

%%


module                 { return verilog_parser::Parser::make_MODULE(m_driver.getLocation()); }

endmodule              { return verilog_parser::Parser::make_ENDMODULE(m_driver.getLocation()); }

wire                   { return verilog_parser::Parser::make_WIRE(m_driver.getLocation()); }

input                  { return verilog_parser::Parser::make_INPUT(m_driver.getLocation()); }

output                 { return verilog_parser::Parser::make_OUTPUT(m_driver.getLocation()); }

assign                 { return verilog_parser::Parser::make_ASSIGN(m_driver.getLocation()); }

[a-zA-Z0-9_]+          { return verilog_parser::Parser::make_IDENTIFIER(yytext, m_driver.getLocation()); }

\(                     { return verilog_parser::Parser::make_LPAR(m_driver.getLocation()); }

\)                     { return verilog_parser::Parser::make_RPAR(m_driver.getLocation()); }

=                      { return verilog_parser::Parser::make_EQUALS(m_driver.getLocation()); }

~                      { return verilog_parser::Parser::make_TILDA(m_driver.getLocation()); }

\|                     { return verilog_parser::Parser::make_OR(m_driver.getLocation()); }

&                      { return verilog_parser::Parser::make_AND(m_driver.getLocation()); }

;                      { return verilog_parser::Parser::make_SEMICOLON(m_driver.getLocation()); }

,                      { return verilog_parser::Parser::make_COMMA(m_driver.getLocation()); }

[ \t]                  { /* ignore whitespace */ }

[\n]                   { m_driver.lines(1); }

\/\/[^\n]*\n           { m_driver.lines(1); }

.                      { m_driver.m_parser.error(m_driver.getLocation(), 
                                                 std::string("Unexpected token '") + yytext + "'"); 
                       }

<<EOF>>                { return vpterminate(); }

%%

