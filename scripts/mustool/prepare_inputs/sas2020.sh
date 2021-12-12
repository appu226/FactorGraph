#!/usr/bin/bash

INPUT_ROOT=/home/parakram/Software/mustool/data/QBFEVAL_20_DATASET
OUTPUT_ROOT=/home/parakram/Software/DDP/data_sets/QBFEVAL_20_DATASET
QDIMACS_OUTPUT_ROOT=${OUTPUT_ROOT}/qdimacs
CNF_OUTPUT_ROOT=${OUTPUT_ROOT}/cnf_q_removed
mkdir -p $OUTPUT_ROOT $QDIMACS_OUTPUT_ROOT $CNF_OUTPUT_ROOT

find  $INPUT_ROOT -name "*.qdimacs" | xargs -n 1 -P 4 $(dirname $0)/simplify_qdimacs.sh ${QDIMACS_OUTPUT_ROOT} ${CNF_OUTPUT_ROOT}




