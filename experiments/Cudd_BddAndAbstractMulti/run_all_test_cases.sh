base_dir=$(dirname $0)
test_cases_dir=$1

results_dir=${base_dir}/results
mkdir -p ${results_dir}
log_file=${results_dir}/full_run.log
rm -f ${log_file}
summary_file=${results_dir}/summary.txt


for test_case in $(ls ${test_cases_dir}/*.blif); do
  test_case_name=$(basename ${test_case} .blif)
  command="blif_solve/blif_solve --under_approximating_method Skip --over_approximating_method ExactAndAbstractMulti --must_count_solutions --verbosity INFO ${test_case}"
  echo Running ${test_case_name} | tee -a ${log_file}
  (timeout 2m ${command}) | tee -a ${log_file}
  echo Finished ${test_case_name}
done

g++ ${base_dir}/process_logs.cpp -o ${base_dir}/process_logs
process_logs <${log_file} | tee ${summary_file}
