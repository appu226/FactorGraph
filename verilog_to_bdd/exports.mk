mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_verilog_to_bdd := ${mydir}/*.h ${mydir}/*.a
export EXPORT_IFLAGS_verilog_to_bdd := ${EXPORT_IFLAGS_dd} -I${mydir}
export EXPORT_LFLAGS_verilog_to_bdd := -L${mydir} -lVerilogToBdd ${EXPORT_LFLAGS_dd}
