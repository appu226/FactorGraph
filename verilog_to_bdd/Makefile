IFLAGS_verilog_to_bdd = ${EXPORT_IFLAGS_dd}
LFLAGS_verilog_to_bdd = ${EXPORT_LFLAGS_dd}

all :                libVerilogToBdd.a


libVerilogToBdd.a : parser.o scanner.o interpreter.o ${EXPORT_ARTIFACTS_dd}
			ar r libVerilogToBdd.a parser.o scanner.o interpreter.o
			ranlib libVerilogToBdd.a


parser.o :           parser.cpp verilog_types.h scanner.h parser.hpp interpreter.h location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} parser.cpp -o parser.o
	   
scanner.o :          scanner.cpp interpreter.h parser.hpp location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} scanner.cpp -o scanner.o
	   
interpreter.o :      interpreter.cpp interpreter.h verilog_types.h location.hh position.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_to_bdd} interpreter.cpp -o interpreter.o

scanner.cpp :        scanner.l
			flex -o scanner.cpp -d scanner.l

parser.hpp location.hh position.hh stack.hh :	parser.cpp
	
parser.cpp :				parser.y
			bison -o parser.cpp -d parser.y

tags :							*.h *.cpp *.hpp *.hh
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean:
			rm -rf scanner.cpp
			rm -rf parser.cpp parser.hpp location.hh position.hh stack.hh
			rm -rf *.o
			rm -rf libVerilogToBdd.a
