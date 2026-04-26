# Factor Graph
Experimental techniques for existential quantification of variables from a CNF.

Author: Parakram Majumdar\
Contact: firstname \[dot\] lastname \[at\] gmail \[dot\] com\
License: See [LICENSE](./LICENSE)

## Algorithms

### Jan 24
This is an entry point for this tool, combining various disparate algorithms to try and eliminate (existentially quantify) as many variables as possible from a CNF.

**Algorithm Overview:**
The Jan 24 approach is a two-stage pipeline:
1. **Preprocessing Phase**: Iteratively applies [Kissat](#kissat-elimination) (SAT-based variable elimination) and [BFSS](#bfss-elimination) (Bounded Formal Synthesis) until no further progress is made or a timeout is reached. Each round runs Kissat repeatedly until convergence, then runs BFSS (if enabled) to further reduce the formula.
2. **Factor Graph and MUS Phase**: If quantified variables remain after preprocessing, applies the [factor graph](#factor-graph) algorithm with optional [MUS tool](#must) exploration to perform additional variable elimination.

**Source code:**\
[jan_24](./jan_24)

**Usage:**
```
python3 jan_24/jan_24.py --help
usage: jan_24 [-h] --test_case_path TEST_CASE_PATH --output_root OUTPUT_ROOT [--run_preprocess RUN_PREPROCESS] [--bfss_timeout_seconds     BFSS_TIMEOUT_SECONDS]
              [--kissat_timeout_seconds KISSAT_TIMEOUT_SECONDS] [--preprocess_timeout_seconds PREPROCESS_TIMEOUT_SECONDS] [--verbosity {QUIETERROR,WARNING,INFO,DEBUG}] --factor_graph_bin
              FACTOR_GRAPH_BIN --bfss_bin BFSS_BIN [--largest_bdd_size LARGEST_BDD_SIZE] [--largest_support_set LARGEST_SUPPORT_SET][--factor_graph_timeout_seconds FACTOR_GRAPH_TIMEOUT_SECONDS]
              [--run_mus_tool RUN_MUS_TOOL] [--run_factor_graph RUN_FACTOR_GRAPH] [--minimalize_assignments MINIMALIZE_ASSIGNMENTS] [--run_bfssRUN_BFSS]     
Generate experimental results for Quantified Boolean Elimination     
options:
  -h, --help            show this help message and exit
  --test_case_path TEST_CASE_PATH
                        Path to test case QDimacs file
  --output_root OUTPUT_ROOT
                        Path to results folder
  --run_preprocess RUN_PREPROCESS
                        Whether to run Kissat+BFSS pre-processing or not
  --bfss_timeout_seconds BFSS_TIMEOUT_SECONDS
                        Timeout, in seconds, for a round of bfss pre-processing
  --kissat_timeout_seconds KISSAT_TIMEOUT_SECONDS
                        Timeout, in seconds, for a round of kissat pre-processing
  --preprocess_timeout_seconds PREPROCESS_TIMEOUT_SECONDS
                        Total timeout for all pre-processing rounds
  --verbosity {QUIET,ERROR,WARNING,INFO,DEBUG}
                        Logging verbosity
  --factor_graph_bin FACTOR_GRAPH_BIN
                        Path to factor graph build outputs folder (e.g. build/out)
  --bfss_bin BFSS_BIN   Path to bfss binaries folder
  --largest_bdd_size LARGEST_BDD_SIZE
                        Largest BDD size while merging factors for factor graph algorithm
  --largest_support_set LARGEST_SUPPORT_SET
                        Largest support set while merging factors/variables for factor graph algorithm
  --factor_graph_timeout_seconds FACTOR_GRAPH_TIMEOUT_SECONDS
                        Timeout for factor graph (and must exploration) in seconds
  --run_mus_tool RUN_MUS_TOOL
                        Whether to run MUST or not
  --run_factor_graph RUN_FACTOR_GRAPH
                        Whether to run factor graph algorithm or not
  --minimalize_assignments MINIMALIZE_ASSIGNMENTS
                        Whether to minimalize assignments found by MUST
  --run_bfss RUN_BFSS   Whether to run bfss or not
```

### Dec 21
### May 22
### Oct 22
### Binary Decision Diagrams
### Kissat Elimination
### BFSS Elimination

### MUST
Given an unsatisfyable conjunction X of boolean functions, Minimal Unsatisfiable Subsets or MUSes are subsets C of X such that:
* every subset of C is satisfyable
* C is unsatisfyable

[MUST](https://github.com/jar-ben/mustool) is an excellent tool for discovering MUSes of a boolean formula.

A copy of MUS is incorporated into the code base, under the [mustool](./mustool) folder, with additional instrumentations for incremental result discovery. An algorithm for using MUS 


### Factor Graph
This is an algorithm for existentially quantifying variables from a CNF, or any conjunction of boolean formulae, taking inspiration from [Belief Propagation](https://en.wikipedia.org/wiki/Belief_propagation) (also known as Sum Product and Factor Graph algorithms). The theory is beautiful and worth a read, although the practical results were quite under whelming.

Full write-up: [factor_graph.pdf](./factor_graph/factor_graph.pdf)

## Setup


## Codebase

## Experimental results


## References

MUST:
```
@inproceedings{DBLP:conf/tacas/BendikC20,
  author    = {Jaroslav Bend{\'{\i}}k and
               Ivana Cern{\'{a}}},
  title     = {{MUST:} Minimal Unsatisfiable Subsets Enumeration Tool},
  booktitle = {{TACAS} {(1)}},
  series    = {Lecture Notes in Computer Science},
  volume    = {12078},
  pages     = {135--152},
  publisher = {Springer},
  year      = {2020}
}
```