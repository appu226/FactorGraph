cmake_minimum_required (VERSION 3.8)

add_library (dd SHARED
  "bdd_factory.h" "bdd_partition.h" "bnet.h" "cuddAndAbsMulti.h" "dd.h" "disjoint_set.h"
  "dotty.h" "lru_cache.h" "max_heap.h" "ntr.h" "optional.h" "bnet.c" "ntr.c" "ntrHeap.c"
  "ntrMflow.c" "bdd_factory.cpp" "bdd_partition.cpp" "cuddAndAbsMulti.cpp" "dd.cpp"
  "dotty.cpp" "qdimacs.h" "qdimacs.cpp" "qdimacs_to_bdd.h" "qdimacs_to_bdd.cpp")
target_include_directories (dd PUBLIC 
  ${PATH_cudd}/include ${PATH_cudd}/util ${PATH_cudd}/cudd ${PATH_cudd}/mtr ${PATH_cudd}/epd
  ${PATH_cudd}/st ${PATH_cudd}/dddmp ${PATH_cudd})
target_link_libraries (dd PUBLIC cudd)
