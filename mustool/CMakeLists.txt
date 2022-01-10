cmake_minimum_required (VERSION 3.8)

add_library (mustool 
"satSolvers/MSHandle.cpp" "satSolvers/mcsmus.cpp" "satSolvers/MSHandle.h"
"satSolvers/SatSolver.h" "satSolvers/SatSolver.cpp" "satSolvers/Dimacs.h"
"satSolvers/ParseUtils.h" "satSolvers/MucCallback.h" "algorithms/tome.cpp"
"algorithms/marco.cpp" "algorithms/remus.cpp" "core/misc.h"
"core/Master.h" "core/misc.cpp" "core/types_h.h" "core/Master.cpp"
"core/Explorer.h" "core/Explorer.cpp" "custom_minisat" "custom_minisat/System.cc"
"custom_minisat/Solver.cc" "custom_minisat/System.h" "custom_minisat/Options.h"
"custom_minisat/Alg.h" "custom_minisat/Alloc.h" "custom_minisat/Dimacs.h"
"custom_minisat/Heap.h" "custom_minisat/IntMap.h" "custom_minisat/IntTypes.h"
"custom_minisat/Map.h" "custom_minisat/ParseUtils.h" "custom_minisat/Queue.h"
"custom_minisat/Rnd.h" "custom_minisat/SolverTypes.h" "custom_minisat/Sort.h"
"custom_minisat/Vec.h" "custom_minisat/XAlloc.h"
"custom_minisat/Options.cc" "custom_minisat/Solver.h" "mcsmus/glucose/simp/SimpSolver.cc"
"mcsmus/glucose/simp/SimpSolver.h" "mcsmus/glucose/core/Solver.cc"
"mcsmus/glucose/core/SolverTypes.h" "mcsmus/glucose/core/BoundedQueue.h"
"mcsmus/glucose/core/Constants.h" "mcsmus/glucose/core/Solver.h"
"mcsmus/minisat/mtl/mcsmus_IntMap.h" "mcsmus/minisat/mtl/mcsmus_Alg.h"
"mcsmus/minisat/mtl/mcsmus_Heap.h" "mcsmus/minisat/mtl/mcsmus_IntTypes.h"
"mcsmus/minisat/mtl/mcsmus_XAlloc.h" "mcsmus/minisat/mtl/mcsmus_Sort.h"
"mcsmus/minisat/mtl/mcsmus_Map.h" "mcsmus/minisat/mtl/mcsmus_Vec.h"
"mcsmus/minisat/mtl/mcsmus_Alloc.h" "mcsmus/minisat/mtl/mcsmus_Queue.h"
"mcsmus/minisat/mtl/mcsmus_Rnd.h" "mcsmus/minisat/simp/SimpSolver.cc"
"mcsmus/minisat/simp/mcsmus_SimpSolver.h" "mcsmus/minisat/utils/System.cc"
"mcsmus/minisat/utils/mcsmus_Options.h" "mcsmus/minisat/utils/mcsmus_System.h"
"mcsmus/minisat/utils/mcsmus_ParseUtils.h" "mcsmus/minisat/utils/Options.cc"
"mcsmus/minisat/core/Solver.cc" "mcsmus/minisat/core/mcsmus_Solver.h"
"mcsmus/minisat/core/mcsmus_SolverTypes.h" "mcsmus/mcsmus/dimacs.hh"
"mcsmus/mcsmus/control.cc" "mcsmus/mcsmus/options.hh" "mcsmus/mcsmus/basesolver.hh"
"mcsmus/mcsmus/control.hh" "mcsmus/mcsmus/options.cc" "mcsmus/mcsmus/Main.cc"
"mcsmus/mcsmus/glucose-wrapper.cc" "mcsmus/mcsmus/glucose-wrapper.hh"
"mcsmus/mcsmus/debug.hh" "mcsmus/mcsmus/minisat-wrapper.cc"
"mcsmus/mcsmus/lingeling-solver.cc" "mcsmus/mcsmus/minisat-wrapper.hh"
"mcsmus/mcsmus/algorithm.hh" "mcsmus/mcsmus/minisolver.hh" "mcsmus/mcsmus/parseutils.hh"
"mcsmus/mcsmus/system.hh" "mcsmus/mcsmus/lingeling-solver.hh" "mcsmus/mcsmus/mcsmus.cc"
"mcsmus/mcsmus/mcsmus.hh")
  
add_definitions(-DUMCSMUS -DNOSMT -DNOLTL -DNDEBUG)

target_link_libraries (mustool PUBLIC z stdc++fs) 
