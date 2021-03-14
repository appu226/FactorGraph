set -e

build/out/test/test1
echo diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
diff temp/testCnfDump.dimacs test/expected_outputs/testCnfDump.dimacs
