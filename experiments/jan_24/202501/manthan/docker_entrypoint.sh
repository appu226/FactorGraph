#!/bin/bash

source manthan-venv/bin/activate

echo docker started for test case $*

python /manthan/manthan.py $* 2>&1 | tee /workspace/manthan_run.log
cp /manthan/*.v /workspace

exit $?
