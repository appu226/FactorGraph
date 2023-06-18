#!/usr/bin/bash
export TIME_OUT=7200
export LARGEST_SUPPORT_SET=40
export LARGEST_BDD_SIZE=100
export TEST_CASE_ROOT=../../../../data_sets/bfss
export RESULT_ROOT=../../../../data_sets/results_20230617_$1
export TEST_PROCESS_COUNT=4
export TEST_CASE_LIST_FILE=all_case_chunks/chunk$1

mkdir -p ${RESULT_ROOT}
cp index.txt ${RESULT_ROOT}/result_summary.txt

echo export TIME_OUT=${TIME_OUT} sec > ${RESULT_ROOT}/test_environment.txt
echo export LARGEST_SUPPORT_SET=${LARGEST_SUPPORT_SET}  >> ${RESULT_ROOT}/test_environment.txt
echo export LARGEST_BDD_SIZE=${LARGEST_BDD_SIZE}  >> ${RESULT_ROOT}/test_environment.txt
echo export TEST_CASE_ROOT=${TEST_CASE_ROOT}  >> ${RESULT_ROOT}/test_environment.txt
echo export RESULT_ROOT=${RESULT_ROOT}  >> ${RESULT_ROOT}/test_environment.txt
echo export TEST_PROCESS_COUNT=${TEST_PROCESS_COUNT} >> ${RESULT_ROOT}/test_environment.txt
echo export TEST_CASE_LIST_FILE=${TEST_CASE_LIST_FILE} >> ${RESULT_ROOT}/test_environment.txt
echo >> ${RESULT_ROOT}/test_environment.txt
echo $0 $* >> ${RESULT_ROOT}/test_environment.txt

cat ${TEST_CASE_LIST_FILE} | xargs --max-procs=${TEST_PROCESS_COUNT} -I TEST_CASE ./run_test.sh TEST_CASE

