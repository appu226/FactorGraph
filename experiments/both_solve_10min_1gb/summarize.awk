BEGIN                             { file = "File";
                                    size = "Size";
                                    pi = "pi";
                                    po = "po";
                                    li = "li";
                                    lo = "lo";
                                    parsingTime = "Parsing_T";
                                    bddCreationTime = "BDD_creation_T";
                                    conjunctionTime = "Conjunction_T"; 
                                    quantificationTime = "Quantification_T";
                                    factorGraphCreationTime = "FG_creation_T";
                                    groupedVars = "Grouped_vars";
                                    groupingTime = "Grouping_T";
                                    convergenceTime = "Convergence_T";
                                    finalConjunctionTime = "FinalConjunction_T";
                                    correctness = "Correctness";
                                  }
/\/home\/parakram\/Software\/DDP/ { print file, size, pi, po, li, lo, parsingTime, bddCreationTime, conjunctionTime, quantificationTime, factorGraphCreationTime, groupedVars, groupingTime, convergenceTime, finalConjunctionTime, correctness;
                                    size = $1; 
                                    fileIdx = split($2, paths, "/");
                                    file = paths[fileIdx];
                                    parsingTime = "X";
                                    pi = "";
                                    po = "";
                                    li = "";
                                    lo = "";
                                    bddCreationTime = "";
                                    conjunctionTime = "";
                                    quantificationTime = "";
                                    factorGraphCreationTime = "";
                                    groupedVars = "";
                                    groupingTime = "";
                                    convergenceTime = "";
                                    finalConjunctionTime = "";
                                    correctness = "";
                                  }
/\[INFO\] Parsed/   { pi = $5;
                      po = $7;
                      li = $9;
                      lo = $11;
                      parsingTime = $15;
                    }
/\[INFO\] Created BDDs in the network in/ { bddCreationTime = $8; }
/\[INFO\] Created conjunction of all functions in/ { conjunctionTime = $8; }
/\[INFO\] Quantified out primary inputs to get transition relation in/ { quantificationTime = $11; }
/\[INFO\] Created factor graph with/ { numFuncs = $6;
                                       factorGraphCreationTime = $9;
                                     }
/\[INFO\] Grouped/ { groupedVars = $3; 
                     groupingTime = $6;
                   }
/\[INFO\] Factor graph messages have converged in/ { convergenceTime = $8; }
/\[INFO\] Computed final factor graph result in / { finalConjunctionTime = $8; }
/\[INFO\] The cudd result does indeed  imply the factor graph result./ { correctness = "Y"; }
/\[INFO\] The cudd result does NOT/ { correctness = "N"; }




