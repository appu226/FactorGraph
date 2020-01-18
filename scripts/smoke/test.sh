set -e

test/test
echo diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
