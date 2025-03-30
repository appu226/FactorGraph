from dataclasses import dataclass
from io import TextIOWrapper
from jan_24_common import add_padding
import os
import re
import sys






def f2s(f: float) -> str:
    return str(round(f, 2))






#TODO
@dataclass
class TestCaseSummary:
    name: str
    status: str
    numVars: int
    numQVars: int
    numClau: int
    bfssElimVars: int
    bfssElimQVars: int
    bfssElimClau: int
    bfssSecs: float
    bfssTimeout: bool
    kissatElimVars: int
    kissatElimQVars: int
    kissatElimClau: int
    kissatSecs: float
    kissatTimeout: bool
    preprocSecs: int
    preprocRounds: int
    bddCreationTime: float
    bddMergeStats: str
    factorGraphCreationTime: str
    factorGraphConvergenceStats: str
    factorGraphResult: str
    musStarted: bool
    musNumClauses: int
    musExplorationTime: float
    musProcessingTime: float
    musNumDisabledSets: int
    musRounds: int
    fgmusTimedOut: bool
    fgmusTime: str
    fgDelibSkip: bool
    musDelibSkip: bool

    #TODO
    def row(self) -> list[str]:
        return [self.name, self.status,
                f"{self.numVars} / {self.bfssElimVars} / {self.kissatElimVars}",
                f"{self.numQVars} / {self.bfssElimQVars} / {self.kissatElimQVars}",
                f"{self.numClau} / {self.bfssElimClau} / {self.kissatElimClau}",
                f"{round(self.preprocSecs, 2)} / {round(self.bfssSecs, 2)} / {round(self.kissatSecs, 2)}",
                self.preprocStatus(),
                str(round(self.bddCreationTime, 2)) + " sec" if self.bddCreationTime is not None else "",
                self.bddMergeStats,
                f2s(self.factorGraphCreationTime) + " sec" if self.factorGraphCreationTime is not None else "",
                self.factorGraphConvergenceStats + self.factorGraphResult,
                self.musSummary(),
                self.musProgress(),
                self.fgmusTime
                ]
    

    def musSummary(self) -> str:
        if not self.musStarted:
            return ""
        elif not self.fgmusTimedOut:
            return f"MUS Finished after {self.musRounds} rounds"
        else:
            return f"MUS TimedOut after {self.musRounds} rounds"

    def musProgress(self) -> str:
        if self.musRounds > 0:
            return f"{f2s(self.musExplorationTime)} / {f2s(self.musProcessingTime)} / {self.numClau} / {self.musNumDisabledSets}"
        else:
            return ""
    

    def preprocStatus(self) -> str:
        if self.bfssTimeout:
            return f"BFSS Timeout after {self.preprocRounds} rounds"
        elif self.kissatTimeout:
            return f"Kissat Timeout after {self.preprocRounds} rounds"
        else:
            return f"PreProc-ed {self.preprocRounds} rounds"

    
    #TODO
    @staticmethod
    def headers() -> list[str]:
        return ["Name", "Status",
                "T / B / K Vars", 
                "T / B / K QVars", 
                "T / B / K Clauses",
                "T / B / K Time",
                "PreProc Status",
                "Bdd Creation Time",
                "Merge F / V / Time",
                "FG creation time",
                "FG convergence",
                "MUS Status",
                "MUS ExplrTime / ProcTime / NumClauses / NumDisabledSets",
                "FG-MUS time"]






