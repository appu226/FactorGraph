IFLAGS_blif_solve = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_blif_solve = ${EXPORT_LFLAGS_factor_graph}


all : blif_solve

blif_solve:	main.cpp ${EXPORT_ARTIFACTS_factor_graph}
	${CC} ${DBG_FLAGS} main.cpp ${IFLAGS_blif_solve} ${LFLAGS_blif_solve} -o blif_solve

clean :
	rm -f blif_solve
