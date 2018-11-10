IFLAGS_dd = ${EXPORT_IFLAGS_cudd}
LFLAGS_dd = ${EXPORT_LFLAGS_cudd}

all : 			libDd.a

libDd.a :	dd.o
			ar r libDd.a dd.o
			ranlib libDd.a
	
dd.o :			dd.cpp dd.h
			${CC} ${CC_FLAGS} ${IFLAGS_dd} dd.cpp -o dd.o

tags : 			*.cpp *.h
			touch tag
			rm tags
			ctags-exuberant ./*.cpp ./*.h ../${CUDD}/cudd/cuddAPI.c

clean :
			rm -f *.o libDd.a

