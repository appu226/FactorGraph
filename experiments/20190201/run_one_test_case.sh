base_dir=$1
test_case=$2;
outdir=$3


echo running $test_case;
test_case_log=$outdir/$test_case.log
rm -r $test_case_log
for vns in 1 5 10 20 30 40; do
  for fns in 1 5 10; do
    for nconv in 1 2 3 4 5 10 20; do
      command="blif_solve/blif_solve --must_count_solutions --under_approximating_method False --var_node_size $vns --func_node_size $fns --num_convergence $nconv --over_approximating_method FactorGraphApprox --verbosity INFO --seed 20190201 ../../data_sets/bddblif/$test_case"
      echo $command >> $test_case_log
      ((sleep 2m; killall blif_solve) & ($command; killall sleep)) | tee -a $test_case_log
    done
  done
done
