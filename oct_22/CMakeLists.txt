cmake_minimum_required (VERSION 3.8)

add_executable (oct_22 "main.cpp")
target_link_libraries (oct_22 blif_solve_lib factor_graph dd mustool)
add_definitions(-DUMCSMUS -DNOSMT -DNOLTL)
