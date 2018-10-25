IFLAGS_factor_graph = ${EXPORT_IFLAGS_cudd}
LFLAGS_factor_graph = ${EXPORT_LFLAGS_cudd}

all : 			libFactorGraph.a

libFactorGraph.a :	factor_graph.o hash_table.o dd.o qbf.o
			ar r libFactorGraph.a factor_graph.o hash_table.o dd.o qbf.o
			ranlib libFactorGraph.a
	
factor_graph.o :	factor_graph.cpp factor_graph.h dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} factor_graph.cpp -o factor_graph.o


hash_table.o :		hash_table.cpp hash_table.h factor_graph.h dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} hash_table.cpp -o hash_table.o

dd.o :			dd.cpp dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} dd.cpp -o dd.o

qbf.o :			qbf.cpp qbf.h factor_graph.h dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} qbf.cpp -o qbf.o

tags : 			*.cpp *.h
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean :
			rm -f *.o libFactorGraph.a

