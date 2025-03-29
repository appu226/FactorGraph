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
from jan_24_common import LogLevelMapping
import logging
import os
import shutil
import subprocess
import sys
import time




######## some debug printing utilities ########
pde = "ERROR"
pdo = "OK"
sp = ' '

def str2bool(s: str | bool) -> bool:
    if isinstance(s, bool):
        return s
    elif s.lower() in ['0', 'n', 'no', 'f', 'false']:
        return False
    elif s.lower() in ['1', 'y', 'yes', 't', 'true']:
        return True
    else:
        raise argparse.ArgumentTypeError("Not a boolean value.")

def pad(input: str, final_length: int) -> str:
    if len(input) > final_length:
        return input
    output = input
    for i in range(final_length - len(input)):
        output = output + ' '
    return output








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






######## Diagnostics about a specific qdimacs file ########
@dataclass
class QdimacsDiagnostics:
    error: bool
    numHeaderVars: int
    numHeaderQuantifiedVars: int
    numHeaderClauses: int
    numActualVars: int
    numActualQuantifiedVars: int
    numActualClauses: int

def parseDiagnostics(qdimacsFilePath: str, c_functions: ctypes.CDLL) -> QdimacsDiagnostics :
    error = ctypes.c_bool()
    numHeaderVars = ctypes.c_int()
    numHeaderQuantifiedVars = ctypes.c_int()
    numHeaderClauses = ctypes.c_int()
    numActualVars = ctypes.c_int()
    numActualQuantifiedVars = ctypes.c_int()
    numActualClauses = ctypes.c_int()
    c_functions.diagnostics(bytes(qdimacsFilePath, 'UTF-8'),
                            ctypes.byref(error),
                            ctypes.byref(numHeaderVars),
                            ctypes.byref(numHeaderQuantifiedVars),
                            ctypes.byref(numHeaderClauses),
                            ctypes.byref(numActualVars),
                            ctypes.byref(numActualQuantifiedVars),
                            ctypes.byref(numActualClauses))
    return QdimacsDiagnostics(error.value,
                              numHeaderVars.value,
                              numHeaderQuantifiedVars.value,
                              numHeaderClauses.value,
                              numActualVars.value,
                              numActualQuantifiedVars.value,
                              numActualClauses.value)

def serializeDiagnostics(diag: QdimacsDiagnostics) -> str:
    return f"Status: {pde if diag.error else pdo} NumHeaderVars: {diag.numHeaderVars} NumHeaderQuantifiedVars: {diag.numHeaderQuantifiedVars} NumHeaderClauses: {diag.numHeaderClauses} NumActualVars: {diag.numActualVars} NumActualQuantifiedVars: {diag.numActualQuantifiedVars} NumActualClauses: {diag.numActualClauses}"






######## struct to capture how many Vars/Clauses etc  ########
######## were eliminated by a SINGLE step in the algo ########
@dataclass
class QdimacsProgress:
    numVarChange: int = 0
    numQuantifiedVarChange: int = 0
    numClauseChange: int = 0
    timeTakenSeconds: float = 0






