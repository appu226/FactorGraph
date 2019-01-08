IFLAGS_blif_solve_lib = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_blif_solve_lib = ${EXPORT_LFLAGS_factor_graph}

all : 			libBlifSolveLib.a

libBlifSolveLib.a :	cnf_dump.o log.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACS_dd}
			ar r libBlifSolveLib.a cnf_dump.o log.o
			ranlib libBlifSolveLib.a
	
cnf_dump.o :	cnf_dump.cpp ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_FLAGS} ${IFLAGS_blif_solve_lib} cnf_dump.cpp -o cnf_dump.o

log.o :	log.cpp log.h
			${CC} ${CC_FLAGS} log.cpp -o log.o

tags : 			*.cpp *.h
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h 

clean :
			rm -f *.o libBlifSolveLib.a

