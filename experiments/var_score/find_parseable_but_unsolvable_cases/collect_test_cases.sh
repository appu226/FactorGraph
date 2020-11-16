ls ../../data_sets/all_blifs -s | sort -n | grep -v total | awk '{ } {print $2;}'

