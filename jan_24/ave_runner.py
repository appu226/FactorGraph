#!/usr/bin/env python3

import argparse
import sys
import os
import subprocess
import logging
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from jan_24_common import add_padding



# [DEBUG] avemain CLO parsed: { inputFile: tmp/ave_runner_test/inputs/Arithmetic_intermediate32_kissat_output.qdimacs, outputFile: tmp/ave_runner_test/outputs/Arithmetic_intermediate32_kissat_output.qdimacs.cnf, maxClauseTreeSize: 5, timeoutSeconds: 30 }
# [INFO] Parsed qdimacs file with 803 clauses and 319 variables.
# [INFO] AVE: Time limit reached, terminating further processing.
# [INFO] Finished ave_main on tmp/ave_runner_test/inputs/Arithmetic_intermediate32_kissat_output.qdimacs
# [INFO] Wrote result to tmp/ave_runner_test/outputs/Arithmetic_intermediate32_kissat_output.qdimacs.cnf in 0.00036 sec.
# [INFO] Wrote result with 0 clauses to tmp/ave_runner_test/outputs/Arithmetic_intermediate32_kissat_output.qdimacs.cnf
@dataclass
class TestCaseResult:
    name: str
    return_code: int
    num_clauses: int
    num_variables: int
    timed_out: bool
    result_num_clauses: int


# Import from jan_24_common
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from jan_24_common import LogLevelMapping

def run_test_case(test_case, args, output_root, logger) -> TestCaseResult:
    """Execute a single test case and handle output redirection."""
    test_case_name = test_case.name
    output_file = os.path.join(output_root, test_case_name + ".cnf")
    log_file = os.path.join(output_root, test_case_name + ".log")
    err_file = os.path.join(output_root, test_case_name + ".err")
    
    cmd = [
        args.ave_binary,
        "--inputFile", str(test_case),
        "--outputFile", output_file,
        "--maxClauseTreeSize", str(args.max_clause_tree_size),
        "--logVerbosity", args.log_verbosity,
        "--timeoutSeconds", str(args.timeout_seconds)
    ]
    
    logger.info(f"Running: {' '.join(cmd)}")
    try:
        with open(log_file, 'w') as log_f, open(err_file, 'w') as err_f:
            result = subprocess.run(cmd, stdout=log_f, stderr=err_f)
        if result.returncode != 0:
            logger.warning(f"Test case {test_case_name} exited with code {result.returncode}")
    except Exception as e:
        logger.error(f"Error running test case {test_case_name}: {e}")
    # parse log file to get num_clauses, num_variables, timed_out, result_num_clauses
    with open(log_file, 'r') as log_f:
        log_contents = log_f.read()
        num_clauses = 0
        num_variables = 0
        timed_out = False
        result_num_clauses = 0
        for line in log_contents.splitlines():
            if "Parsed qdimacs file with" in line:
                parts = line.split()
                num_clauses = int(parts[5])
                num_variables = int(parts[8].rstrip('.'))
            if "Time limit reached" in line:
                timed_out = True
            if "Wrote result with" in line:
                parts = line.split()
                result_num_clauses = int(parts[4])
    return TestCaseResult(
        name=test_case_name,
        return_code=result.returncode,
        num_clauses=num_clauses,
        num_variables=num_variables,
        timed_out=timed_out,
        result_num_clauses=result_num_clauses
    )

