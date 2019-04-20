IFLAGS_dd = ${EXPORT_IFLAGS_cudd}
LFLAGS_dd = ${EXPORT_LFLAGS_cudd}

all : 			libDd.a

libDd.a :	dd.o bnet.o ntr.o ntrHeap.o ntrMflow.o cuddAndAbsMulti.o bdd_partition.o
			ar r libDd.a dd.o bnet.o ntr.o ntrHeap.o ntrMflow.o cuddAndAbsMulti.o bdd_partition.o
			ranlib libDd.a
	
dd.o :			dd.cpp dd.h bnet.h ntr.h cuddAndAbsMulti.h
			${CC} ${CC_FLAGS} ${IFLAGS_dd} dd.cpp -o dd.o

bnet.o :    bnet.c bnet.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} bnet.c -o bnet.o

ntr.o :     ntr.c ntr.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} ntr.c -o ntr.o

ntrHeap.o : ntrHeap.c ntr.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} ntrHeap.c -o ntrHeap.o

ntrMflow.o : ntrMflow.c ntr.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} ntrMflow.c -o ntrMflow.o

bdd_partition.o : bdd_partition.cpp bdd_partition.h dd.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} bdd_partition.cpp -o bdd_partition.o

cuddAndAbsMulti.o : cuddAndAbsMulti.cpp cuddAndAbsMulti.h lru_cache.h optional.h
	    ${CC} ${CC_FLAGS} ${IFLAGS_dd} cuddAndAbsMulti.cpp -o cuddAndAbsMulti.o

tags : 			*.cpp *.h
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean :
			rm -f *.o libDd.a

