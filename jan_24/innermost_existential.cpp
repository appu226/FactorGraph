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

#include "cnf_sax_parser.h"

#include <fstream>
#include <unordered_set>

#include <blif_solve_lib/clo.hpp>
#include <blif_solve_lib/log.h>

struct Clo {
    std::string inputFile;
    std::string outputFile;
    bool addUniversalQuantifier;
    blif_solve::Verbosity verbosity;

    static Clo parse(int argc, char const * const * const argv);
};





class InnermostExistentialSaxParser: public jan_24::ICnfSaxParser
{
    public:

    InnermostExistentialSaxParser(const std::string& outFile, bool addUniversalQuantifier) :
        m_fout(outFile), 
        m_addUniversalQuantifier(addUniversalQuantifier),
        m_quantifiersPrinted(false),
        m_numVars(0),
        m_innermostExistentialLine(),
        m_existentiallyQuantifiedVariables()
    { }

    void parseComment(const std::string& line) override;
    void parsePHeader(const std::string& line, int numVars, int numClauses) override;
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override;
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;

    private:
    std::ofstream m_fout;
    bool m_addUniversalQuantifier;
    bool m_quantifiersPrinted;
    int m_numVars;
    std::string m_innermostExistentialLine;
    std::unordered_set<int> m_existentiallyQuantifiedVariables;
};







int main(int argc, char const * const * const argv) {
    auto clo = Clo::parse(argc, argv);

    blif_solve_log(INFO, "Starting innermost_existential (" << clo.inputFile << " -> " << clo.outputFile << ")");
    
    InnermostExistentialSaxParser cnfParser(clo.outputFile, clo.addUniversalQuantifier);
    std::ifstream fin(clo.inputFile);
    cnfParser.parse(fin);

    blif_solve_log(INFO, "Finished innermost_existential");
    return 0;
}










void InnermostExistentialSaxParser::parseComment(const std::string& line) { 
    m_fout << line << '\n'; 
}
void InnermostExistentialSaxParser::parsePHeader(
    const std::string& line, 
    int numVars, 
    int numClauses
) { 
    m_numVars = numVars;
    m_fout << line << '\n';
    blif_solve_log(DEBUG, "innermost_existential writing result with " << numVars << " vars and " << numClauses << " clauses");
}
void InnermostExistentialSaxParser::parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int>& literalsTerminatedWithZero) {
    if (quantifier == 'e')
    {
        m_existentiallyQuantifiedVariables.insert(literalsTerminatedWithZero.cbegin(), literalsTerminatedWithZero.cend());
        m_innermostExistentialLine = line;
    }
    for (const int literal: literalsTerminatedWithZero)
    {
        if (std::abs(literal) > m_numVars)
            m_numVars = std::abs(literal);
    }
}

void InnermostExistentialSaxParser::parseClause(
    const std::string& line, 
    const std::vector<int>& // literalsTerminatedWithZero unused parameter
) {
    if (!m_quantifiersPrinted)
    {
        if (m_addUniversalQuantifier)
        {
            std::stringstream uline;
            uline << "a ";
            for (int v = 1; v <= m_numVars; ++v)
            {
                if (m_existentiallyQuantifiedVariables.count(v) == 0)
                {
                    uline << v << ' ';
                }
            }
            m_fout << uline.str() << 0 << '\n';
        }
        m_fout << m_innermostExistentialLine << '\n';
        m_quantifiersPrinted = true;
    }
    m_fout << line << '\n';
    
}






Clo Clo::parse(int argc, char const * const * const argv) {
    using blif_solve::CommandLineOption;
    auto inputFile = std::make_shared<CommandLineOption<std::string> >(
        "--inputFile", "Input QDimacs file path", true
    );
    auto outputFile = std::make_shared<CommandLineOption<std::string> >(
        "--outputFile", "Output QDimacs file path", true
    );
    auto addUniversalQuantifier = std::make_shared<CommandLineOption<bool> >(
        "--addUniversalQuantifier", "Whether to add a universal quantifier with remaining variables (0/1, default 0)", false,
        std::optional<bool>(false)
    );
    auto verbosity = std::make_shared<CommandLineOption<std::string> >(
        "--verbosity",
        "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG)",
        false,
        std::string("ERROR")
    );
    
    blif_solve::parse({inputFile, outputFile, addUniversalQuantifier, verbosity}, argc, argv);

    Clo clo{
        inputFile->value.value(),
        outputFile->value.value(),
        addUniversalQuantifier->value.value(),
        blif_solve::parseVerbosity(verbosity->value.value())
    };

    blif_solve::setVerbosity(clo.verbosity);

    return clo;
}









