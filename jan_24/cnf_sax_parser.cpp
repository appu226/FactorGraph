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

#include <regex>
#include <stdexcept>



namespace {

    static std::regex p_header_regex("p cnf ([0-9]*) ([0-9]*)", std::regex_constants::ECMAScript | std::regex_constants::icase);

} // end anonymous namespace




namespace jan_24 {

    void ICnfSaxParser::parse(std::istream& cnfStream)
    {
        std::string line;
        std::vector<int> literalsTerminatedWithZero;
        while (getline(cnfStream, line))
        {
            if (line.size() == 0)
                continue;
            else if (line[0] == 'c' || line[0] == '/' || line[0] == '#')
                parseComment(line);
            else if (line[0] == 'p')
            {
                std::smatch p_header_match;
                if (!std::regex_match(line, p_header_match, p_header_regex))
                    throw std::runtime_error("Could not match p header line {" + line + '}');
                int numVars = std::atoi(p_header_match[1].str().c_str());
                int numClauses = std::atoi(p_header_match[2].str().c_str());
                parsePHeader(line, numVars, numClauses);
            }
            else if (line[0] == 'a' || line[0] == 'e')
            {
                std::stringstream liness(line);
                char quantifier;
                liness >> quantifier;
                literalsTerminatedWithZero.clear();
                int literal;
                while(liness >> literal)
                    literalsTerminatedWithZero.push_back(literal);
                parseQuantifierLine(line, quantifier, literalsTerminatedWithZero);
            }
            else if (line[0] == '-' || (line[0] >= '0' && line[0] <= '9'))
            {
                literalsTerminatedWithZero.clear();
                int literal;
                std::stringstream liness (line);
                while(liness >> literal)
                    literalsTerminatedWithZero.push_back(literal);
                parseClause(line, literalsTerminatedWithZero);
            }
            else
                throw std::runtime_error("Could not parse qdimacs line {" + line + '}');
        }
    }

} // end namespace jan_24