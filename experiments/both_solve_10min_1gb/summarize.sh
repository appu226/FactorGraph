awk -f summarize.awk full_log.txt | column -t -s ' ' | tee table.txt