######## struct to capture how many vars/clauses etc ########
######## ALL steps in the algo have eliminated       ########
class PreprocessorProgress:
    current_diagnostics: QdimacsDiagnostics
    all_progress: dict[str, QdimacsProgress]
    c_functions: ctypes.CDLL

    def __init__(self, origQdimacsFilePath: str, c_functions: ctypes.CDLL):
        self.current_diagnostics = parseDiagnostics(origQdimacsFilePath, c_functions)
        self.all_progress = {}
        self.c_functions = c_functions
        original_operation = "original"
        logging.debug(f"[DIAG] [{pad(original_operation, 15)}] {serializeDiagnostics(self.current_diagnostics)}")

    def checkProgress(self, operation: str, qdimacsFilePath: str, timeTakenSeconds: float) -> bool:
        new_diag = parseDiagnostics(qdimacsFilePath, self.c_functions)
        logging.debug(f"[DIAG] [{pad(operation, 15)}] {serializeDiagnostics(new_diag)}")
        if new_diag.error:
            logging.error(f"Error while collecting diagnostics from {qdimacsFilePath}")
            return False
        
        if operation not in self.all_progress:
            self.all_progress[operation] = QdimacsProgress()
        operation_progress = self.all_progress[operation]

        operation_progress.numVarChange += new_diag.numActualVars - self.current_diagnostics.numActualVars
        operation_progress.numQuantifiedVarChange += new_diag.numActualQuantifiedVars - self.current_diagnostics.numActualQuantifiedVars
        operation_progress.numClauseChange += new_diag.numActualClauses - self.current_diagnostics.numActualClauses
        operation_progress.timeTakenSeconds += timeTakenSeconds

        if new_diag.numActualVars > self.current_diagnostics.numActualVars:
            raise RuntimeError(f"Number of actual variables has increased from {self.current_diagnostics.numActualVars} to {new_diag.numActualVars} in {qdimacsFilePath}")
        if new_diag.numActualQuantifiedVars > self.current_diagnostics.numActualQuantifiedVars:
            raise RuntimeError(f"Number of actual quantified variables has increased from {self.current_diagnostics.numActualQuantifiedVars} to {new_diag.numActualQuantifiedVars} in {qdimacsFilePath}")
        progress = new_diag.numActualVars < self.current_diagnostics.numActualVars or new_diag.numActualQuantifiedVars < self.current_diagnostics.numActualQuantifiedVars
        self.current_diagnostics = new_diag
        return progress
    
    def summarize(self) -> None:
        for operation in self.all_progress:
            prog = self.all_progress[operation]
            logging.info(f"[PREP] [{pad(operation, 15)}] numVarChange: {prog.numVarChange} numQuantifiedVarChange: {prog.numQuantifiedVarChange} numClauseChange: {prog.numClauseChange} timeTakenSeconds: {prog.timeTakenSeconds}")

    def isSolved(self):
        return self.current_diagnostics.numActualQuantifiedVars == 0







########### Command line options for application ##########
@dataclass
class CommandLineOptions:
    test_case_path: str
    output_root: str
    run_preprocess: bool
    bfss_timeout_seconds: int
    kissat_timeout_seconds: int
    preprocess_timeout_seconds: int
    verbosity: str
    factor_graph_bin: str
    bfss_bin: str
    largest_bdd_size: str
    largest_support_set: str
    factor_graph_timeout_seconds: str
    run_mus_tool: str
    run_factor_graph: str
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
        ap.add_argument("--run_preprocess", type=str2bool, required=False, default=True,
                        help="Whether to run Kissat+BFSS pre-processing or not")
        ap.add_argument("--bfss_timeout_seconds", type=int, required=False, default=60,
                        help="Timeout, in seconds, for a round of bfss pre-processing")
        ap.add_argument("--kissat_timeout_seconds", type=int, required=False, default=60,
                        help="Timeout, in seconds, for a round of kissat pre-processing")
        ap.add_argument("--preprocess_timeout_seconds", type=int, required=False, default=600,
                        help="Total timeout for all pre-processing rounds")
        ap.add_argument("--verbosity", type=str, required=False, default="ERROR",
                        choices=list(LogLevelMapping.keys()),
                        help="Logging verbosity")
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
        ap.add_argument("--run_mus_tool", type=str2bool, required=False, default=True,
                        help="Whether to run MUST or not")
        ap.add_argument("--run_factor_graph", type=str2bool, required=False, default=True,
                        help="Whether to run factor graph algorithm or not")
        ap.add_argument("--minimalize_assignments", type=str2bool, required=False, default=True,
                        help="Whether to minimalize assignments found by MUST")
        args = ap.parse_args()

        logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[args.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S',
            handlers=[logging.StreamHandler(sys.stdout)])

        return CommandLineOptions(test_case_path=args.test_case_path,
                                  output_root=args.output_root,
                                  run_preprocess=args.run_preprocess,
                                  bfss_timeout_seconds=args.bfss_timeout_seconds,
                                  kissat_timeout_seconds=args.kissat_timeout_seconds,
                                  preprocess_timeout_seconds=args.preprocess_timeout_seconds,
                                  verbosity=args.verbosity,
                                  factor_graph_bin=args.factor_graph_bin,
                                  bfss_bin=args.bfss_bin,
                                  largest_bdd_size=str(args.largest_bdd_size),
                                  largest_support_set=str(args.largest_support_set),
                                  factor_graph_timeout_seconds=str(args.factor_graph_timeout_seconds),
                                  run_mus_tool = "1" if bool(args.run_mus_tool) else "0",
                                  run_factor_graph = "1" if bool(args.run_factor_graph) else "0",
                                  minimalize_assignments = "1" if bool(args.minimalize_assignments) else "0")





