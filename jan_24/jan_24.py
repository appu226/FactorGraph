from __future__ import annotations
import argparse
import ctypes
from dataclasses import dataclass
from datetime import datetime, timedelta
from distutils.cmd import Command
from enum import Enum
import logging
import os
import shutil
import subprocess
import sys





###### utility enum for parsing log verbosity ######
LogLevelMapping: dict[str, int] = {
    'QUIET': logging.CRITICAL,
    'ERROR': logging.ERROR,
    'WARNING': logging.WARN,
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG
}






######## utility class for generating standard file names ########
class FileNameGen:
    root_file_name: str
    original_qdimacs: str
    bfss_input: str
    bfss_output: str
    kissat_input: str
    kissat_output: str
    final_preprocessed_result: str
    def __init__(self: FileNameGen, root_file_name: str):
        if root_file_name.endswith(".qdimacs"):
            root_file_name = root_file_name[0:-len(".qdimacs")]
        self.root_file_name = root_file_name
        self.original_qdimacs = root_file_name + "_original.qdimacs"
        self.bfss_input = root_file_name + "_bfss_input.qdimacs"
        self.bfss_output = self.bfss_input + ".noUnary"
        self.kissat_input = self.bfss_output
        self.kissat_output = root_file_name + "_kissat_output.qdimacs"
        self.final_preprocessed_result = root_file_name + "_preprocessed.qdimacs"








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
    factor_graph_bin: str
    bfss_bin: str

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
        ap.add_argument("--factor_graph_bin", type=str, required=True,
                        help="Path to factor graph build outputs folder (e.g. build/out)")
        ap.add_argument("--bfss_bin", type=str, required=True,
                        help="Path to bfss binaries folder")
        args = ap.parse_args()

        logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[args.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S')

        return CommandLineOptions(test_case_path=args.test_case_path,
                                  output_root=args.output_root,
                                  run_bfss_preprocess=args.run_bfss_preprocess,
                                  bfss_timeout_seconds=args.bfss_timeout_seconds,
                                  run_kissat_preprocess=args.run_kissat_preprocess,
                                  kissat_timeout_seconds=args.kissat_timeout_seconds,
                                  preprocess_timeout_seconds=args.preprocess_timeout_seconds,
                                  verbosity=args.verbosity,
                                  factor_graph_bin=args.factor_graph_bin,
                                  bfss_bin=args.bfss_bin)





######## create output folder, copy test case, create a simplified test case
def prepare_output_folder(clo: CommandLineOptions) -> FileNameGen:
    os.makedirs(clo.output_root, exist_ok=True)
    new_file_root = FileNameGen(os.path.join(clo.output_root, os.path.basename(clo.test_case_path)))
    shutil.copyfile(clo.test_case_path, new_file_root.original_qdimacs)
    logging.debug(f"Test case copied from {clo.test_case_path} to {new_file_root.original_qdimacs}")
    return new_file_root







########          remove all quantifiers except innermost              ########
######## then add all remaining vars as outermost universal quantifier ########
def convert_qdimacs_to_bfss_input(fng: FileNameGen, clo: CommandLineOptions) -> None:
    subprocess.run([
        os.path.join(clo.factor_graph_bin, "jan_24", "innermost_existential"),
        "--inputFile", fng.original_qdimacs,
        "--outputFile", fng.bfss_input,
        "--addUniversalQuantifier", "1",
        "--verbosity", clo.verbosity])





###### time calculation functions ######
def compute_deadline(seconds_to_deadline: int) -> datetime:
    return datetime.now() + timedelta(seconds=float(seconds_to_deadline))

def remaining_time(deadline: datetime) -> float:
    return (deadline - datetime.now()).total_seconds()





class FinalPreprocessOutput(Enum):
    BFSS_INPUT = 0
    KISSAT_INPUT = 1





def run_bfss(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime) -> int:
    time_left = min(float(clo.bfss_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.bfss_bin, 'readCnf'), os.path.basename(fng.bfss_input)]
    logging.debug(f"Running command: {cmd}")
    readCnf_process = subprocess.Popen(cmd, cwd=clo.output_root, stdout=subprocess.DEVNULL)
    try:
        return_code = readCnf_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"bfss timed out in {time_left} secs for {fng.bfss_input}")
        readCnf_process.kill()
    return return_code





def convert_bfss_output_to_kissat(fng: FileNameGen) -> int:
    return 0 # kissat works directly on bfss output





def run_kissat(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime) -> int:
    time_left = min(float(clo.kissat_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.factor_graph_bin, "jan_24", "kissat_preprocess"), 
           "--inputFile", fng.kissat_input,
           "--outputFile", fng.kissat_output,
           "--verbosity", clo.verbosity]
    logging.debug(f"Running command: {cmd}")
    kissat_process = subprocess.Popen(cmd, cwd=clo.output_root)
    try:
        return_code = kissat_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"kissat timed out in {time_left} secs for {fng.kissat_input}")
        kissat_process.kill()
    return return_code







def compare_bfss_input_and_kissat_output(fng: FileNameGen, clo: CommandLineOptions, c_functions: ctypes.CDLL) -> bool:
    result = c_functions.compare_bfss_input_and_kissat_output(bytes(fng.bfss_input, 'UTF-8'), bytes(fng.kissat_output, 'UTF-8'))
    if result == 1:
        return True
    elif result == 0:
        return False
    else:
        raise RuntimeError(f"compare bfss_input_and_kissat_output returned {result} indicating an error.")







def convert_kissat_output_to_bfss(fng: FileNameGen) -> int:
    raise RuntimeError("compare_kissat_output_to_bfss not yet implemented")






########## main application ###########
def main() -> int:
    clo = CommandLineOptions.parse()
    logging.info("jan_24 Starting...")
    logging.debug(clo)

    so_file = os.path.join(clo.factor_graph_bin, "jan_24", "libcheck_progress.so")
    c_functions = ctypes.CDLL(so_file)
    c_functions.setVerbosity(bytes(clo.verbosity, "UTF-8"))

    fng = prepare_output_folder(clo)

    convert_qdimacs_to_bfss_input(fng, clo)

    change = True
    preprocess_deadline = compute_deadline(clo.preprocess_timeout_seconds)
    final_preprocess_output: FinalPreprocessOutput = FinalPreprocessOutput.BFSS_INPUT

    while change and remaining_time(preprocess_deadline) > 0:
        if run_bfss(fng, clo, preprocess_deadline) != 0:
            logging.debug("run_bfss gave non-zero return code")
            break
        if convert_bfss_output_to_kissat(fng) != 0:
            logging.debug("convert_bfss_output_to_kissat gave non-zero return code")
            break
        final_preprocess_output = FinalPreprocessOutput.KISSAT_INPUT
        if run_kissat(fng, clo, preprocess_deadline) != 0:
            logging.debug("run_kissat gave non-zero return code")
            break
        change = compare_bfss_input_and_kissat_output(fng, clo, c_functions)
        if convert_kissat_output_to_bfss(fng) != 0:
            logging.debug("convert_kissat_output_to_bfss gave non-zero return code")
            break
        final_preprocess_output = FinalPreprocessOutput.BFSS_INPUT
        logging.debug("Finished pre-processing round")
        

    logging.info("jan_24 Done!")
    return 0



if __name__ == "__main__":
    sys.exit(main())