filename=$(basename $3 .qdimacs)
build/out/cnf_dump/simplify_qdimacs $3 $1/${filename}.qdimacs $2/${filename}.cnf
echo $filename

