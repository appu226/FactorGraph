from __future__ import annotations
import argparse
from dataclasses import dataclass
import sys





########### Command line options for application ##########
@dataclass
class CommandLineOptions:
    test_case_path: str
    output_root: str
    run_bfss_preprocess: bool
    bfss_timeout_seconds: int
    run_kissat_preprocess: bool
    kissat_timeout_seconds: int
    preprocess_timeout_seconds: int

    @staticmethod
    def parse() -> CommandLineOptions:
        ap = argparse.ArgumentParser(
            prog="jan_24", 
            description="Generate experimental results for Quantified Boolean Elimination")
        ap.add_argument("--test_case_path", type=str, required=True,
                        help="Path to test case QDimacs file")
        ap.add_argument("--output_root", type=str, required=True,
                        help="Path to results folder")
        ap.add_argument("--run_bfss_preprocess", type=bool, required=False, default=True,
                        help="Whether to run BFSS pre-processing or not")
        ap.add_argument("--bfss_timeout_seconds", type=int, required=False, default=60,
                        help="Timeout, in seconds, for a round of bfss pre-processing")
        ap.add_argument("--run_kissat_preprocess", type=bool, required=False, default=True,
                        help="Whether to run Kissat pre-processing or not")
        ap.add_argument("--kissat_timeout_seconds", type=int, required=False, default=60,
                        help="Timeout, in seconds, for a round of kissat pre-processing")
        ap.add_argument("--preprocess_timeout_seconds", type=int, required=False, default=600,
                        help="Total timeout for all pre-processing rounds")
        ap.parse_args()
        return CommandLineOptions(test_case_path=ap.test_case_path,
                                  output_root=ap.output_root,
                                  run_bfss_preprocess=ap.run_bfss_preprocess,
                                  bfss_timeout_seconds=ap.bfss_timeout_seconds,
                                  run_kissat_preprocess=ap.run_kissat_preprocess,
                                  kissat_timeout_seconds=ap.kissat_timeout_seconds,
                                  preprocess_timeout_seconds=ap.preprocess_timeout_seconds)






########## main application ###########
def main() -> int:
    clo = CommandLineOptions.parse()
    print(clo)
    return 0



if __name__ == "__main__":
    sys.exit(main())