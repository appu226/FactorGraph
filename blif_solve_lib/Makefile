IFLAGS_blif_solve_lib = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_blif_solve_lib = ${EXPORT_LFLAGS_factor_graph}

all : 			libBlifSolveLib.so

libBlifSolveLib.so :	clo.o cnf_dump.o blif_factors.o log.o approx_merge.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACS_dd}
	${CC} ${CC_LINK_FLAGS} -o libBlifSolveLib.so clo.o cnf_dump.o blif_factors.o log.o approx_merge.o

clo.o : clo.cpp clo.hpp
	${CC} ${CC_FLAGS} ${I_FLAGS_blif_solve_lib} clo.cpp -o clo.o

cnf_dump.o :	cnf_dump.cpp ${EXPORT_ARTIFACTS_dd}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve_lib} cnf_dump.cpp -o cnf_dump.o

blif_factors.o: blif_factors.cpp blif_factors.h ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve_lib} blif_factors.cpp -o blif_factors.o

log.o :	log.cpp log.h
	${CC} ${CC_FLAGS} log.cpp -o log.o

approx_merge.o:	approx_merge.cpp approx_merge.h ${EXPORT_ARTIFACTS_dd}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve_lib} approx_merge.cpp -o approx_merge.o

tags : 			*.cpp *.h
	touch tag
	rm tags
	ctags-exuberant ./*.cpp ./*.h ./*.hpp

clean :
	rm -f *.o libBlifSolveLib.so

