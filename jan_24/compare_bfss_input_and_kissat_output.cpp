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
#include "cnf_sax_parser.h"
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>


extern "C" {

    // set log verbosity
    void setVerbosity(char const * const verbosity);

    // compare 2 qdimacs files
    // RETURN: 0 -> mis-match
    //         1 -> match
    //         anything else -> error
    int bfss_input_equals_kissat_output(char const * const bfss_input_path, char const * const kissat_output_path);
}






struct QDimacs {
    using Clause = std::set<int>;
    using Quantification = std::pair<char, Clause>;

    int numVars;
    std::vector<Quantification> quantifiers;
    std::set<Clause> clauses;
    
    bool operator ==(const QDimacs& that) const;
};


class QDimacsParser: private jan_24::ICnfSaxParser
{
public:
    using UPtr = std::unique_ptr<QDimacsParser const>;
    static UPtr parseFile(std::istream& file);

    QDimacs qdimacs;

private:
    QDimacsParser(std::istream& file);

    void parseComment(const std::string& line) override;
    void parsePHeader(const std::string& line, int numVars, int numClauses) override;
    void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) override;
    void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) override;

};















void setVerbosity(char const * const verbosity)
{
    blif_solve::setVerbosity(blif_solve::parseVerbosity(verbosity));
}



// RETURN: 0 -> mis-match
//         1 -> match
//         anything else -> error
int bfss_input_equals_kissat_output(char const * const bfss_input_path, char const * const kissat_output_path)
{
    blif_solve_log(DEBUG, "Comparing " << bfss_input_path << " and " << kissat_output_path);
    try {
        std::ifstream bfss_input(bfss_input_path);
        auto bfss_parsed = QDimacsParser::parseFile(bfss_input);
        std::ifstream kissat_output(kissat_output_path);
        auto kissat_parsed = QDimacsParser::parseFile(kissat_output);
        bool match = (bfss_parsed->qdimacs == kissat_parsed->qdimacs);
        return match ? 1 : 0;
    } 
    catch (const std::bad_alloc&)
    {
        blif_solve_log(ERROR, "bad_alloc in compare_bfss_input_and_kissat_output()");
        throw;
    }
    catch (const std::exception& e)
    {
        blif_solve_log(ERROR, "Exception {" << e.what() << "} in compare_bfss_input_and_kissat_output()")
        return 2;
    }
    catch (...)
    {
        blif_solve_log(ERROR, "Unknown exception in compare_bfss_input_and_kissat_output()")
        return 2;
    }
}








bool QDimacs::operator==(const QDimacs& that) const
{
    return numVars == that.numVars
            && quantifiers == that.quantifiers 
            && clauses == that.clauses;
}








QDimacsParser::UPtr QDimacsParser::parseFile(std::istream& file)
{
    return std::unique_ptr<QDimacsParser const>(new QDimacsParser(file));
}

QDimacsParser::QDimacsParser(std::istream& file)
{
    this->parse(file);
}

void QDimacsParser::parseComment(const std::string& ) { }
void QDimacsParser::parsePHeader(const std::string& , int numVars, int)
{
    qdimacs.numVars = numVars;
}
void QDimacsParser::parseQuantifierLine(const std::string&, char quantifier, const std::vector<int> & literalsTerminatedWithZero)
{
    qdimacs.quantifiers.emplace_back(quantifier, std::set<int>(literalsTerminatedWithZero.cbegin(), literalsTerminatedWithZero.cend()));
}
void QDimacsParser::parseClause(const std::string& , const std::vector<int> & literalsTerminatedWithZero)
{
    qdimacs.clauses.insert(std::set<int>(literalsTerminatedWithZero.cbegin(), literalsTerminatedWithZero.cend()));
}