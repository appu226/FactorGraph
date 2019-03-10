BEGIN { print "sno,clip ,time ,num ,file";
        print "   ,depth,taken,soln,name";
        print "   ,     ,(sec),     ,";
        print "---,---,---,---,---";
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
        print sno c clip_depth c time_taken c num_solutions c file_name;
    }
}

/^blif_solve\/blif_solve/ {
    print_row();
    ++sno; 
    file_name=$16; 
    clip_depth=$7;
    num_solutions="NA"
    is_first=0;
    time_taken="NA";
}

/^\[INFO\] Over approximating method ClippingOverApprox finished with/ {
                                                                        num_solutions = $8;
                                                                      }

/^\[INFO\] Finised over approximating method ClippingOverApprox in/ {
                                                                     time_taken = $8;
                                                                   }

END {
    print_row();
}
