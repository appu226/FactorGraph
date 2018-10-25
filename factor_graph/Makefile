IFLAGS_factor_graph = ${EXPORT_IFLAGS_cudd}
LFLAGS_factor_graph = ${EXPORT_LFLAGS_cudd}

all : 			libFactorGraph.a

libFactorGraph.a :	factor_graph.o hash_table.o dd.o
			ar r libFactorGraph.a factor_graph.o hash_table.o dd.o
			ranlib libFactorGraph.a
	
factor_graph.o :	factor_graph.c factor_graph.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} factor_graph.c -o factor_graph.o

hash_table.o :		hash_table.c hash_table.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} hash_table.c -o hash_table.o

dd.o :			dd.c dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_factor_graph} dd.c -o dd.o

tags : 			*.c *.h
			touch tag
			rm tags
			ctags-exuberant ./*.c ./*.h ../${CUDD}/cudd/cuddAPI.c

clean :
			rm -f *.o libFactorGraph.a

