set -e

echo testing simplify_qdimacs
build/out/cnf_dump/simplify_qdimacs test/data/simplify_qdimacs.qdimacs temp/simplify_qdimacs.qdimacs temp/simplify_qdimacs.cnf
echo diff temp/simplify_qdimacs.qdimacs test/expected_outputs/simplify_qdimacs.qdimacs
diff temp/simplify_qdimacs.qdimacs test/expected_outputs/simplify_qdimacs.qdimacs
echo diff temp/simplify_qdimacs.cnf test/expected_outputs/simplify_qdimacs.cnf
diff temp/simplify_qdimacs.cnf test/expected_outputs/simplify_qdimacs.cnf
echo SUCCESS
