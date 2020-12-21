base_dir=$1
test_case=$2;
approximation_method=$3
max_bdd_size=$4
outdir=$5
timeout=$6


# log outputs of all sub tests in this test case
test_case_log=$outdir/$test_case.log

# list of completed test_sub_cases
completed_list=$outdir/completed_list.txt
touch $completed_list


# run sub test case only if the completed list does not have this sub test
test_case_marker=${test_case}_${approximation_method}_${max_bdd_size}
 if grep -xq $test_case_marker $completed_list
then
  echo skipping $test_case_marker as it was already run
else
  echo running $test_case_marker
  


  # the actual command to run
  command="var_score/var_score --blif ../../data_sets/all_blifs/${test_case} --verbosity INFO --approximationMethod ${approximation_method} --maxBddSize ${max_bdd_size} --mustCountNumSolutions 1"

  rm -f ${test_case_log}.tmp
  echo $command >> ${test_case_log}.tmp


  # voodoo to limit execution time
  # starts in parallel a killer command to kill blif_solve in 2 mins
  # if blif_solve command completes in less than 2 mins, then kill the killer command
  # also note the .tmp file used to record log
  ((sleep ${timeout}; killall var_score) & ($command; killall sleep)) | tee -a ${test_case_log}.tmp


  cat ${test_case_log}.tmp >> $test_case_log
  rm -f ${test_case_log}.tmp
  

  echo $test_case_marker >> ${completed_list}
fi


