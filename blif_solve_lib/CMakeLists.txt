cmake_minimum_required (VERSION 3.8)

add_library (blif_solve_lib SHARED
  "approx_merge.h" "blif_factors.h" "cnf_dump.h" "command_line_options.h"
  "log.h" "approx_merge.cpp" "blif_factors.cpp" "clo.cpp" "cnf_dump.cpp"
  "log.cpp" "clo.hpp" "var_elim_merge.cpp")

target_link_libraries (blif_solve_lib PUBLIC dd factor_graph)
