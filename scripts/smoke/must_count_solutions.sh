set -e
blif_solve/blif_solve --under_approximating_method False --over_approximating_method ExactAndAccumulate --must_count_solutions --verbosity INFO test/data/simple_and.blif | grep "finished with" | tee temp/must_count_solutions.out
(exit ${PIPESTATUS[0]})
echo diff-ing temp/must_count_solutions.out test/expected_outputs/must_count_solutions.out
diff temp/must_count_solutions.out test/expected_outputs/must_count_solutions.out
echo SUCCESS
