IFLAGS_test = ${EXPORT_IFLAGS_var_score}
LFLAGS_test = ${EXPORT_LFLAGS_var_score}

TEST_CC_FLAGS = -std=c++17 ${cpp.opt.flag} -Werror

all : test cnf_dump_test clipping_and_abstract_test

test : test.cpp testApproxMerge.o testVarScoreQuantification.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib} ${EXPORT_ARTIFACTS_var_score}
	${CC} ${TEST_CC_FLAGS} test.cpp testVarScoreQuantification.o testApproxMerge.cpp ${IFLAGS_test} ${LFLAGS_test} -o test

testApproxMerge.o : testApproxMerge.cpp testApproxMerge.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${TEST_CC_FLAGS} -c testApproxMerge.cpp ${IFLAGS_test} ${LFLAGS_test} -o testApproxMerge.o

testVarScoreQuantification.o : testVarScoreQuantification.cpp testVarScoreQuantification.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib} ${EXPORT_ARTIFACTS_var_score}
	${CC} ${TEST_CC_FLAGS} -c testVarScoreQuantification.cpp ${IFLAGS_test} ${LFLAGS_test} -o testVarScoreQuantification.o

cnf_dump_test : cnf_dump_test.cpp random_bdd_generator.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${TEST_CC_FLAGS} cnf_dump_test.cpp random_bdd_generator.o ${IFLAGS_test} ${LFLAGS_test} -o cnf_dump_test

random_bdd_generator.o : random_bdd_generator.h random_bdd_generator.cpp
	${CC} ${TEST_CC_FLAGS} -c random_bdd_generator.cpp ${IFLAGS_test} ${LFLAGS_test} -o random_bdd_generator.o

clipping_and_abstract_test : clipping_and_abstract_test.cpp random_bdd_generator.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${TEST_CC_FLAGS} clipping_and_abstract_test.cpp random_bdd_generator.o ${IFLAGS_test} ${LFLAGS_test} -o clipping_and_abstract_test

clean :
	rm -f *.o cnf_dump_test clipping_and_abstract_test test

