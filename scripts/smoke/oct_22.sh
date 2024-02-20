set -e

echo solving 8 bit factorization
build/out/oct_22/oct_22 --largestSupportSet 90 --largestBddSize 5000 --inputFile test/data/Factorization_factorization8_factor_graph_input.qdimacs --outputFile temp/Factorization_factorization8_factor_graph_output.qdimacs --runMusTool 1 --minimalizeAssignments 1 --verbosity QUIET

echo Checking 8 bit factors
build/out/jan_24/factorization_debugger --inputFile temp/Factorization_factorization8_factor_graph_output.qdimacs --bitVars 87,88,89,90,91,92,93,94 --verbosity QUIET

