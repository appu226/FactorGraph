base_dir=$1
test_case=$2;
outdir=$3


echo running $test_case;

# log outputs of all sub tests in this test case
test_case_log=$outdir/$test_case.log

# list of completed test_sub_cases
completed_list=$outdir/completed_list.txt
touch $completed_list

# loop across all sub tests
for vns in 1 5 10 20 30 40; do          # var node size
  for fns in 1 5 10; do                 # func node size
    for nconv in 1 2 3 4 5 10 20; do    # number of convergences


      # run sub test case only if the completed list does not have this sub test
      test_case_marker=${test_case}_${vns}_${fns}_${nconv}
      if grep -xq $test_case_marker $completed_list
      then
        echo skipping $test_case_marker

      else
        echo running $test_case_marker

        # the actual command to run
        command="blif_solve/blif_solve --must_count_solutions --under_approximating_method False --var_node_size $vns --func_node_size $fns --num_convergence $nconv --over_approximating_method FactorGraphApprox --verbosity INFO --seed 20190201 ../../data_sets/bddblif/$test_case"
        
        
        # create a temp file to store the log until the command is finished
        rm -f ${test_case_log}.tmp
        echo $command >> ${test_case_log}.tmp

        
        
        # voodoo to limit execution time
        # starts in parallel a killer command to kill blif_solve in 2 mins
        # if blif_solve command completes in less than 2 mins, then kill the killer command
        # also note the .tmp file used to record log
        ((sleep 2m; killall blif_solve) & ($command; killall sleep)) | tee -a ${test_case_log}.tmp


        # record the log of this test case into the permanent file
        cat ${test_case_log}.tmp >> $test_case_log
        rm ${test_case_log}.tmp


        # mark test sub case as completed
        echo $test_case_marker >> $completed_list


      fi # end condition to run sub test case



    done # end loop over nconv
  done   # end loop over fns
done     # end loop over vns
