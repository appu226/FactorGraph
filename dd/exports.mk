mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_dd := ${mydir}/*.h ${mydir}/*.so
export EXPORT_IFLAGS_dd := ${EXPORT_IFLAGS_cudd} -I${mydir}
export EXPORT_LFLAGS_dd := -L${mydir} -lDd ${EXPORT_LFLAGS_cudd} -Wl,-rpath,${mydir}
