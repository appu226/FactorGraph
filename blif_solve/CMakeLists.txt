cmake_minimum_required (VERSION 3.8)

add_executable (blif_solve
  "blif_solve_method.h" "command_line_options.h" "blif_solve_method.cpp"
  "command_line_options.cpp" "main.cpp")
target_link_libraries (blif_solve blif_solve_lib factor_graph dd)
