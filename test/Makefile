IFLAGS_test = ${EXPORT_IFLAGS_dd}
LFLAGS_test = ${EXPORT_LFLAGS_dd}


all : test

test :	main.cpp ${EXPORT_ARTIFACTS_dd}
	${CC} ${DBG_FLAGS} main.cpp ${IFLAGS_test} ${LFLAGS_test} -o test

clean :
	rm -f test

