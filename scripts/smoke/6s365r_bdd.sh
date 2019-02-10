set -e

blif_solve/blif_solve --verbosity INFO --dot_dump_path temp --under_approximating_method AcyclicViaForAll --over_approximating_method FactorGraphApprox --var_node_size 20 --diff_output_path temp/6s365r_bdd.dimacs test/data/6s365r_bdd.blif
diff temp/6s365r_bdd.dimacs test/expected_outputs/6s365r_bdd.dimacs
echo SUCCESS