#TODO
pattern_num = r'[\-\.0-9]*'
pattern_diag = f'numVarChange: ({pattern_num}) numQuantifiedVarChange: ({pattern_num}) numClauseChange: ({pattern_num}) timeTakenSeconds: ({pattern_num})'
pattern_info_time = r'\[INFO\] \[[0-9:\- ]*\]'
pattern_original_diags = re.compile(pattern_info_time + r' \[original\] Status: (\w*) NumHeaderVars: (\w*) NumHeaderQuantifiedVars: (\w*) NumHeaderClauses: (\w*) NumActualVars: (\w*) NumActualQuantifiedVars: (\w*) NumActualClauses: (\w*)')
pattern_bfss_diag   = re.compile(f"{pattern_info_time} \\[PREP\\] \\[bfss           \\] {pattern_diag}")
pattern_kissat_diag = re.compile(f"{pattern_info_time} \\[PREP\\] \\[kissat         \\] {pattern_diag}")
pattern_preprocessing = re.compile(f"{pattern_info_time} Finished proprocessing in ({pattern_num}) rounds, in ({pattern_num}) seconds.")
pattern_bfss_timeout = re.compile(f"{pattern_info_time} bfss timed out in")
pattern_kissat_timeout = re.compile(f"{pattern_info_time} kissat timed out in")
pattern_fg_started = re.compile(f"{pattern_info_time} Factor graph input prepared in")
pattern_fg_skipped = re.compile(f"{pattern_info_time} Factor graph skipped as pre-processing solved the problem")
pattern_fg_parsed_qdimacs = "[INFO] Parsed qdimacs file in with"
pattern_fg_bdd_created = re.compile(f"\\[INFO\\] Created bdds in ({pattern_num}) sec")
pattern_fg_bdd_merge = re.compile(f"\\[INFO\\] Merged to ({pattern_num}) factors and ({pattern_num})variables in ({pattern_num}) sec")
pattern_fg_created = re.compile(f"\\[INFO\\] Created factor graph in ({pattern_num}) sec")
pattern_fg_converged = re.compile(f"\\[INFO\\] Factor graph converged after ({pattern_num}) iterations in ({pattern_num}) secs")
pattern_fg_one = "[INFO] All factor graph results are ONE."
pattern_fg_zero = "[INFO] Some factor graph result was ZERO."
pattern_mus_started = "Number of constraints in the input set"
pattern_mus_explored = re.compile(f"\\[INFO\\] MUS exploration finished in ({pattern_num}) sec")
pattern_mus_clause = re.compile(f"\\[INFO\\] Adding clause ({pattern_num} )* to solution")
pattern_mus_processed = re.compile(f"\\[INFO\\] MUC processing finished in ({pattern_num}) sec")
pattern_mus_disabled_sets = re.compile(f"\\[INFO\\] Disabled ({pattern_num}) sets from must solver.")
pattern_fg_timeout = re.compile(f"{pattern_info_time} factor graph timed out in ({pattern_num}) secs")
pattern_fg_finished = re.compile(f"{pattern_info_time} Factor graph and must finished in ({pattern_num}) seconds")
pattern_fg_delib_skip = re.compile("\\[INFO\\] Skipping factor graph")
pattern_mus_delib_skip = re.compile("\\[INFO\\] Skipping mus tool")

