#!/bin/bash
export verb=INFO

build/out/jan_24/factorization_debugger --inputFile temp/Factorization_factorization8_original.qdimacs --bitVars 87,88,89,90,91,92,93,94 --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/innermost_existential --inputFile temp/Factorization_factorization8_original.qdimacs --outputFile temp/Factorization_factorization8_bfss_input.qdimacs --addUniversalQuantifier 1 --verbosity ${verb}
build/out/jan_24/factorization_debugger --inputFile temp/Factorization_factorization8_bfss_input.qdimacs --bitVars 87,88,89,90,91,92,93,94 --verbosity ${verb}

cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
build/out/jan_24/factorization_debugger --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --bitVars 87,88,89,90,91,92,93,94 --verbosity ${verb}

/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
cd temp; /home/parakram/Software/DDP/FactorGraph/jan_24/bfss/bin/readCnf Factorization_factorization8_bfss_input.qdimacs; cd ..
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/remove_unaries --inputFile temp/Factorization_factorization8_bfss_input.qdimacs.noUnary --outputFile temp/Factorization_factorization8_kissat_input.qdimacs --verbosity ${verb}
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/kissat_preprocess --inputFile temp/Factorization_factorization8_kissat_input.qdimacs --outputFile temp/Factorization_factorization8_kissat_output.qdimacs --verbosity ${verb}
cp temp/Factorization_factorization8_kissat_input.qdimacs temp/Factorization_factorization8_bfss_input.qdimacs
/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/jan_24/innermost_existential --inputFile temp/Factorization_factorization8_bfss_input.qdimacs --outputFile temp/Factorization_factorization8_factor_graph_input.qdimacs --addUniversalQuantifier 0 --verbosity ${verb}
build/out/jan_24/factorization_debugger --inputFile temp/Factorization_factorization8_factor_graph_input.qdimacs --bitVars 87,88,89,90,91,92,93,94 --verbosity ${verb}

/home/parakram/Software/DDP/FactorGraph/jan_24/FactorGraph/build/out/oct_22/oct_22 --inputFile temp/Factorization_factorization8_factor_graph_input.qdimacs --outputFile temp/Factorization_factorization8_factor_graph_output.qdimacs --verbosity ${verb} --largestSupportSet 20 --largestBddSize 100 --runMusTool 1 --minimalizeAssignments 1
