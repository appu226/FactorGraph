make -s -j 4 blif_solve
if [ $# -gt 0 ] && [ $1 = "all" ] #if first argument is 'all' then run all files with 10min and 1Gb limit
then
  for blif_file in /home/parakram/Software/DDP/data_sets/bddblif/*; do
    ls -sh $blif_file;
    timeout 10m blif_solve/blif_solve --verbosity INFO --factor_graph $blif_file;
  done;
else # else run a test file
  blif_solve/blif_solve --verbosity INFO --factor_graph /home/parakram/Software/DDP/data_sets/bddblif/6s365r_bdd.blif
fi

