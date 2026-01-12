/*

Copyright 2025 Parakram Majumdar

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

#pragma once

#include <dd/qdimacs.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>

namespace oct_22
{
    struct Ave2Clause
    {
        using CPtr = std::shared_ptr<const Ave2Clause>;

        // clause literals
        std::vector<int> literals;
        Ave2Clause(std::vector<int> v_literals, bool sortAndUnique = true);
        bool operator==(const Ave2Clause& that) const
        {
            return this->literals == that.literals;
        }
        bool operator <(const Ave2Clause& that) const
        {
            return this->literals < that.literals;
        }

        CPtr intersect(const Ave2Clause& that) const;
        CPtr concatenate(const Ave2Clause& that) const;
        CPtr subtract(const Ave2Clause& that) const;
        std::optional<CPtr> resolveOnVar(size_t var, const Ave2Clause& that) const;
    };

    using Ave2ClauseCPtr = Ave2Clause::CPtr;
    using Ave2ClauseVec = std::shared_ptr<std::vector<Ave2ClauseCPtr> >;

    struct Ave2ClauseMap
    {
        std::unordered_map<int, Ave2ClauseVec> clauseMap;
        void insert(Ave2ClauseCPtr const& clause);
    };


    class Ave2
    {
        public:
        using Ptr = std::shared_ptr<Ave2>;
        static Ptr parseQdimacs(const dd::Qdimacs& qdimacs);
        Ave2ClauseVec approximatelyEliminateAllVariables(size_t searchDepth);

        static Ave2ClauseVec filterOutClausesWithNoVarsToEliminate(Ave2ClauseVec& clauses, Ave2Clause const& literalsToEliminate);

        private:

        Ave2ClauseVec m_clauses;
        Ave2ClauseMap m_literalToClause;
        Ave2ClauseCPtr m_varsToEliminate;
        Ave2ClauseCPtr m_literalsToEliminate;

        void growSeed(
            Ave2ClauseCPtr const& seed, 
            Ave2ClauseVec& resultClauses, 
            size_t searchDepth, 
            Ave2Clause const& alreadyEliminatedLiterals, 
            std::unordered_map<int, Ave2ClauseCPtr>& literalToResolverMap);

    };

} // end namespace oct_22