#!/bin/bash
basedir=$1
inputdir=$2
outputfile=$3
awk -F "[/ ]" -f $basedir/process_logs.awk -- $inputdir/*.log  | column -t -s, > $outputfile
