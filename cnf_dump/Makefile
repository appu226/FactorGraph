IFLAGS_cnf_dump = ${EXPORT_IFLAGS_dd} ${EXPORT_IFLAGS_blif_solve_lib}
LFLAGS_cnf_dump = ${EXPORT_LFLAGS_dd} ${EXPORT_LFLAGS_blif_solve_lib}


all : cnf_dump

cnf_dump:	main.o cnf_dump_clo.o ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} main.o cnf_dump_clo.o ${LFLAGS_cnf_dump} -o cnf_dump

main.o:	main.cpp cnf_dump_clo.h ${EXPORT_ARTIFACTS_dd} ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_cnf_dump} main.cpp -o main.o

cnf_dump_clo.o: cnf_dump_clo.cpp cnf_dump_clo.h ${EXPORT_ARTIFACTS_blif_solve_lib}
	${CC} ${CC_FLAGS} ${IFLAGS_cnf_dump} cnf_dump_clo.cpp -o cnf_dump_clo.o

clean :
	rm -f cnf_dump *.o
