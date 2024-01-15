from __future__ import annotations
import argparse
from dataclasses import dataclass
from distutils.cmd import Command
import logging
import os
import shutil
import subprocess
import sys





###### utility enum for parsing log verbosity ######
LogLevelMapping: dict[str, int] = {
    'QUIET': logging.CRITICAL,
    'ERROR': logging.ERROR,
    'WARN': logging.WARN,
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG
}






######## utility class for generating standard file names ########
class FileNameGen:
    root_file_name: str
    def __init__(self: FileNameGen, root_file_name: str):
        self.root_file_name = root_file_name

    def original_qdimacs(self: FileNameGen) -> str:
        return self.root_file_name + ".original.qdimacs"
    
    def bfss_input(self: FileNameGen) -> str:
        return self.root_file_name + ".bfss_input.qdimacs"
    
    def bfss_output(self: FileNameGen) -> str:
        return self.root_file_name + ".bfss_utput.qdimacs"







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
    verbosity: str

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
        ap.add_argument("--verbosity", type=str, required=False, default="ERROR",
                        choices=list(LogLevelMapping.keys()),
                        help="Total timeout for all pre-processing rounds")
        args = ap.parse_args()

        logging.basicConfig(
            format='%(asctime)s %(levelname)-8s %(message)s',
            level=LogLevelMapping[args.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S')

        return CommandLineOptions(test_case_path=args.test_case_path,
                                  output_root=args.output_root,
                                  run_bfss_preprocess=args.run_bfss_preprocess,
                                  bfss_timeout_seconds=args.bfss_timeout_seconds,
                                  run_kissat_preprocess=args.run_kissat_preprocess,
                                  kissat_timeout_seconds=args.kissat_timeout_seconds,
                                  preprocess_timeout_seconds=args.preprocess_timeout_seconds,
                                  verbosity=args.verbosity)





######## create output folder, copy test case, create a simplified test case
def prepare_output_folder(clo: CommandLineOptions) -> FileNameGen:
    os.makedirs(clo.output_root, exist_ok=True)
    new_file_root = FileNameGen(os.path.join(clo.output_root, os.path.basename(clo.test_case_path)))
    shutil.copyfile(clo.test_case_path, new_file_root.original_qdimacs())
    logging.debug(f"Test case copied from {clo.test_case_path} to {new_file_root.original_qdimacs()}")
    return new_file_root







########          remove all quantifiers except innermost              ########
######## then add all remaining vars as outermost universal quantifier ########
def convert_qdimacs_to_bfss_input(fng: FileNameGen, verbosity) -> None:
    subprocess.run(["build/out/jan_24/innermost_existential", "--inputFile", fng.original_qdimacs(), "--outputFile", fng.bfss_input(), "--addUniversalQuantifier", "1", "--verbosity", verbosity])





########## main application ###########
def main() -> int:
    clo = CommandLineOptions.parse()
    logging.info("jan_24 Starting...")
    logging.debug(clo)

    fng = prepare_output_folder(clo)

    convert_qdimacs_to_bfss_input(fng, clo.verbosity)

    logging.info("jan_24 Done!")
    return 0



if __name__ == "__main__":
    sys.exit(main())