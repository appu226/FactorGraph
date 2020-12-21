basedir="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
outdir=$basedir/results
timeout=5m
mkdir -p $outdir
for test_case in $($basedir/collect_test_cases.sh); do
  $basedir/run_one_test_case.sh $basedir $test_case exact 100 $outdir $timeout
  for approximation_method in factor_graph early_quantification; do
    for max_bdd_size in 100 330 1000 3300 10000 33000 100000; do
      $basedir/run_one_test_case.sh $basedir $test_case $approximation_method $max_bdd_size $outdir $timeout
    done
  done
done
g++ -g -std=c++17 ${basedir}/process_logs.cpp -o ${basedir}/process_logs
${basedir}/process_logs ${outdir}/*.log | tee ${basedir}/Summary.txt
