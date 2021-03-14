cmake_minimum_required (VERSION 3.8)

add_library (factor_graph 
  "factor_graph.h" "hash_table.h" "qbf.h" "srt.h" "factor_graph.cpp"
  "hash_table.cpp" "qbf.cpp" "srt.cpp" "fgpp.h" "fgpp.cpp")

target_link_libraries (factor_graph PUBLIC dd)
