basedir="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
outdir=$basedir/results
timeout=10s
mkdir -p $outdir
for test_case in $($basedir/collect_test_cases.sh); do
  $basedir/run_one_test_case.sh $basedir $test_case $outdir $timeout
done
g++ -g -std=c++17 ${basedir}/process_logs.cpp -o ${basedir}/process_logs
${basedir}/process_logs ${outdir}/*.log | tee ${basedir}/Summary.txt
