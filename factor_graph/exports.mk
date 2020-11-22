mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_factor_graph := ${mydir}/*.h ${mydir}/*.so
export EXPORT_IFLAGS_factor_graph := ${EXPORT_IFLAGS_dd} -I${mydir}
export EXPORT_LFLAGS_factor_graph := -L${mydir} -lFactorGraph ${EXPORT_LFLAGS_dd} -Wl,-rpath,${mydir}
