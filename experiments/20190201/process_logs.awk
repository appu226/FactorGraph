BEGIN { print "sno,var ,func,total ,actual,factor,avg  ,num ,file";
        print "   ,node,node,reruns,reruns,graph ,iters,soln,name";
        print "   ,size,size,      ,      ,iters ,     ,    ,";
        print "---,---,---,---,---,---,---,---,---";
        sno=0;
        is_first=1;
        c=",";
     }

function print_row()
{
    if (! (is_first) )
    {
        if (actual_iterations)
          avg_iters = factor_graph_iters / actual_iterations;
        else
          avg_iters = 0;
        print sno c var_node_size c func_node_size c total_iterations c actual_iterations c factor_graph_iters c avg_iters c num_solutions c file_name;
    }
}

/^blif_solve\/blif_solve/ {
    print_row();
    ++sno; 
    file_name=$22; 
    var_node_size=$7; 
    func_node_size=$9;
    total_iterations=$11;
    actual_iterations=0;
    factor_graph_iters=0;
    num_solutions="NA"
    is_first=0;
}

/^\[INFO\] Factor graph messages have converged in [0-9]* iterations/ {
                                                                        factor_graph_iters += $8;
                                                                      }
/^\[INFO\] Over approximating method FactorGraphApprox finished with/ {
                                                                        num_solutions = $8;
                                                                      }
/^\[INFO\] Ran [0-9]* FactorGraph convergences/ {
                                                  actual_iterations = $3;
                                                }

END {
    print_row();
}
