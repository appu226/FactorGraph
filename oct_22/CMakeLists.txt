cmake_minimum_required (VERSION 3.8)

add_library(oct_22_lib "oct_22_lib.h" "approx_var_elim.h" "ave2.h" "oct_22_lib.cpp" "approx_var_elim.cpp" "ave2.cpp")
target_link_libraries (oct_22_lib blif_solve_lib factor_graph dd mustool)
add_executable (oct_22 "main.cpp")
target_link_libraries (oct_22 oct_22_lib blif_solve_lib factor_graph dd mustool)
# Add new executable for approx_var_elim runner
add_executable (ave_main "ave_main.cpp")
target_link_libraries (ave_main oct_22_lib blif_solve_lib factor_graph dd mustool)
add_definitions(-DUMCSMUS -DNOSMT -DNOLTL)
