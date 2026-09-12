# Small-scale AVE benchmark summary

Date: 2026-09-12

## What was tested

This experiment ran the two approximate variable elimination implementations against a small benchmark subset drawn from the full QDIMACS corpus in the workspace.

- Benchmark source: /home/parakram/Software/DDP/all_test_cases
- Subset folder used: temp/small_test_set
- Selection method: the 25 smallest `.qdimacs` files by file size, selected with:

  find /home/parakram/Software/DDP/all_test_cases -maxdepth 1 -type f -name '*.qdimacs' -printf '%s\t%p\n' | sort -n | head -25

The following executables were run:

- AVE: build/out/oct_22/ave_main
- AVE2: build/out/oct_22/ave2_main

Each case was run with:

- maxClauseTreeSize = 5
- timeoutSeconds = 30
- logVerbosity = ERROR

Outputs were written to:

- temp/small_test_set/ave_results
- temp/small_test_set/ave2_results

## Benchmark subset

The 25 input QDIMACS files were:

- Arithmetic_decomposition32.qdimacs
- Arithmetic_floor32.qdimacs
- Arithmetic_intermediate32.qdimacs
- Arithmetic_max32.qdimacs
- Arithmetic_min32.qdimacs
- Arithmetic_subtraction32.qdimacs
- Disjunctive_Decomposition_kenoopp1_all_bit_differing_from_cycle.qdimacs
- Factorization_factorization10.qdimacs
- Factorization_factorization12.qdimacs
- Factorization_factorization14.qdimacs
- Factorization_factorization8.qdimacs
- QBFEval2017_falsequ_query42_1344n.qdimacs
- QBFEval2017_nxquery_query50_1344n.qdimacs
- QBFEval2017_small-bug1-fixpoint-10.qdimacs
- QBFEval2017_small-bug1-fixpoint-3.qdimacs
- QBFEval2017_small-bug1-fixpoint-4.qdimacs
- QBFEval2017_small-bug1-fixpoint-5.qdimacs
- QBFEval2017_small-bug1-fixpoint-6.qdimacs
- QBFEval2017_small-bug1-fixpoint-8.qdimacs
- QBFEval2017_small-bug1-fixpoint-9.qdimacs
- QBFEval2017_small-dyn-partition-fixpoint-1.qdimacs
- QBFEval2017_small-dyn-partition-fixpoint-2.qdimacs
- QBFEval2017_small-dyn-partition-fixpoint-3.qdimacs
- QBFEval2017_trueque_query42_1344n.qdimacs
- QBFEval2018_nxquery_query50_1344n.qdimacs

## What the results showed

Fresh verification after the run showed:

- Input count: 25
- AVE output count: 24
- AVE2 output count: 24
- Missing in both runs: QBFEval2018_nxquery_query50_1344n

This means the algorithms completed on nearly every instance under the chosen tight settings, but the actual residual outputs were dominated by extremely small or empty formulas rather than rich intermediate CNFs.

### Per-case classification

The completed outputs were categorized by inspecting the actual CNF content in the generated result files.

#### 1. Trivial empty residuals

These cases ended with a near-empty output such as:

- `c ind 0`
- `p cnf 0 0`

This was the dominant pattern among the completed runs. Examples include:

- Arithmetic_decomposition32
- Arithmetic_floor32
- Arithmetic_intermediate32
- Arithmetic_max32
- Arithmetic_subtraction32
- Disjunctive_Decomposition_kenoopp1_all_bit_differing_from_cycle
- Factorization_factorization10
- Factorization_factorization12
- Factorization_factorization14
- Factorization_factorization8
- QBFEval2017_falsequ_query42_1344n
- QBFEval2017_nxquery_query50_1344n
- QBFEval2017_trueque_query42_1344n

#### 2. Tiny residuals with a small index marker

Some fixpoint-style cases produced very small outputs containing an index prefix rather than a substantive CNF, for example:

- `c ind 53 0`
- `c ind 87 0`
- `c ind 138 0`
- `c ind 205 0`
- `c ind 320 0`

These were still effectively trivial residuals, but they suggest the solver did a strong reduction and then stopped with a nearly empty object. Examples include:

- QBFEval2017_small-bug1-fixpoint-10
- QBFEval2017_small-bug1-fixpoint-3
- QBFEval2017_small-bug1-fixpoint-4
- QBFEval2017_small-bug1-fixpoint-5
- QBFEval2017_small-bug1-fixpoint-6
- QBFEval2017_small-bug1-fixpoint-8
- QBFEval2017_small-bug1-fixpoint-9
- QBFEval2017_small-dyn-partition-fixpoint-1
- QBFEval2017_small-dyn-partition-fixpoint-2
- QBFEval2017_small-dyn-partition-fixpoint-3

#### 3. Non-trivial residual formula

Only one case in the small set produced a clearly non-empty CNF that retained a real clause structure:

- Arithmetic_min32

File inspected: temp/small_test_set/ave_results/Arithmetic_min32.cnf

Observed content:

- `p cnf 181 1`
- `-181 118 0`

This is the clearest evidence that the algorithm can leave a nontrivial residual formula behind under the current settings. It is also an outlier: the majority of cases collapsed to empty or nearly empty outputs.

