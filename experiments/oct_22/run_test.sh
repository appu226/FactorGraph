#!/bin/bash
if [ -z ${TIME_OUT+x} ]; then echo Error: env var TIME_OUT needs to be set to number of seconds; exit 1; fi
if [ -z ${TEST_CASE_ROOT+x} ]; then echo Error: env var TEST_CASE_ROOT needs to be set to test case root folder; exit 1; fi
if [ -z ${RESULT_ROOT+x} ]; then echo Error: env var RESULT_ROOT needs to be set to result root folder; exit 1; fi
test_name=$1
export OUTPUT_PATH=${RESULT_ROOT}/${test_name}
mkdir -p ${OUTPUT_PATH}
echo $test_name
../../build/out/cnf_dump/simplify_qdimacs ${TEST_CASE_ROOT}/${test_name} ${OUTPUT_PATH}/simplified.qdimacs /dev/null
ulimit -t ${TIME_OUT}
../../build/out/oct_22/oct_22 --largestSupportSet 50 --inputFile ${OUTPUT_PATH}/simplified.qdimacs --verbosity INFO --outputFile ${OUTPUT_PATH}/result.cnf 1> ${OUTPUT_PATH}/run.log  2> ${OUTPUT_PATH}/run.err

