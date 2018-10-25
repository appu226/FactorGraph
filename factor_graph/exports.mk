mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_factor_graph := ${mydir}/*.h ${mydir}/*.a
export EXPORT_IFLAGS_factor_graph := ${EXPORT_IFLAGS_cudd} -I${mydir}
export EXPORT_LFLAGS_factor_graph := ${EXPORT_LFLAGS_cudd} -L${mydir} -lFactorGraph