### AVE summary

AVE produced outputs for 24 of the 25 cases.

The output pattern was:

- trivial empty residuals: most arithmetic and factorization cases
- tiny residual index outputs: most fixpoint cases
- non-trivial residual: Arithmetic_min32 only

Missing case for AVE:

- QBFEval2018_nxquery_query50_1344n

### AVE2 summary

AVE2 produced outputs for 24 of the 25 cases.

The output pattern was very similar to AVE:

- trivial empty residuals: most arithmetic and factorization cases
- tiny residual index outputs: most fixpoint cases
- no clear additional non-trivial residuals beyond the AVE-like pattern

Missing case for AVE2:

- QBFEval2018_nxquery_query50_1344n

### ASCII classification table

+----------------------------------+---------------------------+---------------------------------------------+
| Case family                      | Count                     | Result pattern                               |
+----------------------------------+---------------------------+---------------------------------------------+
| Arithmetic / decomposition       | 6                         | Mostly trivial empty residuals               |
| Factorization                    | 4                         | Mostly trivial empty residuals               |
| QBFEval fixpoint / dyn partition | 10                        | Tiny residual index outputs, mostly trivial  |
| Single non-trivial residual      | 1                         | Arithmetic_min32 produced a real CNF clause  |
| Missing / failed                 | 1                         | QBFEval2018_nxquery_query50_1344n            |
+----------------------------------+---------------------------+---------------------------------------------+

Exact per-case signal:

+------------------------------------------------------+-------------------------------+--------------------------+
| Benchmark                                            | AVE result                    | AVE2 result              |
+------------------------------------------------------+-------------------------------+--------------------------+
| Arithmetic_decomposition32                            | empty residual                | empty residual           |
| Arithmetic_floor32                                    | empty residual                | empty residual           |
| Arithmetic_intermediate32                             | empty residual                | empty residual           |
| Arithmetic_max32                                      | empty residual                | empty residual           |
| Arithmetic_min32                                      | non-trivial residual          | empty residual           |
| Arithmetic_subtraction32                              | empty residual                | empty residual           |
| Disjunctive_Decomposition_kenoopp1_all_bit_differing_ | empty residual                | empty residual           |
| Factorization_factorization10                         | empty residual                | empty residual           |
| Factorization_factorization12                         | empty residual                | empty residual           |
| Factorization_factorization14                         | empty residual                | empty residual           |
| Factorization_factorization8                          | empty residual                | empty residual           |
| QBFEval2017_falsequ_query42_1344n                    | empty residual                | empty residual           |
| QBFEval2017_nxquery_query50_1344n                    | empty residual                | empty residual           |
| QBFEval2017_small-bug1-fixpoint-10                   | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-3                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-4                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-5                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-6                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-8                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-bug1-fixpoint-9                    | tiny residual index           | tiny residual index      |
| QBFEval2017_small-dyn-partition-fixpoint-1           | tiny residual index           | tiny residual index      |
| QBFEval2017_small-dyn-partition-fixpoint-2           | tiny residual index           | tiny residual index      |
| QBFEval2017_small-dyn-partition-fixpoint-3           | tiny residual index           | tiny residual index      |
| QBFEval2017_trueque_query42_1344n                    | empty residual                | empty residual           |
| QBFEval2018_nxquery_query50_1344n                   | missing                       | missing                 |
+------------------------------------------------------+-------------------------------+--------------------------+

Legend:

- empty residual: output looks like `c ind 0` / `p cnf 0 0`
- tiny residual index: output contains a tiny index marker such as `c ind 53 0`
- non-trivial residual: actual clause-bearing CNF remains, e.g. `p cnf 181 1`
- missing: no output file created under the run budget

## Interpretation

This run was materially more informative than a raw pass/fail count suggests:

- There was broad progress in terms of successful completion across the subset.
- However, the residual outputs were overwhelmingly degenerate under the very tight settings used here (maxClauseTreeSize = 5, timeoutSeconds = 30).
- In practical terms, the solver usually eliminated enough variables to collapse the formula to a trivial final form rather than to a small but still meaningful CNF.
- The only clearly non-trivial residual was the Arithmetic_min32 case, which suggests the algorithm is capable of preserving real formula structure on at least some inputs, but that it is not consistently doing so under the chosen budget.

This is not a failure of the benchmark harness; it is evidence that these settings are conservative and that the algorithms prioritize strong simplification over preserving rich residual structure. The findings suggest the next useful step is to rerun with looser caps to see whether more interesting residuals survive when the solver is allowed to keep building a larger clause tree.

## Conclusion

The small-scale run shows that both AVE and AVE2 are operational and robust on most easy-to-medium instances in this benchmark slice, but under the current settings they usually reduce formulas all the way to empty or near-empty outputs. The real substantive result is therefore not a large set of rich partial eliminations, but a clear pattern that the algorithms are effective at aggressive simplification and that the configuration is too restrictive to expose many meaningful residual formulas.

This makes the subset useful as a first validation sweep, but not as strong evidence of a broad performance advantage between the algorithms. A larger and more permissive run is the next logical step.
