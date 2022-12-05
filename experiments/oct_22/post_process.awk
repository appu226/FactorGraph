function casename(file, a, n) {
    n = split(file, a, "/")
    return a[n-1]
} 
/BEGIN/ {numClausesAddedToSolution=0; disabledSets=0; numMus=0}
/Parsed qdimacs file in with/ {numClauses=$7; numVars=$10; qdimacsParsingTime=$13;}
/Created bdds in/ {bddCreationTime=$5}
/Merged to [0-9]* factors and [0-9]*variables in [0-9\.]* sec/ {mergeTime=$9; mergedFn=$4; mergedVr=substr($7, 0, length($7)-9);}
/Created factor graph in/ {factorGraphCreationTime=$6}
/Factor graph converged after/ {factorGraphIterations=$6; factorGraphConvergenceTime=$9}
/Factor graph result converted to cnf in/ {cnfConversionTime=$9}
/Adding clause/ {numClausesAddedToSolution++}
/sets from must solver/ {disabledSets += $3}
/Found MUS/ {numMus++}
END {printf("%8s | %8s | %8.2f | %8.2f | %8.2f | %8s | %8s | %8.2f | %8s | %8.2f | %8.2f | %8s | %16s | %8s | %s\n", numClauses, numVars, qdimacsParsingTime, bddCreationTime, mergeTime, mergedFn, mergedVr, factorGraphCreationTime, factorGraphIterations, factorGraphConvergenceTime, cnfConversionTime, numClausesAddedToSolution, disabledSets, numMus, casename(FILENAME))}