######## create output folder, copy test case, create a simplified test case
def prepare_output_folder(clo: CommandLineOptions, c_functions: ctypes.CDLL) -> FileNameGen:
    os.makedirs(clo.output_root, exist_ok=True)
    fng = FileNameGen(clo.output_root, clo.test_case_path)
    shutil.copyfile(clo.test_case_path, fng.original_qdimacs)
    logging.debug(f"Test case copied from {clo.test_case_path} to {fng.original_qdimacs}")
    return fng







########          remove all quantifiers except innermost              ########
######## then add all remaining vars as outermost universal quantifier ########
def convert_qdimacs_to_bfss_input(fng: FileNameGen, clo: CommandLineOptions, c_functions: ctypes.CDLL) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "innermost_existential"),
        "--inputFile", fng.original_qdimacs,
        "--outputFile", fng.bfss_input,
        "--addUniversalQuantifier", "1",
        "--verbosity", clo.verbosity]
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)





###### time calculation functions ######
def compute_deadline(seconds_to_deadline: int) -> datetime:
    return datetime.now() + timedelta(seconds=float(seconds_to_deadline))

def remaining_time(deadline: datetime) -> float:
    return (deadline - datetime.now()).total_seconds()





def run_bfss(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime, c_functions: ctypes.CDLL) -> int:
    time_left = min(float(clo.bfss_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.bfss_bin, 'readCnf'), os.path.basename(fng.bfss_input)]
    logging.debug(f"Running command: {sp.join(cmd)} at cwd {clo.output_root}")
    readCnf_process = subprocess.Popen(cmd, cwd=clo.output_root, stdout=subprocess.DEVNULL)
    try:
        return_code = readCnf_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"bfss timed out in {time_left} secs for {fng.bfss_input}")
        readCnf_process.kill()
    return return_code




def convert_bfss_output_to_kissat(fng: FileNameGen, clo: CommandLineOptions, c_functions: ctypes.CDLL) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "remove_unaries"),
        "--inputFile", fng.bfss_output,
        "--outputFile", fng.kissat_input,
        "--verbosity", clo.verbosity]
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)



def run_kissat(fng: FileNameGen, clo: CommandLineOptions, deadline: datetime, c_functions: ctypes.CDLL) -> int:
    time_left = min(float(clo.kissat_timeout_seconds), remaining_time(deadline))
    if time_left < 0:
        return -1
    return_code = -1
    cmd = [os.path.join(clo.factor_graph_bin, "jan_24", "kissat_preprocess"), 
           "--inputFile", fng.kissat_input,
           "--outputFile", fng.kissat_output,
           "--verbosity", clo.verbosity]
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
    logging.debug(f"Running command: cp {fng.kissat_output} {fng.bfss_input}")
    shutil.copy(fng.kissat_output, fng.bfss_input)
    return 0




def convert_preprocess_output_to_factor_graph(preprocess_output: str, factor_graph_input: str, clo: CommandLineOptions) -> None:
    cmd = [
        os.path.join(clo.factor_graph_bin, "jan_24", "innermost_existential"),
        "--inputFile", preprocess_output,
        "--outputFile", factor_graph_input,
        "--addUniversalQuantifier", "0",
        "--verbosity", clo.verbosity]
    logging.debug(f"Running command: {sp.join(cmd)}")
    subprocess.run(cmd)





