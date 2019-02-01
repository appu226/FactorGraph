ls ../../data_sets/bddblif -s | sort -n | grep -v total | awk '{ } {print $2;}'

