mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_var_score := ${mydir}/*.h ${mydir}/*.a
export EXPORT_IFLAGS_var_score := ${EXPORT_IFLAGS_blif_solve_lib} -I${mydir}
export EXPORT_LFLAGS_var_score := -L${mydir} -lVarScore ${EXPORT_LFLAGS_blif_solve_lib}
