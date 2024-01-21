cmake_minimum_required (VERSION 3.8)

add_library (cnf_sax_parser SHARED "cnf_sax_parser.h" "cnf_sax_parser.cpp")

add_library (check_progress SHARED "compare_bfss_input_and_kissat_output.cpp")
target_link_libraries(check_progress blif_solve_lib cnf_sax_parser)

add_executable (innermost_existential "innermost_existential.cpp")
target_link_libraries (innermost_existential blif_solve_lib cnf_sax_parser)

add_executable (remove_unaries "remove_unaries.cpp")
target_link_libraries (remove_unaries blif_solve_lib cnf_sax_parser)

add_executable (kissat_preprocess "kissat_preprocess.cpp")
target_include_directories (kissat_preprocess PUBLIC ${PATH_kissat}/..)
target_link_libraries (kissat_preprocess cnf_sax_parser blif_solve_lib kissat)