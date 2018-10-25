IFLAGS_qbf_solve = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_qbf_solve = ${EXPORT_LFLAGS_factor_graph}


all : qbf_solve

qbf_solve:	main.cpp ${EXPORT_ARTIFACTS_factor_graph}
	${CC} ${DBG_FLAGS} main.cpp ${IFLAGS_qbf_solve} ${LFLAGS_qbf_solve} -o qbf_solve

clean :
	rm -f qbf_solve
