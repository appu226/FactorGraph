#pragma once

#include <istream>
#include <string>
#include <vector>

namespace jan_24 {

    class ICnfSaxParser {
    
    public:
        void parse(std::istream& cnfStream);
        virtual void parseComment(const std::string& line) = 0;
        virtual void parsePHeader(const std::string& line, int numVars, int numClauses) = 0;
        virtual void parseQuantifierLine(const std::string& line, char quantifier, const std::vector<int> & literalsTerminatedWithZero) = 0;
        virtual void parseClause(const std::string& line, const std::vector<int> & literalsTerminatedWithZero) = 0;

        virtual ~ICnfSaxParser() {}
    };

} // end namespace jan_24