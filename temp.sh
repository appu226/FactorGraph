#!/bin/bash


##### jan_24 #####
python3 jan_24/jan_24.py --test_case_path ../../data_sets/bfss/benchmarks/Factorization/qdimacs/factorization8.qdimacs --output_root temp --verbosity DEBUG --factor_graph_bin=/home/parakram/Software/DDP/FactorGraph/src/build/out --bfss_bin=/home/parakram/Software/DDP/bfss/bin


# build/out/jan_24/kissat_preprocess --verbosity DEBUG --inputFile temp/factorization8_bfss_input.qdimacs.noUnary --outputFile temp/factorization8_bfss_input_new.qdimacs







##### oct_22 #####
# code=build/out/oct_22/oct_22


# #lss=1
# lss=20
# #lss=50

# inputFile=../../data_sets/QBFEVAL_20_DATASET/qdimacs/LQ_PARITY-N-3.qdimacs 
# #inputFile=equalization256.qdimacs


# vbty=INFO


# outputFile=temp.txt


# args="--largestSupportSet $lss --inputFile $inputFile --verbosity $vbty --outputFile $outputFile"


# if [ $# -gt 0 ] && [ $1 = "gdb" ]
# then
#   echo gdb -ex run --args $code $args "${@:2}"
#   gdb -ex run --args $code $args "${@:2}"
# else
#   echo $code $args ${@:1}
#   $code $args ${@:1}
# fi










##### may_22 #####
#build/out/may_22/may_22 --inputFile ../../data_sets/QBFEVAL_20_DATASET/qdimacs/LQ_PARITY-N-3.qdimacs --verbosity DEBUG --computeExact 1 


