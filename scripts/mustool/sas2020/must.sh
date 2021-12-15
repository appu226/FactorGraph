#!/usr/bin/bash

MUST_ROOT=/home/parakram/Software/mustool/trunk
OUTPUT_FOLDER=$1
INPUT_FILE=$2
filename=$(basename $2 .cnf)
echo $filename
timeout 120 ${MUST_ROOT}/must $INPUT_FILE -o ${OUTPUT_FOLDER}/${filename}.muc > ${OUTPUT_FOLDER}/${filename}.log

