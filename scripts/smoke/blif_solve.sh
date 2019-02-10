set -e

echo creating temp
mkdir -p temp

echo comparing ExactAndAccumulate with FactorGraphAprox
blif_solve/blif_solve --verbosity DEBUG --under_approximating_method ExactAndAccumulate --over_approximating_method FactorGraphApprox --must_count_solutions --diff_output_path temp/simple_and.dimacs test/data/simple_and.blif | grep -v sec > temp/simple_and.out
(exit ${PIPESTATUS[0]})

echo diff temp/simple_and.out and test/expected_outputs/simple_and.out
diff temp/simple_and.out test/expected_outputs/simple_and.out

echo diff temp/simple_and.dimacs test/expected_output/simple_and.dimacs
diff temp/simple_and.dimacs test/expected_outputs/simple_and.dimacs

echo deleting temp
rm -rf temp

echo SUCCESS

