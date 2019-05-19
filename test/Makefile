IFLAGS_test = ${EXPORT_IFLAGS_dd} ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_test = ${EXPORT_LFLAGS_dd} ${EXPORT_LFLAGS_blif_solve_lib}


all : test

test :	main.cpp bdd_factory.cpp bdd_factory.h testApproxMerge.cpp testApproxMerge.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} main.cpp bdd_factory.cpp testApproxMerge.cpp ${IFLAGS_test} ${LFLAGS_test} -o test

clean :
	rm -f test

