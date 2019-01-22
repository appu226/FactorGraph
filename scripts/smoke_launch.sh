set -e
make -s -j 4

factor_graph_main/factor_graph_main | sed "s/^/[factor_graph_main] /"
(exit ${PIPESTATUS[0]})
qbf_solve/qbf_solve | sed "s/^/[qbf_solve] /"
(exit ${PIPESTATUS[0]})

mkdir -p temp
blif_solve/blif_solve --verbosity DEBUG --under_approximating_method ExactAndAccumulate --over_approximating_method FactorGraphApprox --diff_output_path temp/simple_and.dimacs test/data/simple_and.blif | grep -v sec | tee temp/simple_and.out | sed "s/^/[blif_solve] /"
 diff temp/simple_and.out test/expected_outputs/simple_and.out
diff temp/simple_and.dimacs test/expected_outputs/simple_and.dimacs
rm -rf temp
echo "[blif_solve diff] SUCCESS"

mkdir -p temp

test/test | sed "s/^/[test] /"
(exit ${PIPESTATUS[0]})
 diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs

echo [testCnfDump] counting number of solutions
cat temp/testCnfDump.dimacs | scripts/approxmc.sh | grep "Number of solutions" > temp/testCnfDump.scalmc
echo [testCnfDump] $(cat temp/testCnfDump.scalmc)
diff temp/testCnfDump.scalmc test/expected_outputs/testCnfDump.scalmc
echo [testCnfDump] SUCCESS

blif_solve/blif_solve --verbosity INFO --dot_dump_path temp --under_approximating_method AcyclicViaForAll --over_approximating_method FactorGraphApprox --var_node_size 20 --diff_output_path temp/6s365r_bdd.dimacs test/data/6s365r_bdd.blif | sed "s/^/[6s365r_bdd] /"
(exit ${PIPESTATUS[0]})
diff temp/6s365r_bdd.dimacs test/expected_outputs/6s365r_bdd.dimacs
echo [6s365r_bdd] SUCCESS

rm -rf temp
echo
echo SUCCESS
