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
#include <fstream>
#include <iostream>
#include <jan_24/cnf_sax_parser.h>
#include <memory>
#include <stdexcept>
#include <string>




struct CommandLineOptions
{
    std::string file1;
    std::string file2;
    std::string verbosity;

    using UPtr = std::unique_ptr<CommandLineOptions>;

    static UPtr create(int argc, char const * const * const argv);
};
int main(int argc, char const * const * const argv);
std::unique_ptr<dd::ManagerWrapper> ddm_init();
dd::BddWrapper parse_bdd(const std::string& file, DdManager* ddm);
struct BddParser: public jan_24::ICnfSaxParser
{
    dd::BddWrapper m_bdd;
    DdManager* m_ddm;
    BddParser(DdManager* ddm);
    virtual void parseComment(const std::string& line) override {}
    virtual void parsePHeader(const std::string& line, int numVars, int numClauses) override {}
    virtual void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override {}
    virtual void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;
};





int main(int argc, char const * const * const argv)
{
    auto clo = CommandLineOptions::create(argc, argv);
    auto manager = ddm_init();
    auto bdd1 = parse_bdd(clo->file1, manager->manager);
    auto bdd2 = parse_bdd(clo->file2, manager->manager);
    if (bdd1 == bdd2)
    {
        blif_solve_log(INFO, "MATCH");
    }
    else
    {
        blif_solve_log(INFO, "MISMATCH");
        blif_solve_log_bdd(DEBUG, "bdd1:", manager->manager, bdd1.getUncountedBdd());
        blif_solve_log_bdd(DEBUG, "bdd2:", manager->manager, bdd2.getUncountedBdd());
    }
    return 0;
}




CommandLineOptions::UPtr CommandLineOptions::create(int argc, char const * const * const argv)
{
    auto file1 = std::make_shared<blif_solve::CommandLineOption<std::string> >("--file1", "path to cnf file 1", true);
    auto file2 = std::make_shared<blif_solve::CommandLineOption<std::string> >("--file2", "path to cnf file 2", true);
    auto verbosity = std::make_shared<blif_solve::CommandLineOption<std::string> >("--verbosity", "log verbosity", false, std::make_optional<std::string>("INFO"));

    blif_solve::parse({file1, file2, verbosity}, argc, argv, true);
    auto result = std::make_unique<CommandLineOptions>(CommandLineOptions{file1->value.value(), file2->value.value(), verbosity->value.value()});
    blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity->value.value()));
    return result;
}




std::unique_ptr<dd::ManagerWrapper> ddm_init()
{
    auto ddm = std::make_unique<dd::ManagerWrapper>(Cudd_Init(0, 0, 256, 262144, 0));
    if (!ddm->manager)
        throw std::runtime_error("Cudd initialization failed.");
    return ddm;
}




dd::BddWrapper parse_bdd(const std::string& file, DdManager* ddm)
{
    std::ifstream fin(file);
    BddParser parser(ddm);
    parser.parse(fin);
    return parser.m_bdd;
}




BddParser::BddParser(DdManager* ddm):
    m_bdd(bdd_one(ddm), ddm),
    m_ddm(ddm)
{ }




void BddParser::parseClause(const std::string& , const std::vector<int> & literalsTerminatedWithZero)
{
    auto clause = m_bdd.zero();
    for (const auto& l: literalsTerminatedWithZero)
    {
        if (l != 0)
        {
            auto v = (l > 0 ? l : -l);
            auto vbdd = dd::BddWrapper(bdd_new_var_with_index(m_ddm, v), m_ddm);
            auto lbdd = (l > 0 ? vbdd : -vbdd);
            clause = clause + lbdd;
        }
    }
    m_bdd = m_bdd * clause;

}