def main():
    parser = argparse.ArgumentParser(
        description="Run approx_var_elim on a set of qdimacs benchmarks"
    )
    
    # Mandatory arguments
    parser.add_argument(
        "--input_root",
        type=str,
        help="Path to folder with qdimacs files"
    )
    parser.add_argument(
        "--output_root",
        type=str,
        help="Path to folder where results and logs should be written"
    )
    parser.add_argument(
        "--log_verbosity",
        type=str,
        choices=["QUIET", "ERROR", "WARNING", "INFO", "DEBUG"],
        help="Log verbosity level"
    )
    parser.add_argument(
        "--ave_binary",
        type=str,
        help="Path to ave executable"
    )
    
    # Optional arguments
    parser.add_argument(
        "--max_clause_tree_size",
        type=int,
        default=0,
        help="Maximum clause tree size during approx var elim (default: 0 for no limit)"
    )
    parser.add_argument(
        "--timeout_seconds",
        type=int,
        default=0,
        help="Timeout in seconds for each test case (default: 0 for no limit)"
    )
    
    # Get number of CPUs for default and validation
    num_cpus = os.cpu_count()
    if num_cpus is None:
        num_cpus = 1
    default_num_processes = max(1, num_cpus - 2)
    
    parser.add_argument(
        "--num_processes",
        type=int,
        default=default_num_processes,
        help=f"Number of parallel processes to use (default: {default_num_processes}, must be in range [1, {num_cpus}])"
    )
    
    args = parser.parse_args()
    
    # Validate inputs
    if not os.path.isdir(args.input_root):
        print(f"Error: input_root '{args.input_root}' is not a directory", file=sys.stderr)
        sys.exit(1)
    
    if not os.path.isfile(args.ave_binary):
        print(f"Error: ave_binary '{args.ave_binary}' not found", file=sys.stderr)
        sys.exit(1)
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_root, exist_ok=True)
    
    # Set up logging
    log_file_path = os.path.join(args.output_root, "run.log")
    
    # Validate log verbosity using LogLevelMapping
    try:
        log_level = LogLevelMapping[args.log_verbosity]
    except KeyError:
        print(f"Error: Invalid log_verbosity '{args.log_verbosity}'", file=sys.stderr)
        sys.exit(1)
    
    # Initialize logger
    logger = logging.getLogger("ave_runner")
    logger.setLevel(log_level)
    
    # File handler
    file_handler = logging.FileHandler(log_file_path)
    file_handler.setLevel(log_level)
    file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)
    
    # Stderr handler with ERROR level
    stderr_handler = logging.StreamHandler(sys.stderr)
    stderr_handler.setLevel(logging.ERROR)
    stderr_formatter = logging.Formatter('%(levelname)s - %(message)s')
    stderr_handler.setFormatter(stderr_formatter)
    logger.addHandler(stderr_handler)
    
    print(f"Log file: {log_file_path}")
    
    # Validate num_processes
    if args.num_processes < 1 or args.num_processes > num_cpus:
        logger.error(f"--num_processes must be in range [1, {num_cpus}], got {args.num_processes}")
        sys.exit(1)
    
    logger.info(f"Input root: {args.input_root}")
    logger.info(f"Output root: {args.output_root}")
    logger.info(f"Log verbosity: {args.log_verbosity} (level: {log_level})")
    logger.info(f"Ave binary: {args.ave_binary}")
    logger.info(f"Max clause tree size: {args.max_clause_tree_size}")
    logger.info(f"Timeout seconds: {args.timeout_seconds}")
    logger.info(f"Number of processes: {args.num_processes}")
    
    # Find all qdimacs files in input_root
    input_path = Path(args.input_root)
    qdimacs_files = sorted(input_path.glob("*.qdimacs"))
    
    logger.info(f"Found {len(qdimacs_files)} qdimacs files")
    
    # Collect all test cases into a list
    test_cases = list(qdimacs_files)
    
    # Execute test cases in parallel using a pool of executors
    results: list[TestCaseResult] = []
    with ThreadPoolExecutor(max_workers=args.num_processes) as executor:
        futures = [executor.submit(run_test_case, test_case, args, args.output_root, logger) for test_case in test_cases]
        for future in futures:
            results.append(future.result())
    

    # Summarize results into a table
    summary_data = [
        ["Test Case", "Return Code", "Input Clauses", "Input Variables", "Output Clauses", "Timed Out"]
    ]
    for result in results:
        summary_data.append([
            result.name,
            str(result.return_code),
            str(result.num_clauses),
            str(result.num_variables),
            str(result.result_num_clauses),
            "Yes" if result.timed_out else "No"
        ])
    add_padding(summary_data)
    summary_file_path = os.path.join(args.output_root, "summary.txt")
    with open(summary_file_path, 'w') as summary_file:
        for row in summary_data:
            summary_file.write(" | ".join(row) + "\n")
    logger.info(f"Summary written to {summary_file_path}")
    
    
    print("ave_runner completed.")

if __name__ == "__main__":
    main()
