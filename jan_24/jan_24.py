# Copyright 2024 Parakram Majumdar

# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from __future__ import annotations
import argparse
import ctypes
from dataclasses import dataclass
from datetime import datetime, timedelta
from distutils.cmd import Command
from enum import Enum
from jan_24_common import LogLevelMapping
import logging
import os
import shutil
import subprocess
import sys
import time





######## utility class for generating standard file names ########
class FileNameGen:
    root_file_name: str
    original_qdimacs: str
    bfss_input: str
    bfss_output: str
    kissat_input: str
    kissat_output: str
    final_preprocessed_result: str
    factor_graph_input: str
    factor_graph_output: str
    def __init__(self: FileNameGen, output_root: str, src_file_path: str):
        SUFFIX = ".qdimacs"
        root_file_name = os.path.basename(src_file_path)
        if root_file_name.endswith(SUFFIX):
            root_file_name = root_file_name[0:-len(SUFFIX)]
        else:
            raise RuntimeError(f"Filename {src_file_path} does not end in {SUFFIX}")
        root_file_name = os.path.join(output_root, root_file_name)
        self.root_file_name = root_file_name
        self.original_qdimacs = root_file_name + "_original.qdimacs"
        self.bfss_input = root_file_name + "_bfss_input.qdimacs"
        self.bfss_output = self.bfss_input + ".noUnary"
        self.kissat_input = root_file_name + "_kissat_input.qdimacs"
        self.kissat_output = root_file_name + "_kissat_output.qdimacs"
        self.final_preprocessed_result = root_file_name + "_preprocessed.qdimacs"
        self.factor_graph_input = root_file_name + "_factor_graph_input.qdimacs"
        self.factor_graph_output = root_file_name + "_factor_graph_output.qdimacs"








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
    largest_bdd_size: str
    largest_support_set: str
    factor_graph_timeout_seconds: str
    run_mus_tool: str
    minimalize_assignments: str

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
        ap.add_argument("--largest_bdd_size", type=int, required=False,
                        help="Largest BDD size while merging factors for factor graph algorithm",
                        default=100)
        ap.add_argument("--largest_support_set", type=int, required=False,
                        help="Largest support set while merging factors/variables for factor graph algorithm",
                        default=20)
        ap.add_argument("--factor_graph_timeout_seconds", type=int, required=False,
                        help="Timeout for factor graph (and must exploration) in seconds",
                        default=1200)
        ap.add_argument("--run_mus_tool", type=bool, required=False, default=True,
                        help="Whether to run MUST or not")
        ap.add_argument("--minimalize_assignments", type=bool, required=False, default=True,
                        help="Whether to minimalize assignments found by MUST")
        args = ap.parse_args()

        logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[args.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S',
            handlers=[logging.StreamHandler(sys.stdout)])

        return CommandLineOptions(test_case_path=args.test_case_path,
                                  output_root=args.output_root,
                                  run_bfss_preprocess=args.run_bfss_preprocess,
                                  bfss_timeout_seconds=args.bfss_timeout_seconds,
                                  run_kissat_preprocess=args.run_kissat_preprocess,
                                  kissat_timeout_seconds=args.kissat_timeout_seconds,
                                  preprocess_timeout_seconds=args.preprocess_timeout_seconds,
                                  verbosity=args.verbosity,
                                  factor_graph_bin=args.factor_graph_bin,
                                  bfss_bin=args.bfss_bin,
                                  largest_bdd_size=str(args.largest_bdd_size),
                                  largest_support_set=str(args.largest_support_set),
                                  factor_graph_timeout_seconds=str(args.factor_graph_timeout_seconds),
                                  run_mus_tool = "1" if args.run_mus_tool else "0",
                                  minimalize_assignments = "1" if args.minimalize_assignments else "0")





######## create output folder, copy test case, create a simplified test case
def prepare_output_folder(clo: CommandLineOptions) -> FileNameGen:
    os.makedirs(clo.output_root, exist_ok=True)
    fng = FileNameGen(clo.output_root, clo.test_case_path)
    shutil.copyfile(clo.test_case_path, fng.original_qdimacs)
    logging.debug(f"Test case copied from {clo.test_case_path} to {fng.original_qdimacs}")
    return fng







########          remove all quantifiers except innermost              ########
######## then add all remaining vars as outermost universal quantifier ########
def convert_qdimacs_to_bfss_input(fng: FileNameGen, clo: CommandLineOptions) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "innermost_existential"),
        "--inputFile", fng.original_qdimacs,
        "--outputFile", fng.bfss_input,
        "--addUniversalQuantifier", "1",
        "--verbosity", clo.verbosity]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)





###### time calculation functions ######
def compute_deadline(seconds_to_deadline: int) -> datetime:
    return datetime.now() + timedelta(seconds=float(seconds_to_deadline))

def remaining_time(deadline: datetime) -> float:
    return (deadline - datetime.now()).total_seconds()





def run_bfss(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime) -> int:
    time_left = min(float(clo.bfss_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.bfss_bin, 'readCnf'), os.path.basename(fng.bfss_input)]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)} at cwd {clo.output_root}")
    readCnf_process = subprocess.Popen(cmd, cwd=clo.output_root, stdout=subprocess.DEVNULL)
    try:
        return_code = readCnf_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"bfss timed out in {time_left} secs for {fng.bfss_input}")
        readCnf_process.kill()
    return return_code




def convert_bfss_output_to_kissat(fng: FileNameGen, clo: CommandLineOptions) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "remove_unaries"),
        "--inputFile", fng.bfss_output,
        "--outputFile", fng.kissat_input,
        "--verbosity", clo.verbosity]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)



