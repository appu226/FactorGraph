from dataclasses import dataclass
from argparse import ArgumentParser
from multiprocessing import cpu_count
from os import listdir, path, makedirs
import sys
import logging
import subprocess
import time
from multiprocessing.pool import ThreadPool


###### utility enum for parsing log verbosity ######
LogLevelMapping: dict[str, int] = {
    'QUIET': logging.CRITICAL,
    'ERROR': logging.ERROR,
    'WARNING': logging.WARN,
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG
}



def add_padding(table: list[list[str]]) -> None:
    for icol in range(len(table[0])):
        max_size: int = 0
        for irow in range(len(table)):
            max_size = max(max_size, len(table[irow][icol]))
        for irow in range(len(table)):
            cell = table[irow][icol]
            table[irow][icol] = cell + (" " * (max_size - len(cell))) 




@dataclass
class CliArgs:
    manthan_image_name: str
    timeout_seconds: int
    input_folder: str
    output_folder: str
    test_case_names: list[str]
    num_processes: int
    verbosity: str






def parse_cli_args(argv: list[str]) -> CliArgs:
    arg_parser = ArgumentParser("manthan_bulk_run.py",
                                description="Script to run manthan docker image on multiple qdimacs files")
    arg_parser.add_argument("--manthan_image_name", required=True, type=str, 
                            help="Name of docker image with manthan entrypoint")
    arg_parser.add_argument("--timeout_seconds", required=False, default=7200, type=int,
                            help="Number of seconds to run manthan before terminating")
    arg_parser.add_argument("--input_folder", required=True, type=str,
                            help="Path to folder with qdimacs files")
    arg_parser.add_argument("--output_folder", required=True, type=str,
                            help="Path to output folder")
    arg_parser.add_argument("--test_case_names", required=False, default="", type=str,
                            help="Comma separated test case file names (default run all files in input_folder)")
    arg_parser.add_argument("--num_processes", required=False, default=min(cpu_count() - 2, 10), type=int,
                            help="Number of processes to spawn in parallel")
    arg_parser.add_argument("--verbosity", type=str, required=False, default="ERROR", choices=list(LogLevelMapping.keys()), 
                            help="Logging verbosity")
    parsed_args = arg_parser.parse_args(argv[1:])
    return CliArgs(manthan_image_name=parsed_args.manthan_image_name,
                   timeout_seconds=parsed_args.timeout_seconds,
                   input_folder=path.abspath(parsed_args.input_folder),
                   output_folder=path.abspath(parsed_args.output_folder),
                   test_case_names=(parsed_args.test_case_names.split(",") if parsed_args.test_case_names != "" else [x for x in listdir(parsed_args.input_folder) if path.isfile(path.join(parsed_args.input_folder, x)) and x.endswith(".qdimacs")]),
                   num_processes=parsed_args.num_processes,
                   verbosity=parsed_args.verbosity)






@dataclass
class Summary:
    name: str
    status: str
    time_taken: float





def run_test_case(
        test_case: str,
        manthan_image_name: str,
        input_folder: str,
        output_folder: str,
        timeout_seconds: int
) -> Summary:
    start_time = time.time()
    logging.info("Starting test case " + test_case)
    try:
        try:
            cmd = ['docker', 'run', '-v', 
                f"{path.join(input_folder, test_case)}:/workspace/{test_case}",
                "--name", test_case, manthan_image_name,
                f"/workspace/{test_case}"]
            manthan_process = subprocess.Popen(cmd)
            was_timeout = True
            try:
                manthan_process.wait(timeout_seconds)
                was_timeout = False
            except subprocess.TimeoutExpired:
                was_timeout = True
                logging.info(f"manthan timed out in {timeout_seconds} secs for {test_case}")
                manthan_process.kill()
                try:
                    subprocess.run(["docker", "stop", test_case])
                except:
                    logging.error(f"{test_case} timed out, but we could not stop docker process")
            cmd = ['docker', 'cp', f"{test_case}:/workspace", path.join(output_folder, test_case)]
            subprocess.run(cmd)
            if was_timeout:
                return Summary(test_case, "Timed out", time.time() - start_time)
            v_file_path = path.join(output_folder, test_case, test_case[0:-8] + "_skolem.v")
            if not path.exists(v_file_path):
                return Summary(test_case, "Failed", time.time() - start_time)
            return Summary(test_case, "Successful", time.time() - start_time)
        except Exception as e:
            logging.error(f"Error for test case {test_case}: {e}")
            return Summary(test_case, "Error: " + str(e), time.time() - start_time)
        except:
            logging.error(f"Unknown error for test case {test_case}")
            return Summary(test_case, "Unknown error", time.time() - start_time)
    finally:
        logging.info("Finished test case " + test_case)





def write_summaries(summaries: list[Summary], output_folder: str):
    summary_table: list[list[str]] = [["TestCase", "Status", "TimeTaken"]] + [[summary.name, summary.status, str(summary.time_taken)] for summary in summaries]
    add_padding(summary_table)
    with open(path.join(output_folder, "summary.txt"), 'w') as sout:
        sout.writelines([(" | ".join(summary)) + "\n" for summary in summary_table])







def main(argv: list[str]):
    cli_args = parse_cli_args(argv)

    makedirs(cli_args.output_folder, exist_ok=True)
    log_handlers: list[logging.Handler] = [
        logging.StreamHandler(sys.stdout), 
        logging.FileHandler(path.join(cli_args.output_folder, "run.log"))]
    
    logging.basicConfig(
            format='[%(levelname)s] [%(asctime)s] %(message)s',
            level=LogLevelMapping[cli_args.verbosity],
            datefmt='%Y-%m-%d %H:%M:%S',
            handlers=log_handlers)
    logging.info(f"cli_args = {cli_args}")

    pool = ThreadPool(cli_args.num_processes)
    summaries: list[Summary] = pool.starmap(run_test_case,
                                    [(test_case, cli_args.manthan_image_name,
                                      cli_args.input_folder,
                                      cli_args.output_folder,
                                      cli_args.timeout_seconds) for test_case in cli_args.test_case_names])
        
    write_summaries(summaries, cli_args.output_folder)

    logging.info("DONE")






if __name__ == "__main__":
    main(sys.argv)