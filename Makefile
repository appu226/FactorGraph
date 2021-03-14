export CC = g++

export cpp_opt_flag ?= -O3 # -DDEBUG_VAR_SCORE # -DDEBUG_MERGE
export CC_FLAGS = -std=c++17 -c -fPIC ${cpp_opt_flag} -Werror ${DBG_FLAGS}
export CC_LINK_FLAGS = -shared

# set up variables for using cudd as a dependency
PATH_cudd := $(CURDIR)/../cudd-3.0.0
export EXPORT_IFLAGS_cudd = -I${PATH_cudd}/include -I${PATH_cudd}/util -I${PATH_cudd}/cudd -I${PATH_cudd}/mtr -I${PATH_cudd}/epd -I ${PATH_cudd}/st -I${PATH_cudd}/dddmp -I${PATH_cudd}
export EXPORT_LFLAGS_cudd = -L${PATH_cudd}/include -L${PATH_cudd}/cudd/.libs -Wl,-rpath,${PATH_cudd}/cudd/.libs -lcudd

# set of underlying components
build.list ?= dd factor_graph factor_graph_main qbf_solve blif_solve_lib blif_solve cnf_dump var_score test
COMPONENTS = ${build.list} 

include dd/exports.mk
include factor_graph/exports.mk
include blif_solve_lib/exports.mk
include var_score/exports.mk

# pass make command goals to all components
all clean :
	for component in ${COMPONENTS} ; do \
		$(MAKE) -C $$component $@ ; \
	done

factor_graph : dd

factor_graph_main :	factor_graph

qbf_solve :		factor_graph

blif_solve : blif_solve_lib

blif_solve_lib : factor_graph dd

cnf_dump : blif_solve_lib

var_score : blif_solve_lib

test : dd blif_solve_lib

${COMPONENTS} : FORCE
	$(MAKE) -C $@ all

FORCE :

TAGS :
	ctags-exuberant -R . ${PATH_cudd}/cudd
