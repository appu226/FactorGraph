#!/bin/bash
test_name=$1
export OUTPUT_PATH=${RESULT_ROOT}/${test_name}
mkdir -p ${OUTPUT_PATH}
echo $test_name
../../build/out/cnf_dump/simplify_qdimacs ${TEST_CASE_ROOT}/${test_name} ${OUTPUT_PATH}/simplified.qdimacs /dev/null
ulimit -t ${TIME_OUT}
../../build/out/oct_22/oct_22 --largestSupportSet ${LARGEST_SUPPORT_SET} --inputFile ${OUTPUT_PATH}/simplified.qdimacs --verbosity INFO --outputFile ${OUTPUT_PATH}/result.cnf 1> ${OUTPUT_PATH}/run.log  2> ${OUTPUT_PATH}/run.err
awk -f post_process.awk ${OUTPUT_PATH}/run.log >> ${RESULT_ROOT}/result_summary.txt

