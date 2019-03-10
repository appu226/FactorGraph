basedir=experiments/Clipping
outdir=$basedir/results
mkdir -p $outdir
for test_case in $($basedir/collect_test_cases.sh); do
  $basedir/run_one_test_case.sh $basedir $test_case $outdir
done
$basedir/summarize_results.sh $basedir $outdir ${basedir}/summary.txt
