set -e

build/out/blif_solve/blif_solve --verbosity INFO --dot_dump_path temp --under_approximating_method AcyclicViaForAll --over_approximating_method FactorGraphApprox --largest_support_set 20 --largest_bdd_size 100000 --diff_output_path temp/6s365r_bdd.dimacs test/data/6s365r_bdd.blif
echo diff temp/6s365r_bdd.dimacs test/expected_outputs/6s365r_bdd.dimacs
diff temp/6s365r_bdd.dimacs test/expected_outputs/6s365r_bdd.dimacs
echo SUCCESS
