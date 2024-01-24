#include <blif_solve_lib/log.h>
#include <blif_solve_lib/clo.hpp>
#include "cnf_sax_parser.h"
#include <fstream>
extern "C" {
    #include <kissat/src/kissat.h>
    #include <kissat/src/eliminate.h>
    #include <kissat/src/dump.h>
}
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

struct CommandLineOptions {
    std::string inputFile;
    std::string outputFile;
    std::string verbosity;
    static CommandLineOptions parse(int argc, char const * const * const argv);
};

int get_kissat_verbosity();


class KissatWrapper: public jan_24::ICnfSaxParser
{
    public:
    KissatWrapper();
    ~KissatWrapper();

    void parseComment(const std::string& line) override { }
    void parsePHeader(const std::string& line, int numVars, int numClauses) override;
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override;
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;

    void dumpToFile(const std::string& fileName);

    struct kissat* solver;
    int numVars;
    std::vector<int> quantifiedVars;

    KissatWrapper(const KissatWrapper&) = delete;
};


int main(int argc, char const * const * const argv)
{
    auto clo = CommandLineOptions::parse(argc, argv);
    blif_solve_log(INFO, "Starting kissat_preprocess.");

    KissatWrapper kw;
    auto solver = kw.solver;
    kissat_set_option(solver, "verbose", get_kissat_verbosity());
    {
        std::ifstream fin(clo.inputFile);
        kw.parse(fin);
    }

    kissat_eliminate_variables(kw.solver, kw.quantifiedVars.data(), kw.quantifiedVars.size());

    blif_solve_log(DEBUG, "Writing to " << clo.outputFile);
    kw.dumpToFile(clo.outputFile);

    blif_solve_log(INFO, "Finished kissat_preprocess.");
    return 0;

}












CommandLineOptions CommandLineOptions::parse(int argc, char const * const * const argv)
{
    using blif_solve::CommandLineOption;


    auto inputFile = std::make_shared<CommandLineOption<std::string> >(
        "--inputFile", "Path to input noUnary QDimacs file.",
        true, std::optional<std::string>()
    );
    auto outputFile = std::make_shared<CommandLineOption<std::string> >(
        "--outputFile", "Path to final QDimacs file.",
        true, std::optional<std::string>()
    );
    auto verbosity = std::make_shared<CommandLineOption<std::string> >(
        "--verbosity",
        "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG, defualt ERROR)",
        false, "ERROR"
    );

    blif_solve::parse({ verbosity, inputFile, outputFile }, argc, argv);
    blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity->value.value()));

    return CommandLineOptions {
        inputFile->value.value(),
        outputFile->value.value(),
        verbosity->value.value()
    };
}






int get_kissat_verbosity()
{
    auto v = blif_solve::getVerbosity();
    switch (v) {
        case blif_solve::QUIET:
            return 0;
        case blif_solve::ERROR:
            return 0;
        case blif_solve::WARNING:
            return 0;
        case blif_solve::INFO:
            return 1;
        case blif_solve::DEBUG:
            return 3;
    }
    return 3;
}






KissatWrapper::KissatWrapper() : solver(kissat_init()) { }

KissatWrapper::~KissatWrapper()
{
    kissat_release(solver);
}

void KissatWrapper::parsePHeader(const std::string&, int parsedNumVars, int parsedNumClauses)
{
    blif_solve_log(INFO, "kissat_preprocess processing problem with " << parsedNumVars << " total variables and " << parsedNumClauses << " clauses.");
    numVars = parsedNumVars;
}

void KissatWrapper::parseQuantifierLine(const std::string&, char quantifier, const std::vector<int> & literalsTerminatedWithZero)
{
    if (quantifier == 'e')
    {
        blif_solve_log(INFO, "kissat_preprocess processing problem with " << (literalsTerminatedWithZero.size() - 1) << " quantified variables.");
        quantifiedVars = literalsTerminatedWithZero;
        quantifiedVars.pop_back();
    }
}

void KissatWrapper::parseClause(const std::string&, const std::vector<int> & literalsTerminatedWithZero)
{
    for (auto lit: literalsTerminatedWithZero)
        kissat_add(solver, lit);
}

void KissatWrapper::dumpToFile(const std::string& fileName)
{
    unsigned int result_size;
    std::unique_ptr<int[]> cnf_array(kissat_dump_cnf(solver, &result_size));

    std::unordered_set<int> remainingVars;
    int newNumClauses = 0;
    for (unsigned int i = 0; i < result_size; ++i)
    {
        if (cnf_array[i] != 0)
            remainingVars.insert(std::abs(cnf_array[i]));
        else
            ++newNumClauses;
    }
    std::set<int> newExistentiallyQuantifiedVars;
    for (auto qv: quantifiedVars)
        if (remainingVars.count(qv) > 0)
            newExistentiallyQuantifiedVars.insert(qv);
    std::vector<int> newUniversallyQuantifiedVars;
    for (int v = 1; v <= numVars; ++v)
        if (newExistentiallyQuantifiedVars.count(v) == 0)
            newUniversallyQuantifiedVars.push_back(v);

    std::ofstream fout(fileName);
    fout << "p cnf " << numVars << ' ' << newNumClauses << "\n";
    blif_solve_log(INFO, "kissat_preprocess writing results with " << numVars << " total vars, " << newExistentiallyQuantifiedVars.size() << " quantified variables, and " << newNumClauses << " clauses");
    if (newUniversallyQuantifiedVars.size() > 0)
    {
        fout << "a ";
        for (auto uv: newUniversallyQuantifiedVars)
            fout << uv << ' ';
        fout << "0\n";
    }

    if (newExistentiallyQuantifiedVars.size() > 0)
    {
        fout << "e ";
        for (auto ev: newExistentiallyQuantifiedVars)
            fout << ev << ' ';
        fout << "0\n";
    }

    for (auto i = 0; i < result_size; ++i)
    {
        fout << cnf_array[i] << (cnf_array[i] == 0 ? '\n' : ' ');
    }
    fout << std::endl;
    
}