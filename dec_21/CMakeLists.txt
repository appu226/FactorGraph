cmake_minimum_required (VERSION 3.8)

add_executable (dec_21 "main.cpp")
target_link_libraries (dec_21 blif_solve_lib factor_graph dd mustool)
add_definitions(-DUMCSMUS -DNOSMT -DNOLTL -DNDEBUG)
