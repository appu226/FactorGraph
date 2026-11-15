# Small-scale AVE benchmark (maxClauseTreeSize=10, timeout=2 h)

Date of run: 2026-10-05 (UTC; launched the evening of 2026-10-04)

This repeats the 15-minute experiment in `experiments/ave/small_scale_20260912` on Cloud Run,
with the timeout increased to 2 hours.

## Configuration

- Same 25-case subset as `small_scale_20260912` (the 25 smallest `.qdimacs` files in the corpus)
- maxClauseTreeSize = 10
- timeoutSeconds = 7200
- Executables: `build/out/oct_22/ave_main` (AVE) and `build/out/oct_22/ave2_main` (AVE2),
  built inside the image from `experiments/ave/small_scale_20260912/Dockerfile`
- Driver: `jan_24/run_ave_small_resume.py`, run as a Cloud Run job with 50 tasks.
  `CLOUD_RUN_TASK_INDEX = 2*case + alg` selects one (case, algorithm) pair per task
  (even index = AVE, odd index = AVE2).
- All 50 tasks started at 2026-10-05T00:58:41Z and ran in parallel.

## Raw data

Fetched with:

```bash
cd external/small_scale_20261004
gcloud storage cp --recursive \
  "gs://ave-small-test/inputs" "gs://ave-small-test/logs" "gs://ave-small-test/outputs" .
```

Local layout:

- `external/small_scale_20261004/inputs/`: the 25 `.qdimacs` inputs
- `external/small_scale_20261004/logs/{ave,ave2}_<case>.json`: per-run status, including the stdout tail
- `external/small_scale_20261004/logs/night_run_task<N>.log`: per-task driver logs
- `external/small_scale_20261004/outputs/{ave,ave2}_results_max10/<case>.cnf`: result CNFs

Leftovers from smoke tests run at 00:45–00:47, before the real job, which can be ignored:
`logs/night_run.log`, `logs/ave_QBFEval2017_small-bug1-fixpoint-3.log` and
`outputs/ave_results_max10/ave_QBFEval2017_small-bug1-fixpoint-3.cnf`.
The older `external/{inputs,logs,outputs}` folders, which contain `simple.qdimacs`, are also from a smoke test.

### How runtimes were derived

The JSON files don't record runtime: their `timestamp` field is the **start** time.
Runtime was computed as (GCS upload time of the JSON) − (start timestamp).

The line `AVE: Time limit reached, terminating further processing.` is printed by a detached timer
thread after it sleeps the full `timeoutSeconds`. Its presence in `stdout_tail` therefore means the
run used the whole 2 h budget. The runtimes confirm this: every run that printed the line took 7199–7200 s.

## Results

All 50 runs exited with `return_code = 0` and wrote an output file. **Every output contains 0 clauses.**
The files are either `c ind 0 / p cnf 0 0` or `c ind <v> 0 / p cnf 0 0`, where the second form appears on the
fixpoint cases, whose single kept variable `<v>` is listed. An empty clause set means "true". That is a
sound but useless over-approximation.

| Algorithm | Finished before timeout | Hit 2 h timeout | Non-empty result |
|-----------|------------------------:|----------------:|-----------------:|
| AVE       | 5                       | 20              | 0                |
| AVE2      | 0                       | 25              | 0                |

On timeout, both binaries still write whatever result clauses they have collected. So a timed-out run
looks the same as a completed run, unless you check the stdout or the runtime.

### AVE runs that finished

| Case                               | Vars | Clauses | Runtime | Result    |
|------------------------------------|-----:|--------:|--------:|-----------|
| QBFEval2017_small-bug1-fixpoint-3  |   53 |     120 |    26 s | 0 clauses |
| QBFEval2017_small-bug1-fixpoint-4  |   70 |     162 |   219 s | 0 clauses |
| Arithmetic_decomposition32         |  285 |     772 |   869 s | 0 clauses |
| QBFEval2017_small-bug1-fixpoint-5  |   87 |     204 |  1809 s | 0 clauses |
| Arithmetic_subtraction32           |  283 |     762 |  5020 s | 0 clauses |

The `small-bug1-fixpoint-N` family shows the scaling clearly. Each step adds 17 variables and multiplies
runtime by about 8: 26 s → 219 s → 1809 s → over 7200 s (fixpoint-6 timed out).
This is consistent with an exponential search over clause trees.

Even when AVE finished the search, it found no resolvent free of eliminated variables
within a clause tree of size 10.

### Per-case table

