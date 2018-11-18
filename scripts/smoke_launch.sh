set -e
make -s -j 4
factor_graph_main/factor_graph_main
qbf_solve/qbf_solve
blif_solve/blif_solve --verbosity INFO --cudd /home/parakram/Software/DDP/data_sets/bddblif/6s365r_bdd.blif
echo SUCCESS
