import argparse
from dataclasses import dataclass
from jan_24_common import LogLevelMapping, add_padding
import logging
from multiprocessing.pool import ThreadPool
import os
import shutil
import subprocess
import sys



comma = ','


@dataclass
class CliArgs:
    caqe_executable_path: str
    timeout_seconds: int
    input_folder: str
    output_folder: str
    test_case_names: list[str]
    num_processes: int





def parse_cli_args(argv: list[str]) -> CliArgs:
    arg_parser = argparse.ArgumentParser(description="Take a list of qdimacs test cases and run caqe on them")
    arg_parser.add_argument("--caqe_executable_path", type=str, required=True, help="full path to caqe executable")
    arg_parser.add_argument("--timeout_seconds", type=int, required=False, default=60, help="Timeout (in seconds) for caqe invocations")
    arg_parser.add_argument("--input_folder", type=str, required=True, help="Path to folder with test cases")
    arg_parser.add_argument("--output_folder", type=str, required=True, help="Path to write results to")
    arg_parser.add_argument("--test_case_list_path", type=str, required=False, default="", help="Path to file with one test case name per line")
    arg_parser.add_argument("--test_case_names", type=str, required=False, default="", help="Comma separated list of test case names")
    arg_parser.add_argument("--num_processes", type=int, required=False, default=os.cpu_count())
    arg_parser.add_argument("--verbosity", type=str, required=False, default="ERROR", choices=list(LogLevelMapping.keys()), help="Logging verbosity")

    parsed = arg_parser.parse_args(argv[1:])

    test_case_names: list[str] = parsed.test_case_names.split(",")
    if parsed.test_case_list_path != "":
        with open(parsed.test_case_list_path) as fin:
            test_case_names.extend(fin.readlines())
    test_case_names = [line.strip() for line in test_case_names]
    test_case_names = [line for line in test_case_names if line != "" and not line.startswith("#")]

    if len(test_case_names) == 0:
        raise RuntimeError("No test cases found")
    
    logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[parsed.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S',
            handlers=[logging.StreamHandler(sys.stdout)])

    logging.info(f"Processing {len(test_case_names)} test cases")
    logging.debug(f"Test case names: {comma.join(test_case_names)}")

    return CliArgs(
        caqe_executable_path=parsed.caqe_executable_path,
        timeout_seconds=int(parsed.timeout_seconds),
        input_folder=parsed.input_folder,
        output_folder=parsed.output_folder,
        test_case_names=test_case_names,
        num_processes=parsed.num_processes
    )








def run_test_case(input: tuple[str, CliArgs]) -> int:
    test_case, clo = input
    shutil.copy(os.path.join(clo.input_folder, test_case), clo.output_folder)
    cmd = [clo.caqe_executable_path, os.path.join(clo.output_folder, test_case)]
    err_file = os.path.join(clo.output_folder, test_case + ".err")
    log_file = os.path.join(clo.output_folder, test_case + ".log")
    sp = ' '
    
    logging.debug(f"Executing for {test_case}\n\tcommand = {sp.join(cmd)}\n\tlog_file={log_file}\n\terror={err_file}")
    with open(log_file, 'w') as log_handle:
        with open(err_file, 'w') as err_handle:
            caqe_process = subprocess.Popen(cmd, stdout=log_handle, stderr=err_handle)
    return_code = -1
    timeout = clo.timeout_seconds
    try:
        return_code = caqe_process.wait(timeout)
        if return_code == 10:
            logging.info(f"test case {test_case} was SATisfiable")
        elif return_code == 20:
            logging.info(f"test case {test_case} was UNSATisfiable")
        else:
            logging.error(f"test case {test_case} finished with unknown error code {return_code}")
    except subprocess.TimeoutExpired:
        return_code = -1
        logging.info(f"caqe timed out in {timeout} secs for {test_case}")
        caqe_process.kill()
    return return_code








def summarize_results(ret_codes: list[int], cli_args: CliArgs) -> None:
    ret_code_meanings: dict[str, str] = {10: "SAT", 20: "UNSAT", -1: "TIMEOUT"}
    result_table: list[list[str]] = [[test_case_name, ret_code_meanings.get(ret_code, "FAIL")] for test_case_name, ret_code in zip(cli_args.test_case_names, ret_codes)]
    result_table = [["Test Case", "Result"]] + result_table
    add_padding(result_table)
    summary_file = os.path.join(cli_args.output_folder, "summary.txt")
    with open(summary_file, 'w') as fout:
        for row in result_table:
            fout.write(" | ".join(row) + "\n")
    logging.debug(f"Summary written to {summary_file}")







def main(argv: list[str]) -> None:
    cli_args = parse_cli_args(argv)
    logging.info("Starting jan_24/caqe_runner.py")
    logging.debug(f"Found {len(cli_args.test_case_names)} test cases: {cli_args.test_case_names}")
    logging.debug(f"Initialising {cli_args.num_processes} threads")
    os.makedirs(cli_args.output_folder, exist_ok=True)
    pool = ThreadPool(cli_args.num_processes)
    ret_codes: list[int] = pool.map(run_test_case, [(test_case, cli_args) for test_case in cli_args.test_case_names])
    summarize_results(ret_codes, cli_args)
    logging.info("Finished jan_24/run_experiment.py")






if __name__ == '__main__':
    main(sys.argv)