result_dir=temp
rm -rf ${result_dir}
mkdir -p ${result_dir}
#var_score/var_score --blif ../../data_sets/all_blifs/s382.blif --verbosity INFO --approximationMethod factor_graph --maxBddSize 3300  --factorGraphMergeSize 50 --mustCountNumSolutions 1 --dottyFilePrefix ${result_dir}/s382 $*
#var_score/var_score --blif ../../data_sets/all_blifs/s400.blif --verbosity INFO --approximationMethod factor_graph --maxBddSize 1000  --factorGraphMergeSize 50 --mustCountNumSolutions 1 --dottyFilePrefix ${result_dir}/s400 $*
var_score/var_score --blif ../../data_sets/all_blifs/b10.blif  --verbosity INFO --approximationMethod factor_graph --maxBddSize 10000 --factorGraphMergeSize 50 --mustCountNumSolutions 1 --dottyFilePrefix ${result_dir}/b10 $*
#var_score/var_score --blif ../../data_sets/all_blifs/s208.blif --verbosity INFO --approximationMethod factor_graph --maxBddSize 100   --factorGraphMergeSize 50 --mustCountNumSolutions 1 --dottyFilePrefix ${result_dir}/s208 $*
sfdp -Grepulsiveforce=10 -Tsvg -O ${result_dir}/*.dot
rm ${result_dir}/*.dot
