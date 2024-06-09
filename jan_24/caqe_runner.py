import argparse
from dataclasses import dataclass
from jan_24_common import LogLevelMapping
import logging
import os
import sys



log = logging.Logger()
comma = ','


@dataclass
class CliArgs:
    caqe_executable_path: str
    timeout_seconds: int
    input_folder: str
    test_case_names: list[str]
    num_processes: int





def parse_cli_args(argv: list[str]) -> CliArgs:
    arg_parser = argparse.ArgumentParser(description="Take a list of qdimacs test cases and run caqe on them")
    arg_parser.add_argument("--caqe_executable_path", type=str, required=True, help="full path to caqe executable")
    arg_parser.add_argument("--timeout_seconds", type=int, required=False, default=60, help="Timeout (in seconds) for caqe invocations")
    arg_parser.add_argument("--input_folder", type=str, required=True, help="Path to folder with test cases")
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
    
    logging.info(f"Processing {len(test_case_names)} test cases")
    logging.debug(f"Test case names: {comma.join(test_case_names)}")
    
    logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[parsed.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S',
            handlers=[logging.StreamHandler(sys.stdout)])

    return CliArgs(
        caqe_executable_path=parsed.caqe_executable_path,
        timeout_seconds=int(parsed.timeout_seconds),
        input_folder=parsed.input_folder,
        test_case_list_path=test_case_names,
        num_processes=parsed.num_processes
    )








def main(argv: list[str]) -> None:
    cli_args = parse_cli_args(argv)
    raise RuntimeError("Rest of main not yet implemented.")






if __name__ == '__main__':
    main(sys.argv)