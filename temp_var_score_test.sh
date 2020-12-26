rm -rf temp
mkdir -p temp
var_score/var_score --blif ../../data_sets/all_blifs/s400.blif --maxBddSize 1000 --approximationMethod factor_graph --verbosity INFO --mustCountNumSolutions 1 --dottyFilePrefix temp/s400 $*
sfdp -Grepulsiveforce=10 -Tsvg -O temp/*.dot
