cmake_minimum_required (VERSION 3.8)

add_executable (factor_graph_main "main.cpp")
target_link_libraries (factor_graph_main factor_graph dd)
