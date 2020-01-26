base_dir=$1
test_case=$2;
outdir=$3


echo running $test_case;

# log outputs of all sub tests in this test case
test_case_log=$outdir/$test_case.log

# list of completed test_sub_cases
completed_list=$outdir/completed_list.txt
touch $completed_list

# marker file for failures
failure_marker=$outdir/failure.marker

# loop across all sub tests
for lss in 1 5 10 20 40; do          # largest support set
  
  rm -f ${failure_marker}
  
  for nconv in 1 5 10 20; do    # number of convergences



    # run sub test case only if the completed list does not have this sub test
    test_case_marker=${test_case}_${lss}_${nconv}
    if grep -xq $test_case_marker $completed_list
    then
      echo skipping $test_case_marker as it was already run

    # run sub test case only if the previous smaller case succeded
    elif [ -f ${failure_marker} ]; then
      echo skipping $test_case_marker as the previous run failed | tee -a ${test_case_log}
      echo $test_case_marker >> $completed_list
    
    else

      echo running $test_case_marker
      touch ${failure_marker}
      result_cnf=${outdir}/${test_case_marker}.cnf


      # the actual command to run
      command="blif_solve/blif_solve --under_approximating_method Skip --largest_support_set $lss --num_convergence $nconv --over_approximating_method FactorGraphApprox --verbosity INFO --seed 20200123 ../../data_sets/bddblif/$test_case --diff_output_path ${result_cnf}"


      # create a temp file to store the log until the command is finished
      rm -f ${test_case_log}.tmp
      echo $command >> ${test_case_log}.tmp



      # voodoo to limit execution time
      # starts in parallel a killer command to kill blif_solve in 2 mins
      # if blif_solve command completes in less than 2 mins, then kill the killer command
      # also note the .tmp file used to record log
      ((sleep 2m; killall blif_solve) & ($command; killall sleep)) | tee -a ${test_case_log}.tmp


      # using ganak for counting solutions
      ganak_command="scripts/ganak.sh ${PWD}/${result_cnf}"
      echo $ganak_command >> ${test_case_log}.tmp
      ((sleep 2m; killall ganak) & (${ganak_command}; killall sleep)) | tee -a ${test_case_log}.tmp



      # mark this as successful if the resulting cnf file exists
      if [ -f ${result_cnf} ]; then
        rm -f ${failure_marker}
      fi


      # record the log of this test case into the permanent file
      cat ${test_case_log}.tmp >> $test_case_log
      rm -f ${test_case_log}.tmp
      rm -f ${result_cnf} ${result_cnf}.clauses.txt


      # mark test sub case as completed
      echo $test_case_marker >> $completed_list


    fi # end condition to run sub test case



  done # end loop over nconv
done   # end loop over lss
