#include <blif_solve_lib/log.h>
#include <iostream>
#include <stdexcept>


extern "C" {

    void setVerbosity(char const * const verbosity);

    // RETURN: 0 -> mis-match
    //         1 -> match
    //         anything else -> error
    int compare_bfss_input_and_kissat_output(char const * const bfss_input_path, char const * const kissat_output);
}


void setVerbosity(char const * const verbosity)
{
    blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity));
}



// RETURN: 0 -> mis-match
//         1 -> match
//         anything else -> error
int compare_bfss_input_and_kissat_output(char const * const bfss_input_path, char const * const kissat_output_path)
{
    blif_solve_log(DEBUG, "Comparing " << bfss_input_path << " and " << kissat_output_path);
    blif_solve_log(ERROR, "compare_bfss_input_and_kissat_output not yet implemented");
    return 2;
}