set -e
make -s -j 4
factor_graph_main/factor_graph_main
qbf_solve/qbf_solve
blif_solve/blif_solve --verbosity INFO --factor_graph --cudd test/data/simple_and.blif
test/test
echo SUCCESS
