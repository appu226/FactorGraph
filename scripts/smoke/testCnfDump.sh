set -e

echo counting number of solutions
cat temp/testCnfDump.dimacs | scripts/approxmc.sh | grep "Number of solutions" | tee temp/testCnfDump.scalmc
diff temp/testCnfDump.scalmc test/expected_outputs/testCnfDump.scalmc
echo SUCCESS
