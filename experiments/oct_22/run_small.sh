#!/bin/bash
set -e
if [[ $(basename $(pwd)) != oct_22 ]]; 
then
    cd experiments/oct_22; 
fi
rm -rf results_small
TIME_OUT=120 LARGEST_SUPPORT_SET=10 LARGEST_BDD_SIZE=100 TEST_CASE_ROOT=../../../../data_sets/bfss RESULT_ROOT=results_small TEST_PROCESS_COUNT=4 TEST_CASE_LIST_FILE=small_cases.txt ./run_all_tests.sh
cat results_small/result_summary.txt
