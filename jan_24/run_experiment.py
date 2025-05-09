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
from dataclasses import dataclass
from jan_24_common import LogLevelMapping
import logging
import os
import subprocess
import sys
from multiprocessing.pool import ThreadPool




expt_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def str2bool(s: str | bool) -> bool:
    if isinstance(s, bool):
        return s
    elif s.lower() in ['0', 'n', 'no', 'f', 'false']:
        return False
    elif s.lower() in ['1', 'y', 'yes', 't', 'true']:
        return True
    else:
        raise argparse.ArgumentTypeError("Not a boolean value.")



########### Command line options for application ##########
@dataclass
class CommandLineOptions:
    test_cases: list[str]
    input_folder: str
    output_folder: str
    num_processes: int
    run_preprocess: bool
    run_bfss: bool
    bfss_timeout_seconds: int
    kissat_timeout_seconds: int
    preprocess_timeout_seconds: int
    verbosity: str
    largest_bdd_size: str
    largest_support_set: str
    factor_graph_timeout_seconds: int
    run_mus_tool: bool
    run_factor_graph: bool
    minimalize_assignments: bool


    def command(self: CommandLineOptions, test_case: str) -> list[str]:
            result: list[str] = [
                "python3", os.path.join(expt_root, "FactorGraph", "jan_24", "jan_24.py"),
                "--test_case_path", os.path.join(self.input_folder, test_case),
                "--output_root", self.output_folder,
                "--run_preprocess", "1" if self.run_preprocess else "0",
                "--run_bfss", "1" if self.run_bfss else "0",
                "--bfss_timeout_seconds", str(self.bfss_timeout_seconds),
                "--kissat_timeout_seconds", str(self.kissat_timeout_seconds),
                "--preprocess_timeout_seconds", str(self.preprocess_timeout_seconds),
                "--verbosity", self.verbosity,
                "--factor_graph_bin", os.path.join(expt_root, "FactorGraph", "build", "out"),
                "--bfss_bin", os.path.join(expt_root, "bfss", "bin"),
                "--largest_bdd_size", str(self.largest_bdd_size),
                "--largest_support_set", self.largest_support_set,
                "--factor_graph_timeout_seconds", str(self.factor_graph_timeout_seconds),
                "--run_mus_tool", "1" if self.run_mus_tool else "0",
                "--run_factor_graph", "1" if self.run_factor_graph else "0",
                "--minimalize_assignments", "1" if self.minimalize_assignments else "0"
            ]
            return result







def parse_filenames(argv: list[str]) -> list[str]:
    result: list[str] = []
    for entry in argv:
        file_path = os.path.join(expt_root, "FactorGraph", "jan_24", entry)
        if os.path.exists(file_path) and os.path.isfile(file_path):
            with open(file_path) as fin:
                result.extend([x.strip() for x in fin.readlines() if not x.strip() == "" and not x.strip().startswith("#")])
        else:
            result.append(entry)
    return result






