IFLAGS_factor_graph = ${EXPORT_IFLAGS_dd}
LFLAGS_factor_graph = ${EXPORT_LFLAGS_dd}

all : 			libFactorGraph.so

libFactorGraph.so :	factor_graph.o hash_table.o qbf.o srt.o ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_LINK_FLAGS} -o libFactorGraph.so factor_graph.o hash_table.o qbf.o srt.o
	
factor_graph.o :	factor_graph.cpp factor_graph.h ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} factor_graph.cpp -o factor_graph.o


hash_table.o :		hash_table.cpp hash_table.h factor_graph.h ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} hash_table.cpp -o hash_table.o

qbf.o :			qbf.cpp qbf.h factor_graph.h ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} qbf.cpp -o qbf.o

srt.o :			srt.cpp srt.h qbf.h factor_graph.h ${EXPORT_ARTIFACTS_dd}
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} srt.cpp -o srt.o

tags : 			*.cpp *.h
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean :
			rm -f *.o libFactorGraph.so

