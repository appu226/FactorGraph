#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run the small AVE/AVE2 benchmark set overnight with resumable progress."
    )
    parser.add_argument(
        "--input_dir",
        default="temp/small_test_set",
        help="Directory containing the QDIMACS inputs.",
    )
    parser.add_argument(
        "--ave_binary",
        default="build/out/oct_22/ave_main",
        help="Path to the AVE binary.",
    )
    parser.add_argument(
        "--ave2_binary",
        default="build/out/oct_22/ave2_main",
        help="Path to the AVE2 binary.",
    )
    parser.add_argument(
        "--ave_output_dir",
        default="temp/small_test_set/ave_results_max10",
        help="Directory for AVE outputs.",
    )
    parser.add_argument(
        "--ave2_output_dir",
        default="temp/small_test_set/ave2_results_max10",
        help="Directory for AVE2 outputs.",
    )
    parser.add_argument(
        "--max_clause_tree_size",
        type=int,
        default=10,
        help="Clause-tree size cap to pass to each solver.",
    )
    parser.add_argument(
        "--timeout_seconds",
        type=int,
        default=900,
        help="Timeout in seconds for each benchmark case.",
    )
    parser.add_argument(
        "--log_verbosity",
        default="ERROR",
        choices=["QUIET", "ERROR", "WARNING", "INFO", "DEBUG"],
        help="Verbosity passed to the solver.",
    )
    parser.add_argument(
        "--single_algorithm",
        choices=["ave", "ave2", "both"],
        default="both",
        help="Which algorithm to run. Defaults to both.",
    )
    parser.add_argument(
        "--case",
        help="Run only this QDIMACS filename or stem.",
    )
    parser.add_argument(
        "--rerun_existing",
        action="store_true",
        help="Re-run cases even if an output file already exists.",
    )
    parser.add_argument(
        "--status_dir",
        default="temp/small_test_set/run_status",
        help="Directory for per-case status tracking.",
    )
    args = parser.parse_args()
    return args


def ensure_dirs(paths):
    for p in paths:
        Path(p).mkdir(parents=True, exist_ok=True)