def parse_test_case(path: str, file: str) -> TestCaseSummary:
    result = TestCaseSummary(
        name=file[0:-12],
        status="Started",
        numVars = None,
        numQVars = None,
        numClau = None,
        bfssElimVars = 0,
        bfssElimQVars = 0,
        bfssElimClau = 0,
        bfssSecs = 0.0,
        bfssTimeout = False,
        kissatElimVars = 0,
        kissatElimQVars = 0,
        kissatElimClau = 0,
        kissatSecs = 0.0,
        kissatTimeout = False,
        preprocSecs = 0,
        preprocRounds = 0,
        bddCreationTime = None,
        bddMergeStats = "",
        factorGraphCreationTime = None,
        factorGraphConvergenceStats= "",
        factorGraphResult = "",
        musStarted = False,
        musNumClauses = 0,
        musExplorationTime = 0.0,
        musProcessingTime = 0.0,
        musNumDisabledSets = 0,
        musRounds = 0,
        fgmusTimedOut = False,
        fgmusTime = "",
        fgDelibSkip = False,
        musDelibSkip = False
    )
    with open(os.path.join(path, file)) as fin:
        for line in fin.readlines():
            m = pattern_original_diags.match(line)
            if m is not None:
                result.numVars, result.numQVars, result.numClau = m.group(5, 6, 7)
                result.status = "Parsed"
                continue
            m = pattern_bfss_diag.match(line)
            if m is not None:
                v, qv, c, t = m.group(1, 2, 3, 4)
                result.bfssElimVars -= int(v)
                result.bfssElimQVars -= int(qv)
                result.bfssElimClau -= int(c)
                result.bfssSecs += float(t)
                continue
            m = pattern_kissat_diag.match(line)
            if m is not None:
                v, qv, c, t = m.group(1, 2, 3, 4)
                result.kissatElimVars -= int(v)
                result.kissatElimQVars -= int(qv)
                result.kissatElimClau -= int(c)
                result.kissatSecs += float(t)
                continue
            m = pattern_preprocessing.match(line)
            if m is not None:
                r, t = m.group(1, 2)
                result.preprocRounds, result.preprocSecs = int(r), float(t)
                continue
            m = pattern_bfss_timeout.match(line)
            if m is not None:
                result.bfssTimeout = True
                result.status = "Preprocess Timed Out"
                continue
            m = pattern_kissat_timeout.match(line)
            if m is not None:
                result.kissatTimeout = True
                result.status = "Preprocess Timed Out"
                continue
            m = pattern_fg_started.match(line)
            if m is not None:
                result.status = "FactorGraph started"
                continue
            m = pattern_fg_skipped.match(line)
            if m is not None:
                result.status = "Preproc Solved, FG skipped."
                continue
            if line.startswith(pattern_fg_parsed_qdimacs):
                result.status = "FactorGraph parsed"
                continue
            m = pattern_fg_bdd_created.match(line)
            if m is not None:
                result.bddCreationTime = float(m.group(1))
                continue
            m = pattern_fg_bdd_merge.match(line)
            if m is not None:
                result.bddMergeStats = f"{m.group(1)} / {m.group(2)} / {m.group(3)}"
                result.status = "FactorGraph BDDs merged."
                continue
            m = pattern_fg_created.match(line)
            if m is not None:
                result.factorGraphCreationTime = float(m.group(1))
                result.status = "FactorGraph created"
                continue
            m = pattern_fg_converged.match(line)
            if m is not None:
                result.factorGraphConvergenceStats = f"{m.group(1)} iters, {f2s(float(m.group(2)))} sec"
                result.status = "FactorGraph converged"
                result.factorGraphResult = ", NonTrivial"
                continue
            if line.startswith(pattern_fg_one):
                result.factorGraphResult = ", ONE"
                continue
            if line.startswith(pattern_fg_zero):
                result.factorGraphResult = ", ZERO"
                continue
            if line.startswith(pattern_mus_started):
                result.status = "Mus running"
                result.musStarted = True
                continue
            m = pattern_mus_explored.match(line)
            if m is not None:
                result.musExplorationTime = result.musExplorationTime + float(m.group(1))
                continue
            m = pattern_mus_clause.match(line)
            if m is not None:
                result.musNumClauses = result.musNumClauses + 1
                result.musRounds = result.musRounds + 1
                continue
            m = pattern_mus_processed.match(line)
            if m is not None:
                result.musProcessingTime = result.musProcessingTime + float(m.group(1))
                continue
            m = pattern_mus_disabled_sets.match(line)
            if m is not None:
                result.musNumDisabledSets = result.musNumDisabledSets + int(m.group(1))
                result.musRounds = result.musRounds + 1
                continue
            m = pattern_fg_timeout.match(line)
            if m is not None:
                if result.musStarted:
                    result.status = "MUS timed out"
                else:
                    result.status = "FG timed out"
                result.fgmusTimedOut = True
                result.fgmusTime = f2s(float(m.group(1)))
                continue
            m = pattern_fg_finished.match(line)
            if m is not None and not result.fgmusTimedOut:
                result.status = "Preproc finished unsolved, FG " + ("skipped" if result.fgDelibSkip else "finished") + ", MUS " + ("skipped" if result.musDelibSkip else "finished")
                result.fgmusTime = f2s(float(m.group(1)))
                continue
            m = pattern_fg_delib_skip.match(line)
            if m is not None:
                result.fgDelibSkip = True
                continue
            m = pattern_mus_delib_skip.match(line)
            if m is not None:
                result.musDelibSkip = True
                continue


    return result






def print_help():
    print("jan_24/summarize.py")
    print("Utility to summarize the results from a run of jan_24/run_experiment.py")
    print("Usage:")
    print("\tpython3 jan_24/summarize.py [-h|--help] folder1 [folder2 [folder3 ...]]]")
    print("\t\t-h|--help: print this help and exit")
    print("\t\tfolder: output folder of a run of jan_24/run_experiment.py")







#TODO
def find_test_cases(results_folder: str) -> list[str]:
    return [f for f in os.listdir(results_folder) if f.endswith(".qdimacs.log")]






def tabulate(summaries: list[TestCaseSummary]) -> list[str]:
    result = [TestCaseSummary.headers()] + [summary.row() for summary in summaries]
    add_padding(result)
    return [" | ".join(row) + "\n" for row in result]






## do actual work on a folder ##
def summarize(results_folder: str) -> None:
    test_cases = find_test_cases(results_folder)
    summaries = [parse_test_case(results_folder, test_case) for test_case in test_cases]
    table = tabulate(summaries)
    summary_file_path = os.path.join(results_folder, "summary.txt")
    with open(summary_file_path, 'w') as fout:
        fout.writelines(table)







## main entry point function ##
def main(argv: list[str]) -> int:

    ## check for help ##
    help_options = ["--help", "-h"]
    for help_option in help_options:
        if help_option in argv:
            print_help()
            return 0
        
    ## do actual work ##
    for results_folder in argv:
        summarize(results_folder)
        
    return 0







## python main entry point ##
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))