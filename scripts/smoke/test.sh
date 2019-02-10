set -e

test/test
diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