def run_kissat(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime) -> int:
    time_left = min(float(clo.kissat_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.factor_graph_bin, "jan_24", "kissat_preprocess"), 
           "--inputFile", fng.kissat_input,
           "--outputFile", fng.kissat_output,
           "--verbosity", clo.verbosity]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)}")
    kissat_process = subprocess.Popen(cmd)
    try:
        return_code = kissat_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"kissat timed out in {time_left} secs for {fng.kissat_input}")
        kissat_process.kill()
    return return_code







def bfss_input_equals_kissat_output(fng: FileNameGen, clo: CommandLineOptions, c_functions: ctypes.CDLL) -> bool:
    result = c_functions.bfss_input_equals_kissat_output(bytes(fng.bfss_input, 'UTF-8'), bytes(fng.kissat_output, 'UTF-8'))
    if result == 1:
        return True
    elif result == 0:
        return False
    else:
        raise RuntimeError(f"compare bfss_input_and_kissat_output returned {result} indicating an error.")







def convert_kissat_output_to_bfss(fng: FileNameGen) -> int:
    logging.debug(f"Running command: cp {fng.kissat_input} {fng.bfss_input}")
    shutil.copy(fng.kissat_output, fng.bfss_input)
    return 0




def convert_preprocess_output_to_factor_graph(preprocess_output: str, factor_graph_input: str, clo: CommandLineOptions) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "innermost_existential"),
        "--inputFile", preprocess_output,
        "--outputFile", factor_graph_input,
        "--addUniversalQuantifier", "0",
        "--verbosity", clo.verbosity]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)





def run_factor_graph(fng: FileNameGen, clo: CommandLineOptions) -> None:
    time_left = float(clo.factor_graph_timeout_seconds)
    return_code = -1
    cmd = [os.path.join(clo.factor_graph_bin, "oct_22", "oct_22"), 
           "--inputFile", fng.factor_graph_input,
           "--outputFile", fng.factor_graph_output,
           "--verbosity", clo.verbosity,
           "--largestSupportSet", clo.largest_support_set,
           "--largestBddSize", clo.largest_bdd_size,
           "--runMusTool", clo.run_mus_tool,
           "--minimalizeAssignments", clo.minimalize_assignments]
    sp = ' '
    logging.debug(f"Running command: {sp.join(cmd)}")
    factor_graph_process = subprocess.Popen(cmd)
    try:
        return_code = factor_graph_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"factor graph timed out in {time_left} secs for {fng.factor_graph_input}")
        factor_graph_process.kill()
    return
# --largestSupportSet:	largest allowed support set size while clumping cnf factors
# --largestBddSize:	largest allowed bdd size while clumping cnf factors
# --inputFile:	Input qdimacs file with exactly one quantifier which is existential	[Mandatory]
# --verbosity:	Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)
# --computeExactUsingBdd:	Compute exact solution (default false)
# --outputFile:	Cnf file with result
# --runMusTool:	Whether to run MUS tool (default true)
# --minimalizeAssignments:	Whether to minimalize assignments found by must




########## main application ###########
def main() -> int:
    clo = CommandLineOptions.parse()
    logging.info("jan_24 Starting...")
    logging.debug(clo)

    so_file = os.path.join(clo.factor_graph_bin, "jan_24", "libcheck_progress.so")
    c_functions = ctypes.CDLL(so_file)
    c_functions.setVerbosity(bytes(clo.verbosity, "UTF-8"))

    program_start_time = time.time()
    fng = prepare_output_folder(clo)

    convert_qdimacs_to_bfss_input(fng, clo)
    input_prepared_time = time.time()
    logging.info(f"First inputs prepared in {input_prepared_time - program_start_time} sec")

    change = True
    round = 1
    preprocess_deadline = compute_deadline(clo.preprocess_timeout_seconds)
    final_preprocess_output: str = ""

    while change and remaining_time(preprocess_deadline) > 0:
        if run_bfss(fng, clo, preprocess_deadline) != 0:
            logging.debug("run_bfss gave non-zero return code")
            break
        convert_bfss_output_to_kissat(fng, clo) # no conversion required because kissat directly takes bfss_output
        final_preprocess_output = fng.kissat_input


        if run_kissat(fng, clo, preprocess_deadline) != 0:
            logging.debug("run_kissat gave non-zero return code")
            break
        change = not bfss_input_equals_kissat_output(fng, clo, c_functions)
        logging.debug(f"Progress in round {round}: {change}")
        convert_kissat_output_to_bfss(fng)
        final_preprocess_output = fng.bfss_input
        
        
        logging.debug(f"Finished pre-processing round {round}")
        round = round + 1

    preprocessing_finished_time = time.time()
    logging.info(f"Finished proprocessing in {round - 1} rounds, in {preprocessing_finished_time - input_prepared_time} seconds.")

    convert_preprocess_output_to_factor_graph(final_preprocess_output, fng.factor_graph_input, clo)
    factor_graph_input_prepared_time = time.time()
    logging.info(f"Factor graph input prepared in {factor_graph_input_prepared_time - preprocessing_finished_time} seconds")

    run_factor_graph(fng, clo)
    factor_graph_finished_time = time.time()
    logging.info(f"Factor graph and must finished in {factor_graph_finished_time - factor_graph_input_prepared_time} seconds")

    logging.info("jan_24 Done!")
    return 0



if __name__ == "__main__":
    sys.exit(main())