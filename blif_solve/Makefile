IFLAGS_blif_solve = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_blif_solve = ${EXPORT_LFLAGS_factor_graph}


all : blif_solve

blif_solve:	main.o command_line_options.o blif_factors.o log.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd}
	${CC} main.o command_line_options.o blif_factors.o log.o ${LFLAGS_blif_solve} -o blif_solve

main.o:	main.cpp command_line_options.h ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} log.h
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} main.cpp -o main.o

command_line_options.o:	command_line_options.cpp command_line_options.h log.h
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} command_line_options.cpp -o command_line_options.o

log.o: log.cpp log.h
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} log.cpp -o log.o

blif_factors.o: blif_factors.cpp blif_factors.h ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} log.h
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} blif_factors.cpp -o blif_factors.o

clean :
	rm -f blif_solve
	rm -f *.o
