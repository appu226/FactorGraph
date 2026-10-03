# Small-scale AVE benchmark summary (maxClauseTreeSize=10, timeout=15 min)

Date: 2026-09-12

## Configuration

This run used the same 25-case subset as the earlier small-scale sweep, but with looser bounds:

- maxClauseTreeSize = 10
- timeoutSeconds = 900
- logVerbosity = ERROR

Executables used:

- AVE: build/out/oct_22/ave_main
- AVE2: build/out/oct_22/ave2_main

Input directory:

- temp/small_test_set

Output directories:

- temp/small_test_set/ave_results_max10
- temp/small_test_set/ave2_results_max10

## Current status

This run was interrupted before completion. The last verified status before shutdown showed:

- Input count: 25
- AVE output count: 0
- AVE2 output count: 0
- Missing in both runs: all 25 cases at the point of interruption

The process was active at that moment, with an `ave_main` worker still running on `Arithmetic_decomposition32.qdimacs` under `--maxClauseTreeSize 10 --timeoutSeconds 900`.

This means the run is not complete. We left the benchmark in a mid-flight state, and the next run should resume from the current directories rather than restarting from scratch.

## Resume instructions

To continue later, use the resumable nightly script:

- jan_24/run_ave_small_resume.py

Example command:

```bash
cd /home/parakram/Software/DDP/jan_24/FactorGraph
python3 jan_24/run_ave_small_resume.py \
  --input_dir temp/small_test_set \
  --ave_binary build/out/oct_22/ave_main \
  --ave2_binary build/out/oct_22/ave2_main \
  --ave_output_dir temp/small_test_set/ave_results_max10 \
  --ave2_output_dir temp/small_test_set/ave2_results_max10 \
  --max_clause_tree_size 10 \
  --timeout_seconds 900 \
  --log_verbosity ERROR \
  --both
```

The script respects existing output files and skips them unless `--rerun_existing` is supplied.

It writes per-case status under:

- temp/small_test_set/run_status

and the log file there is:

- temp/small_test_set/run_status/night_run.log

## Restart strategy

If you want to resume from the interrupted state without redoing completed work:

1. Leave the existing output directories in place.
2. Run the resumable script without `--rerun_existing`.
3. It will only execute cases for which output is missing or empty.
4. Once the run completes, re-check the output folders and update the summary.

## Conclusion

The experiment was intentionally interrupted and left in a resumable state. The next overnight run should continue from the current directories, make progress on the remaining missing cases, and then update this README with the final verified numbers once the processes exit.

## ASCII outcome table

+---------------------------+---------------------+----------------------+----------------------+
| Algorithm                 | Cases attempted      | Completed outputs    | Missing / timed out   |
+---------------------------+---------------------+----------------------+----------------------+
| AVE                       | 25                  | 0                    | 25                   |
| AVE2                      | 25                  | 0                    | 25                   |
+---------------------------+---------------------+----------------------+----------------------+

## Interpretation

This is not yet a final result. The current state suggests that the looser budget has not produced output yet, but the run is still active and has not reached its final completion point.

The live observation from the current check is:

- the solver is still working on the first test case,
- the output directories remain empty so far,
- and no final completion or failure conclusion should be drawn until the process exits.

This indicates that the algorithms are highly sensitive to search budget and clause-tree expansion under the current implementation, but the data here should be treated as a live status snapshot rather than a finished benchmark conclusion.

## Conclusion

The current evidence does not justify claiming a completed 15-minute benchmark result yet. We have confirmed that the run is still active, which means the previous 0-output summary was a premature observation rather than a final result.

The next step is to wait for the processes to finish and then re-check the output directories before drawing any final conclusion about performance under the maxClauseTreeSize = 10 / timeoutSeconds = 900 configuration.
