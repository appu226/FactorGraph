IFLAGS_var_score = ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_var_score = ${EXPORT_LFLAGS_blif_solve_lib}


all : var_score libVarScore.a

libVarScore.a: var_score_quantification.o
	ar r libVarScore.a var_score_quantification.o
	ranlib libVarScore.a

var_score:	main.o var_score_quantification.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} main.o var_score_quantification.o ${LFLAGS_var_score} -o var_score

main.o:	main.cpp var_score_quantification.h ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_var_score} main.cpp -o main.o


var_score_quantification.o: var_score_quantification.h var_score_quantification.cpp
	${CC} ${CC_FLAGS} ${IFLAGS_var_score} var_score_quantification.cpp -o var_score_quantification.o

clean :
	rm -f libVarScore.a var_score *.o
