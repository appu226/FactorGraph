IFLAGS_verilog_to_bdd = ${EXPORT_IFLAGS_dd}
LFLAGS_verilog_to_bdd = ${EXPORT_LFLAGS_dd}

all :                libVerilogToBdd.a


libVerilogToBdd.a : verilog_parser.o verilog_scanner.o verilog_to_bdd.o ${EXPORT_ARTIFACTS_dd}
			ar r libVerilogToBdd.a verilog_parser.o verilog_scanner.o verilog_to_bdd.o
			ranlib libVerilogToBdd.a


verilog_parser.o :           verilog_parser.cpp verilog_scanner.h verilog_parser.hpp verilog_to_bdd.h location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} verilog_parser.cpp -o verilog_parser.o
	   
verilog_scanner.o :          verilog_scanner.cpp verilog_to_bdd.h verilog_parser.hpp location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} verilog_scanner.cpp -o verilog_scanner.o
	   
verilog_to_bdd.o :      verilog_to_bdd.cpp verilog_to_bdd.h location.hh position.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} verilog_to_bdd.cpp -o verilog_to_bdd.o

verilog_scanner.cpp :        verilog_scanner.l
			flex -o verilog_scanner.cpp -d verilog_scanner.l

verilog_parser.hpp location.hh position.hh stack.hh :	verilog_parser.cpp
	
verilog_parser.cpp :				verilog_parser.y
			bison -o verilog_parser.cpp -d verilog_parser.y

tags :							*.h *.cpp *.hpp *.hh
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean:
			rm -rf verilog_scanner.cpp
			rm -rf verilog_parser.cpp verilog_parser.hpp location.hh position.hh stack.hh
			rm -rf *.o
			rm -rf libVerilogToBdd.a
