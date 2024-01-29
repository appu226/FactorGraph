#!/bin/bash

set -e

export SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

###### get pre-requisites ######
# apt-get update
# apt-get install -y git gcc g++ sudo make automake cmake libz-dev libreadline-dev libboost-all-dev libm4ri-dev build-essential python



###### cudd ######
cd $SCRIPT_DIR/../..
git clone https://github.com/appu226/cudd.git cudd-3.0.0
cd cudd-3.0.0
./configure ACLOCAL=aclocal AUTOMAKE=automake --enable-dddmp --enable-shared
make -j check

###### kissat ######
cd $SCRIPT_DIR/../..
git clone https://github.com/appu226/kissat.git kissat
cd kissat
git checkout cav_2024
./configure
make -j test

###### bfss ######
cd $SCRIPT_DIR/../..
git clone https://github.com/BooleanFunctionalSynthesis/bfss.git bfss
cd bfss
git submodule update --init benchmarks
git submodule update --init dependencies/abc
bash setup.sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/abc/
make UNIGEN=NO BUILD=RELEASE

###### factor graph ######
# cd $SCRIPT_DIR/../..
# git clone https://github.com/appu226/FactorGraph.git FactorGraph
cd $SCRIPT_DIR/..
cmake -S . -B build/out -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j -C build/out && build/out/test/test1


##### collect test cases #####
cd $SCRIPT_DIR/../..
mkdir all_test_cases
for x in $(ls bfss/benchmarks/*/qdimacs/*.qdimacs);
    do mv $x all_test_cases/$(echo $x | cut -f 3 -d /)_$(basename $x);
    done
