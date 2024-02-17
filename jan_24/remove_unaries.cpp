/*

Copyright 2024 Parakram Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <blif_solve_lib/log.h>
#include <blif_solve_lib/clo.hpp>
#include "cnf_sax_parser.h"
#include <fstream>
#include <memory>
#include <set>
#include <unordered_set>






class UnaryCollector: public jan_24::ICnfSaxParser
{
    public:
    void parseComment(const std::string& line) override { }
    void parsePHeader(const std::string& line, int numVars, int numClauses) override { }
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override { }
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;

    std::unordered_set<int> unaries;
};






class ClauseCounter: public jan_24::ICnfSaxParser
{
    public:
    using LiteralSetRef = std::unordered_set<int> const &;
    ClauseCounter(LiteralSetRef v_unaries);
    void parseComment(const std::string& line) override { }
    void parsePHeader(const std::string& line, int numVars, int numClauses) override { }
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override { }
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;
    size_t getNumClauses() const;

    private:
    LiteralSetRef unaries;
    std::set<std::set<int> > clauses;
};







class ResultWriter: public jan_24::ICnfSaxParser
{
    public:
    using LiteralSetRef = std::unordered_set<int> const &;
    ResultWriter(LiteralSetRef unaries, size_t numClauses, const std::string& outputFileName);
    void parseComment(const std::string& line) override;
    void parsePHeader(const std::string& line, int numVars, int numClauses) override;
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override;
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;

    private:
    LiteralSetRef m_unaries;
    size_t m_numClauses;
    std::set<std::set<int> > m_clauses;
    std::ofstream m_fout;

};








struct CommandLineOptions {
    std::string inputFile;
    std::string outputFile;
    blif_solve::Verbosity verbosity;
    static CommandLineOptions parse(int argc, char const * const * const argv);
};







int main(int argc, char const * const * const argv)
{
    auto clo = CommandLineOptions::parse(argc, argv);
    blif_solve::setVerbosity(clo.verbosity);
    blif_solve_log(INFO, "Starting remove_unaries on " << clo.inputFile << " to " << clo.outputFile);

    UnaryCollector uc;
    {
        std::ifstream fin(clo.inputFile);
        uc.parse(fin);
    }
    blif_solve_log(DEBUG, "Found " << uc.unaries.size() << " unaries.");

    size_t numClauses = 0;
    {
        ClauseCounter cc(uc.unaries);
        std::ifstream fin(clo.inputFile);
        cc.parse(fin);
        numClauses = cc.getNumClauses();
    }

    ResultWriter rw(uc.unaries, numClauses, clo.outputFile);
    {
        std::ifstream fin(clo.inputFile);
        rw.parse(fin);
    }

    blif_solve_log(INFO, "Finished remove_uniaries on " << clo.inputFile << " to " << clo.outputFile);
    return 0;
}









CommandLineOptions CommandLineOptions::parse(int argc, char const * const * const argv)
{
    using blif_solve::CommandLineOption;
    auto inputFile =
        std::make_shared<CommandLineOption<std::string> >("--inputFile", "Input QDimacs file with unaries.", true);
    auto outputFile =
        std::make_shared<CommandLineOption<std::string> >("--outputFile", "Output QDimacs file with unaries removed.", true);
    auto verbosity =
        std::make_shared<CommandLineOption<std::string> >("--verbosity", "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG, default ERROR)", "ERROR");
    blif_solve::parse(
        {inputFile, outputFile, verbosity},
        argc, argv
    );
    return CommandLineOptions{
        inputFile->value.value(), outputFile->value.value(), blif_solve::parseVerbosity(verbosity->value.value())
    };
}








void UnaryCollector::parseClause(
    const std::string& line, 
    const std::vector<int> & literalsTerminatedWithZero)
{
    if (literalsTerminatedWithZero.size() != 2 || literalsTerminatedWithZero[1] != 0)
        return;
    int unary = literalsTerminatedWithZero[0];
    if (unaries.count(-unary) > 0)
        throw std::runtime_error("Found conflicting unaries " + std::to_string(unary) + " and " + std::to_string(-unary));
    unaries.insert(unary);
}








ClauseCounter::ClauseCounter(LiteralSetRef v_unaries) :
        unaries(v_unaries),
        clauses()
{ }
void ClauseCounter::parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero)
{
    std::set<int> resultingClause;
    for (const auto l: literalsTerminatedWithZero)
    {
        if (l == 0) continue;
        if (unaries.count(l) > 0) return; // l is true, clause should be eliminated
        if (unaries.count(-l) > 0) continue; // l is false, l should be eliminated from clause
        resultingClause.insert(l);
    }
    if (resultingClause.empty())
        return;
    clauses.insert(resultingClause);
}
size_t ClauseCounter::getNumClauses() const
{
    return clauses.size();
}







ResultWriter::ResultWriter(LiteralSetRef unaries, size_t numClauses, const std::string& outputFileName) :
        m_unaries(unaries),
        m_numClauses(numClauses),
        m_clauses(),
        m_fout(outputFileName)
{ }
void ResultWriter::parseComment(const std::string& line)
{
    m_fout << line << '\n';
}
void ResultWriter::parsePHeader(const std::string& line, int numVars, int)
{
    m_fout << "p cnf " << numVars << ' ' << m_numClauses << '\n';
    blif_solve_log(DEBUG, "Remove Unaries writing result with " << numVars << " vars and " << m_numClauses << " clauses");
}
void ResultWriter::parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero)
{
    m_fout << line << '\n';
}
void ResultWriter::parseClause(const std::string&, const std::vector<int> & literalsTerminatedWithZero)
{
    std::set<int> resultingClauseSet;
    std::vector <int> resultingClauseVec;
    for (const auto l: literalsTerminatedWithZero)
    {
        if (l == 0) continue;
        if (m_unaries.count(l) > 0) return; // l is true, clause should be eliminated
        if (m_unaries.count(-l) > 0) continue; // l is false, l should be eliminated from clause
        resultingClauseSet.insert(l);
        resultingClauseVec.push_back(l);
    }
    if (resultingClauseVec.empty()) return;
    if (m_clauses.count(resultingClauseSet) > 0) return; // clause already written.
    for (const auto l: resultingClauseVec)
        m_fout << l << ' ';
    m_fout << "0\n";
}