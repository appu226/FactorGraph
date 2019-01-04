mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_blif_solve_lib := ${mydir}/*.h ${mydir}/*.a
export EXPORT_IFLAGS_blif_solve_lib := ${EXPORT_IFLAGS_factor_graph} -I${mydir}
export EXPORT_LFLAGS_blif_solve_lib := -L${mydir} -lBlifSolveLib ${EXPORT_LFLAGS_factor_graph}
