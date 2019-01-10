#!/bin/sh
cat $1 | docker run -i -a stdin -a stdout msoos/approxmc
