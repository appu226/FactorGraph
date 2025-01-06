#!/bin/bash

source manthan-venv/bin/activate

echo full command is $*

python manthan.py $1
