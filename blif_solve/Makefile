IFLAGS_blif_solve = ${EXPORT_IFLAGS_factor_graph} ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_blif_solve = ${EXPORT_LFLAGS_factor_graph} ${EXPORT_LFLAGS_blif_solve_lib}


all : blif_solve

blif_solve:	main.o command_line_options.o blif_solve_method.o ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} main.o command_line_options.o blif_solve_method.o ${LFLAGS_blif_solve} -o blif_solve

main.o:	main.cpp command_line_options.h blif_solve_method.h ${EXPORT_ARTIFACTS_factor_graph} ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} ${IFLAGS_blif_solve_lib} main.cpp -o main.o

command_line_options.o:	command_line_options.cpp command_line_options.h ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} command_line_options.cpp -o command_line_options.o

blif_solve_method.o:	blif_solve_method.cpp blif_solve_method.h command_line_options.h
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} blif_solve_method.cpp -o blif_solve_method.o

clean :
	rm -f blif_solve *.o