def run_case(binary, input_path, output_path, max_clause_tree_size, timeout_seconds, log_verbosity):
    cmd = [
        binary,
        "--inputFile",
        str(input_path),
        "--outputFile",
        str(output_path),
        "--maxClauseTreeSize",
        str(max_clause_tree_size),
        "--timeoutSeconds",
        str(timeout_seconds),
        "--logVerbosity",
        log_verbosity,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def file_stem(path: Path) -> str:
    return path.stem


def select_cloud_run_task(qdimacs_files, args):
    task_index_value = os.environ.get("CLOUD_RUN_TASK_INDEX")
    if task_index_value is None:
        return qdimacs_files

    try:
        task_index = int(task_index_value)
    except ValueError:
        raise SystemExit(f"Invalid CLOUD_RUN_TASK_INDEX: {task_index_value}")

    algorithms = ["ave", "ave2"] if args.single_algorithm == "both" else [args.single_algorithm]
    expected_task_count = len(qdimacs_files) * len(algorithms)
    task_count_value = os.environ.get("CLOUD_RUN_TASK_COUNT")
    if task_count_value is not None:
        try:
            task_count = int(task_count_value)
        except ValueError:
            raise SystemExit(f"Invalid CLOUD_RUN_TASK_COUNT: {task_count_value}")
        if task_count != expected_task_count:
            raise SystemExit(
                f"CLOUD_RUN_TASK_COUNT is {task_count}; expected {expected_task_count} "
                "for the selected cases and algorithms"
            )

    if not 0 <= task_index < expected_task_count:
        raise SystemExit(
            f"CLOUD_RUN_TASK_INDEX must be between 0 and {expected_task_count - 1}; "
            f"got {task_index}"
        )

    case_index, algorithm_index = divmod(task_index, len(algorithms))
    args.single_algorithm = algorithms[algorithm_index]
    selected_case = qdimacs_files[case_index]
    print(
        f"Cloud Run task {task_index}/{expected_task_count}: "
        f"{selected_case.stem} with {args.single_algorithm}"
    )
    return [selected_case]


def main():
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    os.chdir(repo_root)

    input_dir = Path(args.input_dir)
    if not input_dir.exists():
        raise SystemExit(f"Input directory does not exist: {input_dir}")

    ave_binary = Path(args.ave_binary)
    ave2_binary = Path(args.ave2_binary)
    if not ave_binary.exists():
        raise SystemExit(f"AVE binary not found: {ave_binary}")
    if not ave2_binary.exists():
        raise SystemExit(f"AVE2 binary not found: {ave2_binary}")

    ensure_dirs([
        args.ave_output_dir,
        args.ave2_output_dir,
        args.status_dir,
    ])

    qdimacs_files = sorted(input_dir.glob("*.qdimacs"))
    if args.case:
        case_name = args.case.removesuffix(".qdimacs")
        qdimacs_files = [path for path in qdimacs_files if path.stem == case_name]
        if not qdimacs_files:
            raise SystemExit(f"No QDIMACS case found: {args.case}")
    if not qdimacs_files:
        print(f"No QDIMACS files found in {input_dir}")
        return 0
    qdimacs_files = select_cloud_run_task(qdimacs_files, args)

    # Concurrent appends to one shared file fail on a Cloud Storage FUSE mount,
    # so each Cloud Run task gets its own log.
    task_index_value = os.environ.get("CLOUD_RUN_TASK_INDEX")
    log_name = "night_run.log" if task_index_value is None else f"night_run_task{task_index_value}.log"
    log_path = Path(args.status_dir) / log_name
    with open(log_path, "a", encoding="utf-8") as log:
        log.write(f"\n[{datetime.now().isoformat()}] Starting run\n")
        log.write(f"input_dir={input_dir}\n")
        log.write(f"max_clause_tree_size={args.max_clause_tree_size}\n")
        log.write(f"timeout_seconds={args.timeout_seconds}\n")
        log.write(f"single_algorithm={args.single_algorithm}\n")
        log.write(f"total_cases={len(qdimacs_files)}\n")

    def run_algorithm(label, binary, output_dir):
        completed = 0
        failed = 0
        skipped = 0
        for qfile in qdimacs_files:
            stem = file_stem(qfile)
            output_path = Path(output_dir) / f"{stem}.cnf"
            status_path = Path(args.status_dir) / f"{label}_{stem}.json"

            if output_path.exists() and output_path.stat().st_size > 0 and not args.rerun_existing:
                skipped += 1
                print(f"[{label}] SKIP {stem} (output exists)")
                continue

            timestamp = datetime.now().isoformat()
            print(f"[{label}] RUN {stem} @ {timestamp}")
            rc, stdout, stderr = run_case(
                str(binary),
                qfile,
                output_path,
                args.max_clause_tree_size,
                args.timeout_seconds,
                args.log_verbosity,
            )

            status = {
                "timestamp": timestamp,
                "case": stem,
                "binary": str(binary),
                "input": str(qfile),
                "output": str(output_path),
                "return_code": rc,
                "max_clause_tree_size": args.max_clause_tree_size,
                "timeout_seconds": args.timeout_seconds,
                "output_size": output_path.stat().st_size if output_path.exists() else 0,
                "stdout_tail": (stdout or "")[-2000:],
                "stderr_tail": (stderr or "")[-2000:],
            }
            with open(status_path, "w", encoding="utf-8") as f:
                json.dump(status, f, indent=2)

            with open(log_path, "a", encoding="utf-8") as log:
                log.write(f"[{label}] {stem} return_code={rc} output_size={status['output_size']}\n")

            if rc == 0:
                completed += 1
            else:
                failed += 1

        return completed, failed, skipped

    if args.single_algorithm in ("both", "ave"):
        c, f, s = run_algorithm("ave", ave_binary, args.ave_output_dir)
        print(f"AVE summary: completed={c}, failed={f}, skipped={s}")
    if args.single_algorithm in ("both", "ave2"):
        c, f, s = run_algorithm("ave2", ave2_binary, args.ave2_output_dir)
        print(f"AVE2 summary: completed={c}, failed={f}, skipped={s}")

    # Final summary
    for label, output_dir in [("ave", args.ave_output_dir), ("ave2", args.ave2_output_dir)]:
        if args.single_algorithm not in ("both", label):
            continue
        outputs = sorted(Path(output_dir).glob("*.cnf"))
        missing = [p.stem for p in qdimacs_files if not (Path(output_dir) / f"{p.stem}.cnf").exists()]
        print(f"[{label}] final outputs={len(outputs)} missing={len(missing)}")
        if missing:
            print(f"[{label}] missing_cases={missing}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
