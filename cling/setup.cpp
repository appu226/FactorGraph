/*

Copyright 2024 Parakram Majumdar

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


#pragma cling add_include_path("../cudd-3.0.0/include")
#pragma cling add_include_path("../cudd-3.0.0/util")
#pragma cling add_include_path("../cudd-3.0.0/cudd")
#pragma cling add_include_path("../cudd-3.0.0/mtr")
#pragma cling add_include_path("../cudd-3.0.0/epd")
#pragma cling add_include_path("../cudd-3.0.0/st")
#pragma cling add_include_path("../cudd-3.0.0/dddmp")
#pragma cling add_include_path("../cudd-3.0.0");
#pragma cling add_library_path("../cudd-3.0.0/include")
#pragma cling add_library_path("../cudd-3.0.0/cudd/.libs")
#pragma cling load("libcudd.so")

#pragma cling add_include_path("dd")
#pragma cling add_library_path("dd")
#pragma cling load("libDd.so")

#pragma cling add_include_path("factor_graph")
#pragma cling add_library_path("factor_graph")
#pragma cling load("libFactorGraph.so")

#pragma cling add_include_path("blif_solve_lib")
#pragma cling add_library_path("blif_solve_lib")
#pragma cling load("libBlifSolveLib.so")

#pragma cling add_include_path("var_score")
#pragma cling add_library_path("var_score")
#pragma cling load("libVarScore.so")
