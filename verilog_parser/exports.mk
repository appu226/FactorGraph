mydir = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
export EXPORT_ARTIFACTS_verilog_parser := ${mydir}/*.h ${mydir}/*.a
export EXPORT_IFLAGS_verilog_parser := ${EXPORT_IFLAGS_dd} -I${mydir}
export EXPORT_LFLAGS_verilog_parser := -L${mydir} -lVerilogParser ${EXPORT_LFLAGS_dd}
