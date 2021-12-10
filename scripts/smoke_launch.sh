set -e

scripts/prefix.sh factor_graph_main scripts/smoke/factor_graph_main.sh

scripts/prefix.sh qbf_solve         scripts/smoke/qbf_solve.sh

scripts/prefix.sh blif_solve        scripts/smoke/blif_solve.sh


mkdir -p temp

scripts/prefix.sh must_count_solutions scripts/smoke/must_count_solutions.sh

scripts/prefix.sh test scripts/smoke/test.sh

scripts/prefix.sh testCnfDump scripts/smoke/testCnfDump.sh

scripts/prefix.sh 6s365r_bdd scripts/smoke/6s365r_bdd.sh

scripts/prefix.sh simplify_qdimacs scripts/smoke/simplify_qdimacs.sh

rm -rf temp

echo
echo SUCCESS
