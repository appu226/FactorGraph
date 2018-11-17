IFLAGS_blif_solve = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_blif_solve = ${EXPORT_LFLAGS_factor_graph}


all : blif_solve

blif_solve:	main.o command_line_options.o ${EXPORT_ARTIFACTS_factor_graph}
	${CC} main.o command_line_options.o ${LFLAGS_blif_solve} -o blif_solve

main.o:	main.cpp command_line_options.h ${EXPORT_ARTIFACTS_factor_graph}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} main.cpp -o main.o

command_line_options.o:	command_line_options.cpp command_line_options.h ${EXPORT_ARTIFACTS_factor_graph}
	${CC} ${CC_FLAGS} ${IFLAGS_blif_solve} command_line_options.cpp -o command_line_options.o

clean :
	rm -f blif_solve