| Case                                                             | Vars | Clauses | AVE              | AVE2    |
|------------------------------------------------------------------|-----:|--------:|------------------|---------|
| Arithmetic_decomposition32                                       |  285 |     772 | done, 869 s      | timeout |
| Arithmetic_floor32                                               |  279 |     760 | timeout          | timeout |
| Arithmetic_intermediate32                                        |  319 |     916 | timeout          | timeout |
| Arithmetic_max32                                                 |  212 |     613 | timeout          | timeout |
| Arithmetic_min32                                                 |  213 |     617 | timeout          | timeout |
| Arithmetic_subtraction32                                         |  283 |     762 | done, 5020 s     | timeout |
| Disjunctive_Decomposition_kenoopp1_all_bit_differing_from_cycle  |  284 |     681 | timeout          | timeout |
| Factorization_factorization8                                     |   95 |     298 | timeout          | timeout |
| Factorization_factorization10                                    |  151 |     485 | timeout          | timeout |
| Factorization_factorization12                                    |  219 |     714 | timeout          | timeout |
| Factorization_factorization14                                    |  295 |     978 | timeout          | timeout |
| QBFEval2017_falsequ_query42_1344n                                |  319 |    1089 | timeout          | timeout |
| QBFEval2017_nxquery_query50_1344n                                |  295 |     825 | timeout          | timeout |
| QBFEval2017_small-bug1-fixpoint-3                                |   53 |     120 | done, 26 s       | timeout |
| QBFEval2017_small-bug1-fixpoint-4                                |   70 |     162 | done, 219 s      | timeout |
| QBFEval2017_small-bug1-fixpoint-5                                |   87 |     204 | done, 1809 s     | timeout |
| QBFEval2017_small-bug1-fixpoint-6                                |  104 |     246 | timeout          | timeout |
| QBFEval2017_small-bug1-fixpoint-8                                |  138 |     330 | timeout          | timeout |
| QBFEval2017_small-bug1-fixpoint-9                                |  155 |     372 | timeout          | timeout |
| QBFEval2017_small-bug1-fixpoint-10                               |  172 |     414 | timeout          | timeout |
| QBFEval2017_small-dyn-partition-fixpoint-1                       |   88 |     221 | timeout          | timeout |
| QBFEval2017_small-dyn-partition-fixpoint-2                       |  205 |     559 | timeout          | timeout |
| QBFEval2017_small-dyn-partition-fixpoint-3                       |  320 |     893 | timeout          | timeout |
| QBFEval2017_trueque_query42_1344n                                |  335 |     531 | timeout          | timeout |
| QBFEval2018_nxquery_query50_1344n                                |  295 |     825 | timeout          | timeout |

Every output, from every run, has 0 clauses.

## Observations

1. **AVE2 ignores `maxClauseTreeSize`.** In `oct_22/ave2.cpp`, `Ave2::growSeed` receives
   `searchDepth` and passes `searchDepth - 1` to its recursive calls, but it never checks for 0.
   `searchDepth` is a `size_t`, so the value wraps around instead of stopping the recursion,
   and the search is bounded only by the timeout. This explains why AVE2 timed out even on the
   53-variable `small-bug1-fixpoint-3`, which AVE finishes in 26 s. As things stand, AVE2 results
   cannot be compared to AVE results at the same `maxClauseTreeSize`.

2. **AVE is exponential in practice and still produced nothing useful.** Where AVE finished, it
   found 0 eliminating resolvents. Where it timed out, it had also collected 0 by the time it stopped.
   A bigger budget alone is unlikely to help: the cases that finished needed between 26 s and 84 min,
   and still returned "true".

3. **Timeouts are indistinguishable from completions in the output files.** The earlier
   30 s / maxClauseTreeSize=5 run in `small_scale_20260912/README.md` reported 24/25 cases as "completed".
   Given these runtimes, many of those were probably silent timeouts too. Its one non-trivial
   result, `Arithmetic_min32` with a single clause `-181 118`, did not appear here, where that case timed out.

## Suggested next steps

- Fix the depth check in `Ave2::growSeed`, by returning when `searchDepth == 0` as
  `ApproxVarElim::elimHelper` does, and rerun AVE2.
- Make the binaries report timeouts explicitly, for example with a distinct exit code or a
  `timed_out` field. Also have the driver record wall-clock runtime in the JSON.
- Try smaller `maxClauseTreeSize` values (for example 2–6) on the fixpoint family, to find where
  AVE first produces non-empty results and how runtime scales with the cap.
