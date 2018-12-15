set -e
make -s -j 4
factor_graph_main/factor_graph_main | sed "s/^/[factor_graph_main] /"
qbf_solve/qbf_solve | sed "s/^/[qbf_solve] /"
blif_solve/blif_solve --verbosity DEBUG --factor_graph --cudd test/data/simple_and.blif | grep -v sec | tee temp/simple_and.out | sed "s/^/[blif_solve] /"
diff temp/simple_and.out test/expected_outputs/simple_and.out
echo "[blif_solve diff] SUCCESS"
test/test | sed "s/^/[test] /"
echo
echo SUCCESS
