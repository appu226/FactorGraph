cmake_minimum_required (VERSION 3.8)
add_library (var_score_lib
  "var_score_approximation.h" "var_score_graph_printer.h" "var_score_quantification.h"
  "var_score_approximation.cpp" "var_score_graph_printer.cpp" "var_score_quantification.cpp")
target_link_libraries (var_score_lib PUBLIC blif_solve_lib factor_graph dd)
add_executable (var_score "main.cpp")
target_link_libraries (var_score var_score_lib)

