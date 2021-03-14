cmake_minimum_required (VERSION 3.8)

add_executable (cnf_dump "cnf_dump_clo.h" "cnf_dump_clo.cpp" "main.cpp")

target_link_libraries (cnf_dump dd factor_graph blif_solve_lib)
