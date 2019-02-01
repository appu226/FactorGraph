set -e
make -s -j 4

scripts/prefix.sh factor_graph_main scripts/smoke/factor_graph_main.sh

scripts/prefix.sh qbf_solve         scripts/smoke/qbf_solve.sh

scripts/prefix.sh blif_solve        scripts/smoke/blif_solve.sh


mkdir -p temp

scripts/prefix.sh must_count_solutions scripts/smoke/must_count_solutions.sh

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
