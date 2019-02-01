set -e
$2 | sed "s/^/[$1] /"
exit ${PIPESTATUS[0]}
