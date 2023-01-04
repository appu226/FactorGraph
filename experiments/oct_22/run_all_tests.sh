#!/bin/bash
if [ -z ${TIME_OUT+x} ]; then echo Error: env var TIME_OUT needs to be set to number of seconds; exit 1; fi
if [ -z ${LARGEST_SUPPORT_SET+x} ]; then echo Error: env var LARGEST_SUPPORT_SET needs to be set; exit 1; fi
if [ -z ${LARGEST_BDD_SIZE+x} ]; then echo Error: env var LARGEST_BDD_SIZE needs to be set; exit 1; fi
if [ -z ${TEST_CASE_ROOT+x} ]; then echo Error: env var TEST_CASE_ROOT needs to be set to test case root folder; exit 1; fi
if [ -z ${RESULT_ROOT+x} ]; then echo Error: env var RESULT_ROOT needs to be set to result root folder; exit 1; fi
if [ -z ${TEST_PROCESS_COUNT+x} ]; then echo env var TEST_PROCESS_COUNT needs to be set to number of parallel test process.; exit 1; fi
if [ -z ${TEST_CASE_LIST_FILE+x} ]; then echo env var TEST_CASE_LIST_FILE needs to be set to file with list of test cases.; exit 1; fi

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
