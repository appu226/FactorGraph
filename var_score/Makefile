IFLAGS_var_score = ${EXPORT_IFLAGS_factor_graph} ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_var_score = ${EXPORT_LFLAGS_factor_graph} ${EXPORT_LFLAGS_blif_solve_lib}


all : var_score

var_score:	main.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} main.o ${LFLAGS_var_score} -o var_score

main.o:	main.cpp ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_var_score} ${IFLAGS_blif_solve_lib} main.cpp -o main.o

clean :
	rm -f var_score *.o
