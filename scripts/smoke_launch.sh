set -e
make -s -j 4
factor_graph_main/factor_graph_main | sed "s/^/[factor_graph_main] /"
(exit ${PIPESTATUS[0]})
qbf_solve/qbf_solve | sed "s/^/[qbf_solve] /"
(exit ${PIPESTATUS[0]})
mkdir -p temp
blif_solve/blif_solve --verbosity DEBUG --factor_graph --cudd test/data/simple_and.blif | grep -v sec | tee temp/simple_and.out | sed "s/^/[blif_solve] /"
diff temp/simple_and.out test/expected_outputs/simple_and.out
rm -rf temp
echo "[blif_solve diff] SUCCESS"
mkdir temp
test/test | sed "s/^/[test] /"
(exit ${PIPESTATUS[0]})
(cat temp/testCnfDump_headers.dimacs; cat temp/testCnfDump_clauses.dimacs) > temp/testCnfDump.dimacs
diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
rm -rf temp
echo
echo SUCCESS
