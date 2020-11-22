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