def run_factor_graph(fng: FileNameGen, clo: CommandLineOptions) -> int:
    time_left = float(clo.factor_graph_timeout_seconds)
    return_code = -1
    cmd = [os.path.join(clo.factor_graph_bin, "oct_22", "oct_22"), 
           "--inputFile", fng.factor_graph_input,
           "--outputFile", fng.factor_graph_output,
           "--verbosity", clo.verbosity,
           "--largestSupportSet", clo.largest_support_set,
           "--largestBddSize", clo.largest_bdd_size,
           "--runMusTool", clo.run_mus_tool,
           "--runFg", clo.run_factor_graph,
           "--minimalizeAssignments", clo.minimalize_assignments]
    logging.debug(f"Running command: {sp.join(cmd)}")
    factor_graph_process = subprocess.Popen(cmd)
    try:
        return_code = factor_graph_process.wait(time_left)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"factor graph timed out in {time_left} secs for {fng.factor_graph_input}")
        factor_graph_process.kill()
    return return_code


#TODO: check return code at every stage

########## main application ###########
def main() -> int:
    clo = CommandLineOptions.parse()
    logging.info("jan_24 Starting...")
    logging.debug(clo)

    so_file = os.path.join(clo.factor_graph_bin, "jan_24", "libcheck_progress.so")
    c_functions = ctypes.CDLL(so_file)
    c_functions.setVerbosity(bytes(clo.verbosity, "UTF-8"))

    program_start_time = time.time()
    fng = prepare_output_folder(clo, c_functions)
    preprog = PreprocessorProgress(fng.original_qdimacs, c_functions)
    logging.info(f"[original] {serializeDiagnostics(parseDiagnostics(fng.original_qdimacs, c_functions))}")

    if clo.run_preprocess:
        convert_qdimacs_to_bfss_input(fng, clo, c_functions)
        input_prepared_time = time.time()
        logging.debug(f"First inputs prepared in {input_prepared_time - program_start_time} sec")
        preprog.checkProgress("init", fng.bfss_input, input_prepared_time - program_start_time)

        change = True
        round = 1
        preprocess_deadline = compute_deadline(clo.preprocess_timeout_seconds)
        final_preprocess_output: str = ""

        while change and remaining_time(preprocess_deadline) > 0:
            change = False



            if preprog.isSolved():
                logging.info("skipping run_bfss as problem is already solved")
                break
            t1 = time.time()
            if run_bfss(fng, clo, preprocess_deadline, c_functions) != 0:
                logging.info("run_bfss gave non-zero return code")
                break
            convert_bfss_output_to_kissat(fng, clo, c_functions) # remove unaries
            t2 = time.time()
            change = preprog.checkProgress("bfss", fng.kissat_input, t2 - t1) or change
            final_preprocess_output = fng.kissat_input



            if preprog.isSolved():
                logging.info("skipping run_kissat as problem is already solved")
                break
            t1 = time.time()
            if run_kissat(fng, clo, preprocess_deadline, c_functions) != 0:
                logging.info("run_kissat gave non-zero return code")
                break
            logging.debug(f"Progress in round {round}: {change}")
            convert_kissat_output_to_bfss(fng)
            t2 = time.time()
            change = preprog.checkProgress("kissat", fng.bfss_input, t2 - t1) or change
            final_preprocess_output = fng.bfss_input
            
            
            logging.debug(f"Finished pre-processing round {round}")
            round = round + 1

        preprocessing_finished_time = time.time()
        logging.info(f"Finished proprocessing in {round - 1} rounds, in {preprocessing_finished_time - input_prepared_time} seconds.")
        preprog.summarize()
    else:
        logging.info("Skipping pre-processing as requested")
        preprocessing_finished_time = time.time()
        final_preprocess_output = fng.original_qdimacs

    if preprog.current_diagnostics.numActualQuantifiedVars > 0:
        convert_preprocess_output_to_factor_graph(final_preprocess_output, fng.factor_graph_input, clo)
        factor_graph_input_prepared_time = time.time()
        logging.info(f"Factor graph input prepared in {factor_graph_input_prepared_time - preprocessing_finished_time} seconds")

        run_factor_graph(fng, clo)
        factor_graph_finished_time = time.time()
        logging.info(f"Factor graph and must finished in {factor_graph_finished_time - factor_graph_input_prepared_time} seconds")
    else:
        shutil.copy(final_preprocess_output, fng.factor_graph_output)
        logging.info("Factor graph skipped as pre-processing solved the problem")

    logging.info("jan_24 Done!")
    return 0



if __name__ == "__main__":
    sys.exit(main())