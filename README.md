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

### Oct 22
Given an existential quantification problem $\exists_X \phi(X, Y)$, and an overapproximate solution $\phi'(Y)$, we can construct a third formula  $\beta(Y) = \phi'(Y) \wedge \neg \exists_X \phi(X, Y) = \phi(Y) \wedge \forall_X \neg \phi(X, Y)$ to investigate whether $\phi'$ is the exact solution or not. 
The `oct_22` algorithm uses MUST to discover unsatisfyable cores for $\forall_X \neg \phi(X, Y)$, iteratively tightening the over-approximate result $\phi'$ until it becomes exact.

**Algorithm overview:**\
[oct_22.pdf](./oct_22/oct_22.pdf).

**Source code:**\
[oct_22](./oct_22])

**Usage:**
```
build/out/oct_22/oct_22 --help
--help/-h:      Print this help and exit.
--largestSupportSet:    largest allowed support set size while clumping cnf factors
--largestBddSize:       largest allowed bdd size while clumping cnf factors
--inputFile:    Input qdimacs file with exactly one quantifier which is existential     [Mandatory]
--verbosity:    Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)
--computeExactUsingBdd: Compute exact solution (default false)
--outputFile:   Cnf file with result
--runMusTool:   Whether to run MUS tool (default true)
--runFg:        Whether to run factor graph (default true)
--minimalizeAssignments:        Whether to minimalize assignments found by must
```


### Dec 21
This is an older version of the Oct 22 algorithm, without some of the improvements such as assiginment minimalization.

**Algorithm overview:**\
[dec_21.pdf](./dec_21/dec_21.pdf)

**Source code:**\
[dec_21](./dec_21)

**Usage:**
```
build/out/dec_21/dec_21 --help
--help/-h:      Print this help and exit.
--largestSupportSet:    largest allowed support set size while clumping cnf factors
--maxMucSize:   max clauses allowed in an MUC
--inputFile:    Input qdimacs file with exactly one quantifier which is existential     [Mandatory]
--verbosity:    Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)
--computeExact: Compute exact solution (default false)
```


### May 22
An incomplete attempt at MUC discovery using statistical techniques.

**Algorithm overview:**\
[may_22.pdf](./may_22/may_22.pdf)

**Source code (incomplete):**\
[may_22](./may_22)

