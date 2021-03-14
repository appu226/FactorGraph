cmake_minimum_required (VERSION 3.8)

add_executable (test1
  "test.cpp" "testApproxMerge.cpp" "testVarScoreQuantification.cpp" "testApproxMerge.cpp")
target_link_libraries (test1 var_score_lib blif_solve_lib factor_graph dd)
add_test (NAME test1 COMMAND test1)

add_executable (cnf_dump_test
  "cnf_dump_test.cpp" "random_bdd_generator.cpp")
target_link_libraries (cnf_dump_test var_score_lib blif_solve_lib factor_graph dd)

add_executable (clipping_and_abstract_test
  "clipping_and_abstract_test.cpp" "random_bdd_generator.cpp")
target_link_libraries(clipping_and_abstract_test blif_solve_lib factor_graph dd)
