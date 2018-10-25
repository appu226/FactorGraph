IFLAGS_factor_graph_main = ${EXPORT_IFLAGS_factor_graph}
LFLAGS_factor_graph_main = ${EXPORT_LFLAGS_factor_graph}


all : factor_graph_main

factor_graph_main :	main.cpp ${EXPORT_ARTIFACTS_factor_graph}
	${CC} ${DBG_FLAGS} main.cpp ${IFLAGS_factor_graph_main} ${LFLAGS_factor_graph_main} -o factor_graph_main

clean :
	rm -f factor_graph_main