### Factor Graph
This is an algorithm for existentially quantifying variables from a CNF, or any conjunction of boolean formulae, taking inspiration from [Belief Propagation](https://en.wikipedia.org/wiki/Belief_propagation) (also known as Sum Product and Factor Graph algorithms). The theory is beautiful and worth a read, although the practical results were quite under whelming.

**Algorithm overview:**\
[factor_graph.pdf](./factor_graph/factor_graph.pdf)

**Source code:**:\
[factor_graph](./factor_graph)

**Usage:**\
`factor_graph_main` is an interactive command line executable that takes a path to a cnf file as the one and only command line argument.
```
build/out/factor_graph_main/factor_graph_main <path to cnf file>
max memory allowed : 4000 MB
numclauses is 4, numvars = 5
factor graph created
-> help
quit/exit                : end the program
help                     : print this message
verify                   : verify the factor graph
print                    : print the factor graph to files
createpng                : create a png of the factorgraph
checkpoint               : set checkpoint 2
rollback                 : rollback to checkpoint 0
makeacyclic              : attempt to make the graph acyclic
converge                 : pass messages to convergence
setvar                   : set a variable to be true or false
assertmsg                : check whether message passing works
mergevar                 : merge two variable nodes
mergefunc                : merge two function nodes
mergeheur3               : merge 2-neigh func nodes with other func nodes of super set of neighbors
fgtest3                  : make the graph acyclic by recursively assigning values to variables
fgtest4                  : compare the exact answer timewise with factorgraph result
randomgroup              : make a random grouping of variables
```

## Dependencies

### CUDD
The CUDD package is a package written in C for the manipulation of
decision diagrams.  It supports binary decision diagrams (BDDs),
algebraic decision diagrams (ADDs), and Zero-Suppressed BDDs (ZDDs).

**Source code:**
* Official repository at [cuddorg](https://github.com/cuddorg/cudd)
* Private fork of cuddorg repo by [appu226](https://github.com/appu226/cudd/tree/feature/rename_macro_fail), with minor compilation fixes
* Official repository at [Open ROAD](https://github.com/The-OpenROAD-Project/cudd)

**Wrapper:**\
This project uses CUDD for implementing the Factor Graph algorithm, using the following utlities on top of CUDD:
* [cuddAndAbsMulti](./dd/cuddAndAbsMulti.h): A low level implementation of the "And Abstract" operation ($\exists_X \wedge_{i=1}^N F_i(X, Y)$) for more than two functions ($N>2$)
* [dd.h](./dd/dd.h): Utility layer with automatic reference count increment. (Not recommended for low level use.)
* [bdd_factory.h](./dd/bdd_factory.h): A C++ wrapper over CUDD BDDs with automatic resource management. (Not recommended for low level use.)


### Kissat Elimination
Kissat is a "keep it simple and clean bare metal SAT solver" written in C.
The original project is hosted at https://github.com/arminbiere/kissat.
This Factor Graph project depends on a [fork](https://github.com/appu226/kissat/tree/cav_2024), that exposes a low level function [`kissat_eliminate_variables`](https://github.com/appu226/kissat/blob/66cfd9012bdba800f26e2b44b3d2d0e02a9b80f0/src/kissat.h#L42) for existential quantification of a specific subset of variables.

```c
int kissat_eliminate_variables (kissat *solver, int *idx_array, unsigned idx_array_size);
```


### BFSS Elimination
BFSS, or Blazingly Fast Skolem function Synthesis, is a tool based on work reported in the following two papers:
* S. Akshay, Supratik Chakraborty, Shubham Goel, Sumith Kulal, Shetal Shah, "Boolean Functional Synthesis: Hardness and Practical Algorithms", to appear in Formal Methods in System Design, 2020
* S. Akshay, Supratik Chakraborty, Shubham Goel, Sumith Kulal, and Shetal Shah, "What's hard about Boolean Functional Synthesis?" in Proceedings of 30th International Conference on Computer Aided Verification, 2018

This Factor Graph project uses the [`readCnf`](https://github.com/BooleanFunctionalSynthesis/bfss/blob/master/src/readCnf.cpp) executable from BFSS to try and eliminate existentially quantified variables from a qdimacs.



### MUST
Given an unsatisfyable conjunction X of boolean functions, Minimal Unsatisfiable Subsets or MUSes are subsets C of X such that:
* every subset of C is satisfyable
* C is unsatisfyable

[MUST](https://github.com/jar-ben/mustool) is an excellent tool for discovering MUSes of a boolean formula.

A copy of MUS is incorporated into the code base, under the [mustool](./mustool) folder, with additional instrumentations for incremental result discovery. An algorithm for using MUS 


## Setup


## Experimental results


## References

**CUDD:**\
[CUDD: CU decision diagram package release 2.4.2](https://scholar.google.com/scholar?oi=bibs&cluster=11128894880390368791&btnI=1&hl=en)\
F Somenzi - University of Colorado at Boulder, 2009

**MUST:**
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

**Kissat:**
<p>
<a href="https://cca.informatik.uni-freiburg.de/biere/index.html#publications">Armin Biere</a>,
<a href="/biere/index.html">Tobias Faller</a>,
Katalin Fazekas,
<a href="https://cca.informatik.uni-freiburg.de/fleury/index.html">Mathias Fleury</a>,
Nils Froleyks
and
<a href="https://cca.informatik.uni-freiburg.de/pollittf.html">Florian Pollitt</a>
<br>
<a href="https://cca.informatik.uni-freiburg.de/papers/BiereFallerFazekasFleuryFroleyksPollitt-SAT-Competition-2024-solvers.pdf">CaDiCaL, Gimsatul, IsaSAT and Kissat Entering the SAT Competition 2024</a>
<br>
<i>Proc.&nbsp;SAT Competition 2024: Solver, Benchmark and Proof Checker Descriptions</i>
<br>
Marijn Heule, Markus Iser, Matti J&auml;rvisalo, Martin Suda (editors)
<br>
Department of Computer Science Report Series B
<br>
vol.&nbsp;B-2024-1,
pages 8-10,
University of Helsinki 2024
<br>
[ <a href="https://cca.informatik.uni-freiburg.de/papers/BiereFallerFazekasFleuryFroleyksPollitt-SAT-Competition-2024-solvers.pdf">paper</a>
| <a href="https://cca.informatik.uni-freiburg.de/papers/BiereFallerFazekasFleuryFroleyksPollitt-SAT-Competition-2024-solvers.bib">bibtex</a>
| <a href="https://github.com/arminbiere/cadical">cadical</a>
| <a href="https://github.com/arminbiere/kissat">kissat</a>
| <a href="https://github.com/arminbiere/gimsatul">gimsatul</a>
| <a href="https://cca.informatik.uni-freiburg.de/sat24medals">medals</a>
]
</p>


**BFSS:**
```
@InProceedings{bfss-cav2018,
    author="Akshay, S.
    and Chakraborty, Supratik
    and Goel, Shubham
    and Kulal, Sumith
    and Shah, Shetal",
    editor="Chockler, Hana
    and Weissenbacher, Georg",
    title="What's Hard About Boolean Functional Synthesis?",
    booktitle="Computer Aided Verification",
    year="2018",
    publisher="Springer International Publishing",
    address="Cham",
    pages="251--269",
    abstract="Given a relational specification between Boolean inputs and outputs, the goal of Boolean functional synthesis is to synthesize each output as a function of the inputs such that the specification is met. In this paper, we first show that unless some hard conjectures in complexity theory are falsified, Boolean functional synthesis must generate large Skolem functions in the worst-case. Given this inherent hardness, what does one do to solve the problem? We present a two-phase algorithm, where the first phase is efficient both in terms of time and size of synthesized functions, and solves a large fraction of benchmarks. To explain this surprisingly good performance, we provide a sufficient condition under which the first phase must produce correct answers. When this condition fails, the second phase builds upon the result of the first phase, possibly requiring exponential time and generating exponential-sized functions in the worst-case. Detailed experimental evaluation shows our algorithm to perform better than other techniques for a large number of benchmarks.",
    isbn="978-3-319-96145-3"
}
```
