#!/usr/bin/bash

INPUT_FOLDER=/home/parakram/Software/DDP/data_sets/QBFEVAL_20_DATASET/cnf_q_removed
OUTPUT_FOLDER=/home/parakram/Software/DDP/data_sets/QBFEVAL_20_DATASET/cnfsat

find $INPUT_FOLDER -name "*.cnf" | xargs -n 1 -P 4 $(dirname $0)/must.sh $OUTPUT_FOLDER