def parse_args(argv: list[str]) -> CommandLineOptions:
    ap = argparse.ArgumentParser("run_experiment.py", description="Run multiple test cases")
    ap.add_argument("test_cases", metavar='T', type=str, nargs='+',
                    help="Individual qdimacs test case names, or filename with list of test cases.")
    ap.add_argument("--input_folder", type=str, required=False, default=os.path.join(expt_root, "all_test_cases"),
                    help="Input folder with original qdimacs files.")
    ap.add_argument("--output_folder", type=str, required=True, default=os.path.join(expt_root, "FactorGraph", "experiments", "jan_24", "results"),
                    help="Output folder to put results in")
    ap.add_argument("--num_processes", type=int, help="Number of processes to run in parallel.", required=False, default=os.cpu_count())
    ap.add_argument("--run_preprocess", type=str2bool, required=False, default=True,
                    help="Whether to run pre-processing or not")
    ap.add_argument("--run_bfss", type=str2bool, required=False, default=True,
                    help="Whether to run BFSS pre-processing or not")
    ap.add_argument("--bfss_timeout_seconds", type=int, required=False, default=60,
                    help="Timeout, in seconds, for a round of bfss pre-processing")
    ap.add_argument("--kissat_timeout_seconds", type=int, required=False, default=60,
                    help="Timeout, in seconds, for a round of kissat pre-processing")
    ap.add_argument("--preprocess_timeout_seconds", type=int, required=False, default=600,
                    help="Total timeout for all pre-processing rounds")
    ap.add_argument("--verbosity", type=str, required=False, default="ERROR",
                    choices=list(LogLevelMapping.keys()),
                    help="Total timeout for all pre-processing rounds")
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
                    help="Whether to run factor graph or not")
    ap.add_argument("--minimalize_assignments", type=bool, required=False, default=True,
                    help="Whether to minimalize assignments found by MUST")
    args = ap.parse_args(argv[1:])
    os.makedirs(args.output_folder, exist_ok=True)
    logging.basicConfig(
        format='[%(levelname)s] [%(asctime)s] %(message)s',
        level=LogLevelMapping[args.verbosity],
        datefmt='%Y-%m-%d %H:%M:%S',
        handlers=[logging.StreamHandler(sys.stdout)])
    return CommandLineOptions(
        test_cases=parse_filenames(args.test_cases),
        input_folder=args.input_folder,
        output_folder=args.output_folder,
        num_processes=args.num_processes,
        run_preprocess=args.run_preprocess,
        run_bfss=args.run_bfss,
        bfss_timeout_seconds=int(args.bfss_timeout_seconds),
        kissat_timeout_seconds=int(args.kissat_timeout_seconds),
        preprocess_timeout_seconds=int(args.preprocess_timeout_seconds),
        verbosity=args.verbosity,
        largest_bdd_size=str(args.largest_bdd_size),
        largest_support_set=str(args.largest_support_set),
        factor_graph_timeout_seconds=int(args.factor_graph_timeout_seconds),
        run_mus_tool = args.run_mus_tool,
        run_factor_graph = args.run_factor_graph,
        minimalize_assignments = args.minimalize_assignments)





def run_test_case(input: tuple[str, CommandLineOptions]) -> int:
    test_case, clo = input
    cmd = clo.command(test_case)
    err_file = os.path.join(clo.output_folder, test_case + ".err")
    log_file = os.path.join(clo.output_folder, test_case + ".log")
    sp = ' '
    logging.debug(f"Executing for {test_case}\n\tcommand = {sp.join(cmd)}\n\tlog_file={log_file}\n\terror={err_file}")
    with open(log_file, 'w') as log_handle:
        with open(err_file, 'w') as err_handle:
            jan_24_process = subprocess.Popen(cmd, stdout=log_handle, stderr=err_handle)
    return_code = -1
    timeout = clo.preprocess_timeout_seconds + clo.factor_graph_timeout_seconds + 5
    try:
        return_code = jan_24_process.wait(timeout)
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"jan_24 timed out in {timeout} secs for {test_case}")
        jan_24_process.kill()
    except:
        return_code = -1
        logging.info(f"jan_24 failed with unexpected error for {test_case}")
    return return_code
    





def main(argv: list[str]) -> int:
    clo = parse_args(argv)
    logging.info("Starting jan_24/run_experiment.py")
    logging.debug(f"Found {len(clo.test_cases)} test cases: {clo.test_cases}")
    logging.debug(f"Initialising {clo.num_processes} threads")
    pool = ThreadPool(clo.num_processes)
    ret_codes: list[int] = pool.imap_unordered(run_test_case, [(test_case, clo) for test_case in clo.test_cases])
    final_ret_code = 0
    for ret_code in ret_codes:
        if ret_code != 0:
            final_ret_code = ret_code
    logging.info("Finished jan_24/run_experiment.py")
    return final_ret_code






if __name__ == "__main__":
    sys.exit(main(sys.argv))
