export CC = g++

export DBG_FLAGS = -g
export CC_FLAGS = -c -Werror ${DBG_FLAGS}

# set up variables for using cudd as a dependency
PATH_cudd := $(CURDIR)/../cudd-3.0.0
export EXPORT_IFLAGS_cudd = -I${PATH_cudd}/include -I${PATH_cudd}/util -I${PATH_cudd}/cudd -I${PATH_cudd}/mtr -I${PATH_cudd}/epd -I ${PATH_cudd}/st -I${PATH_cudd}/dddmp -I${PATH_cudd}
export EXPORT_LFLAGS_cudd = -L${PATH_cudd}/include -L${PATH_cudd}/cudd/.libs -Wl,-rpath,${PATH_cudd}/cudd/.libs -lcudd

# set of underlying components
COMPONENTS = dd factor_graph factor_graph_main qbf_solve blif_solve

include dd/exports.mk
include factor_graph/exports.mk

# pass make command goals to all components
all clean :
	for component in ${COMPONENTS} ; do \
		$(MAKE) -C $$component $@ ; \
	done

factor_graph : dd

factor_graph_main :	factor_graph

qbf_solve :		factor_graph

blif_solve : factor_graph

${COMPONENTS} : FORCE
	$(MAKE) -C $@ all

FORCE :

TAGS :
	ctags-exuberant -R . ${PATH_cudd}/cudd
