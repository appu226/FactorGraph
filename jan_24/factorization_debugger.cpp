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

#include <blif_solve_lib/clo.hpp>
#include <blif_solve_lib/log.h>
#include <dd/bdd_factory.h>
#include <jan_24/cnf_sax_parser.h>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>








struct CommandLineOptions {
    std::string inputFile;
    std::vector<int> bitVars;
    CommandLineOptions(int argc, char const * const * argv);
    typedef std::unique_ptr<CommandLineOptions const> UCPtr;
};

class CnfToBdd: public jan_24::ICnfSaxParser {
    public:
        void parseComment(const std::string& line) override;
        void parsePHeader(const std::string& line, int numVars, int numClauses) override;
        void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override;
        void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;
        
        CnfToBdd(DdManager* v_ddm);
        DdManager* ddm;
        dd::BddWrapper cnfBdd;
};

int main(int const argc, char const * const * const argv);
dd::BddWrapper generateNumBdd(DdManager* ddm, const std::vector<int>& bitVars, int number);
bool checkFactorizationViaBdd(const dd::BddWrapper& cnf, int number, const std::vector<int>& bitVars);
bool checkFactorizationViaMath(int number, int maxNumber);
std::unique_ptr<dd::ManagerWrapper> ddm_init();
dd::BddWrapper parseCnfBdd(const std::string& qdimacsFile, DdManager* ddm);
char const * const b2s(bool v);





int main(int const argc, char const * const * const argv)
{
    auto clo = std::make_unique<CommandLineOptions>(argc, argv);
    auto ddm = ddm_init();
    auto cnfBdd = parseCnfBdd(clo->inputFile, ddm->manager);
    const int MaxNumber = (1 << clo->bitVars.size()) - 1;
    for (int n = 1; n <=  MaxNumber; ++n)
    {
        auto expected = checkFactorizationViaMath(n, MaxNumber);
        auto computed = checkFactorizationViaBdd(cnfBdd, n, clo->bitVars);
        if (expected == computed)
        {
            blif_solve_log(DEBUG, n << " PASSed (expected: " << b2s(expected) << ", computed: " << b2s(computed) << ")");
        }
        else
        {
            blif_solve_log(INFO, "\x1B[31m" << n << " FAILed\033[0m (expected: " << b2s(expected) << ", computed: " << b2s(computed) << ")");
        }
    }
    blif_solve_log(INFO, "DONE");
    return 0;
}







void CnfToBdd::parseComment(const std::string& )
{ }
void CnfToBdd::parsePHeader(const std::string& , int , int )
{ }
void CnfToBdd::parseQuantifierLine(const std::string& , char , const std::vector<int> & )
{ }
void CnfToBdd::parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero)
{
    dd::BddWrapper clause(bdd_zero(ddm), ddm);
    for (int lit: literalsTerminatedWithZero)
    {
        if (lit == 0)
            continue;
        dd::BddWrapper litBdd(bdd_new_var_with_index(ddm, std::abs(lit)), ddm);
        if (lit < 0)
            litBdd = -litBdd;
        clause = clause + litBdd;
    }
    cnfBdd = cnfBdd * clause;
}

CnfToBdd::CnfToBdd(DdManager* v_manager) :
    ddm(v_manager),
    cnfBdd(bdd_one(v_manager), v_manager)
{ }







CommandLineOptions::CommandLineOptions(int argc, char const * const * argv)
{
    using blif_solve::CommandLineOption;
    auto c_inputFile = std::make_shared<CommandLineOption<std::string> >("--inputFile", "Input qdimacs file", true);
    auto c_bitVars = std::make_shared<CommandLineOption<std::string> >("--bitVars", "Comma separated list of bit variables (most to least significant)", true);
    auto c_verbosity = std::make_shared<CommandLineOption<std::string> >("--verbosity", "Log verbosity (QUIET/ERROR/WARNING/INFO/DEBUG, default ERROR)", false, "ERROR");
    blif_solve::parse({c_inputFile, c_bitVars, c_verbosity}, argc, argv, true);
    blif_solve::setVerbosity(blif_solve::parseVerbosity(c_verbosity->value.value()));
    blif_solve_log(DEBUG, "Setting log verbosity to " << c_verbosity->value.value());
    inputFile = c_inputFile->value.value();
    blif_solve_log(DEBUG, "Setting input file to " << inputFile);
    std::string bitVarStr;
    std::stringstream bitVarSs(c_bitVars->value.value());
    while (std::getline(bitVarSs, bitVarStr, ','))
    {
        if (bitVarStr.size() > 0 && bitVarStr[bitVarStr.size() - 1] == ',')
            bitVarStr = bitVarStr.substr(0, bitVarStr.size() - 1);
        if (!bitVarStr.empty())
        {
            bitVars.push_back(std::atoi(bitVarStr.c_str()));
            blif_solve_log(DEBUG, "Adding bit var " << bitVarStr);
        }
    }
}






dd::BddWrapper generateNumBdd(DdManager* manager, const std::vector<int>& bitVars, int number)
{
    int orig = number;
    dd::BddWrapper result(bdd_one(manager), manager);
    for (auto digit = bitVars.rbegin(); digit != bitVars.rend(); ++digit)
    {
        dd::BddWrapper l(bdd_new_var_with_index(manager, std::abs(*digit)), manager);
        if (number % 2 == 0)
            l = -l;
        result = result * l;
        number = (number >> 1);
    }
    // blif_solve_log_bdd(DEBUG, "bdd for " << orig, manager, result.getUncountedBdd());
    return result;
}






bool checkFactorizationViaBdd(const dd::BddWrapper& cnf, int number, const std::vector<int>& bitVars)
{
    auto numBdd = generateNumBdd(cnf.getManager(), bitVars, number);
    auto newBdd = cnf * numBdd;
    return !newBdd.isZero();
}







bool checkFactorizationViaMath(int number, int maxNumber)
{
    for (int i = 2; i * i <= number; ++i)
    {
        if (number % i == 0 && (number / i) * (number / i) < maxNumber)
            return true;
    }
    return false;
}






std::unique_ptr<dd::ManagerWrapper> ddm_init()
{
    auto ddm = std::make_unique<dd::ManagerWrapper>(Cudd_Init(0, 0, 256, 262144, 0));
    if (!ddm->manager)
        throw std::runtime_error("Cudd initialization failed.");
    return ddm;
}





dd::BddWrapper parseCnfBdd(const std::string& qdimacsFile, DdManager* ddm)
{
    blif_solve_log(DEBUG, "Parsing cnf bdd");
    CnfToBdd parser(ddm);
    std::ifstream fin(qdimacsFile);
    parser.parse(fin);
    blif_solve_log(INFO, "Parsed cnf bdd");
    return parser.cnfBdd;
}





char const * const b2s(bool v)
{
    return (v ? "TRUE": "FALSE");
}