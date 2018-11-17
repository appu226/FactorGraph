make -s -j 4 blif_solve
if [ $# -gt 0 ] && [ $1 = "all" ] #if first argument is 'all' then run all files with 10min and 1Gb limit
then
  for blif_file in /home/parakram/Software/DDP/data_sets/bddblif/*; do
    ls -sh $blif_file;
    timeout 10m blif_solve/blif_solve $blif_file;
  done;
elif [ $# -eq 0 ] # else run a test file
then
  blif_solve/blif_solve --verbosity INFO --cudd /home/parakram/Software/DDP/data_sets/bddblif/6s365r_bdd.blif;
else
  blif_solve/blif_solve $@;
fi

