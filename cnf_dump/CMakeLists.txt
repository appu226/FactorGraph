cmake_minimum_required (VERSION 3.8)

add_executable (cnf_dump "cnf_dump_clo.h" "cnf_dump_clo.cpp" "main.cpp")
target_link_libraries (cnf_dump dd factor_graph blif_solve_lib)

add_executable (simplify_qdimacs "simplify_qdimacs.cpp")
target_link_libraries(simplify_qdimacs dd)

add_executable(generate_random_qdimacs "generate_random_qdimacs.cpp")
target_link_libraries(generate_random_qdimacs dd factor_graph blif_solve_lib)
