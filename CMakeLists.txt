cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)


set (PATH_cudd ${CMAKE_CURRENT_SOURCE_DIR}/../cudd-3.0.0)
set (PATH_kissat ${CMAKE_CURRENT_SOURCE_DIR}/../kissat)
link_directories (${PATH_cudd}/include ${PATH_cudd}/cudd/.libs ${PATH_kissat}/build)
include_directories (${CMAKE_SOURCE_DIR})


enable_testing ()

project ("FactorGraph")
add_subdirectory ("mustool")
add_subdirectory ("dd")
add_subdirectory ("factor_graph")
add_subdirectory ("factor_graph_main")
add_subdirectory ("qbf_solve")
add_subdirectory ("blif_solve_lib")
add_subdirectory ("blif_solve")
add_subdirectory ("cnf_dump")
add_subdirectory ("var_score")
add_subdirectory ("dec_21")
add_subdirectory ("may_22")
add_subdirectory ("oct_22")
add_subdirectory ("jan_24")
add_subdirectory ("test")
