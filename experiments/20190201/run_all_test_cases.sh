basedir=experiments/20190201
outdir=$basedir/results
mkdir -p $outdir
for test_case in $(cat $basedir/remaining_test_cases.txt); do
  $basedir/run_one_test_case.sh $basedir $test_case $outdir
done
