#!/bin/bash
if [ -z ${TIME_OUT+x} ]; then echo Error: env var TIME_OUT needs to be set to number of seconds; exit 1; fi
if [ -z ${LARGEST_SUPPORT_SET+x} ]; then echo Error: env var LARGEST_SUPPORT_SET needs to be set; exit 1; fi
if [ -z ${TEST_CASE_ROOT+x} ]; then echo Error: env var TEST_CASE_ROOT needs to be set to test case root folder; exit 1; fi
if [ -z ${RESULT_ROOT+x} ]; then echo Error: env var RESULT_ROOT needs to be set to result root folder; exit 1; fi
if [ -z ${TEST_PROCESS_COUNT+x} ]; then echo env var TEST_PROCESS_COUNT needs to be set to number of parallel test process.; exit 1; fi
cat unsolvable.txt | xargs --max-procs=${TEST_PROCESS_COUNT} -I TEST_CASE ./run_test.sh TEST_CASE
