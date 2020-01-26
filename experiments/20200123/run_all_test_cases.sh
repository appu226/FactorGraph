basedir=experiments/20200123
outdir=$basedir/results
mkdir -p $outdir
for test_case in $($basedir/collect_test_cases.sh); do
  $basedir/run_one_test_case.sh $basedir $test_case $outdir
done
g++ -std=c++17 $basedir/process_logs.cpp -o $basedir/process_logs
$basedir/process_logs $outdir/*.log > $basedir/summary.txt
