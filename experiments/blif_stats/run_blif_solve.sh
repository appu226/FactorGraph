make -C ../.. -s -j 4 blif_solve
for blif_file in /home/parakram/Software/DDP/data_sets/bddblif/*; do
  ../../blif_solve/blif_solve --verbosity INFO $blif_file;
done;

