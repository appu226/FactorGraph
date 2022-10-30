#!/bin/bash
if [ -z ${TEST_PROCESS_COUNT+x} ]; echo env var TEST_PROCESS_COUNT needs to be set to number of parallel test process.; exit 1; fi
cat unsolvable.txt | xargs --max-procs=${TEST_PROCESS_COUNT} -I TEST_CASE ./run_test.sh TEST_CASE
