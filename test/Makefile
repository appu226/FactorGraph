IFLAGS_test = ${EXPORT_IFLAGS_dd} ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_test = ${EXPORT_LFLAGS_dd} ${EXPORT_LFLAGS_blif_solve_lib}


all : test cnf_dump_test clipping_and_abstract_test

test : test.cpp bdd_factory.o testApproxMerge.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} test.cpp bdd_factory.o testApproxMerge.cpp ${IFLAGS_test} ${LFLAGS_test} -o test

bdd_factory.o : bdd_factory.cpp bdd_factory.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} -c bdd_factory.cpp ${IFLAGS_test} ${LFLAGS_test} -o bdd_factory.o

testApproxMerge.o : testApproxMerge.cpp testApproxMerge.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} -c testApproxMerge.cpp ${IFLAGS_test} ${LFLAGS_test} -o testApproxMerge.o

cnf_dump_test : cnf_dump_test.cpp random_bdd_generator.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} cnf_dump_test.cpp random_bdd_generator.o ${IFLAGS_test} ${LFLAGS_test} -o cnf_dump_test

random_bdd_generator.o : random_bdd_generator.h random_bdd_generator.cpp
	${CC} ${DBG_FLAGS} -c random_bdd_generator.cpp ${IFLAGS_test} ${LFLAGS_test} -o random_bdd_generator.o

clipping_and_abstract_test : clipping_and_abstract_test.cpp random_bdd_generator.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${DBG_FLAGS} clipping_and_abstract_test.cpp random_bdd_generator.o ${IFLAGS_test} ${LFLAGS_test} -o clipping_and_abstract_test

clean :
	rm -f *.o cnf_dump_test clipping_and_abstract_test test

