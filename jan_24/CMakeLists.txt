cmake_minimum_required (VERSION 3.8)

add_executable (innermost_existential "cnf_sax_parser.h" "cnf_sax_parser.cpp" "innermost_existential.cpp")
target_link_libraries (innermost_existential blif_solve_lib)
