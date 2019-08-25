IFLAGS_verilog_parser = ${EXPORT_IFLAGS_dd}
LFLAGS_verilog_parser = ${EXPORT_LFLAGS_dd}

all :                libVerilogParser.a


libVerilogParser.a : parser.o scanner.o interpreter.o ${EXPORT_ARTIFACTS_dd}
			ar r libVerilogParser.a parser.o scanner.o interpreter.o
			ranlib libVerilogParser.a


parser.o :           parser.cpp verilog_types.h scanner.h parser.hpp interpreter.h location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_parser} parser.cpp -o parser.o
	   
scanner.o :          scanner.cpp interpreter.h parser.hpp location.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_parser} scanner.cpp -o scanner.o
	   
interpreter.o :      interpreter.cpp interpreter.h verilog_types.h location.hh position.hh
			${CC} ${CC_FLAGS} ${IFLAGS_verilog_parser} interpreter.cpp -o interpreter.o

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
			rm -rf libVerilogParser
