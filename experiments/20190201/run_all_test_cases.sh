basedir=experiments/20190201
outdir=$basedir/results
mkdir -p $outdir
for test_case in $($basedir/collect_test_cases.sh); do
  echo running $test_case;
  test_case_log=$outdir/$test_case.log
  rm -r $test_case_log
  for vns in 1 5 10 20 30 40; do
    for fns in 1 5 10; do
      command="blif_solve/blif_solve --under_approximating_method False --var_node_size $vns --func_node_size $fns --over_approximating_method FactorGraphApprox --verbosity INFO ../../data_sets/bddblif/$test_case"
      echo $command >> $test_case_log
      ((sleep 2m; killall blif_solve) & ($command; killall sleep)) | tee -a $test_case_log
    done
  done
done
