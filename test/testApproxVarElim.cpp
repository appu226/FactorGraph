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

#include "testApproxVarElim.h"

#include <dd/qdimacs.h>
#include <fstream>
#include <iostream>
#include <dd/bdd_factory.h>
#include <dd/dd.h>

void testApproxVarElim(DdManager * manager)
{
    std::ifstream fin("test/data/adder.qdimacs");
    auto qdimacs = dd::Qdimacs::parseQdimacs(fin);
    auto ave = oct_22::ApproxVarElim::parseQdimacs(*qdimacs);
    ave->approximatelyEliminateAllVariables(0);
    auto const& clauses = ave->getClauses();
    dd::BddWrapper over_approx_result = dd::BddWrapper(bdd_one(manager), manager);
    for (auto const& clause: clauses)
    {
        auto bdd_clause = over_approx_result.zero();
        for (auto const& var: clause->literals)
        {
            dd::BddWrapper bdd_var(bdd_new_var_with_index(manager, (var > 0 ? var : -var)), manager);
            if (var < 0) bdd_var = -bdd_var;
            bdd_clause = bdd_clause + bdd_var;
        }
        over_approx_result = over_approx_result * bdd_clause;
    }

    std::ifstream expected_fin("test/expected_outputs/adder.cnf");
    auto expected_qdimacs = dd::Qdimacs::parseQdimacs(expected_fin);
    dd::BddWrapper expected_result = dd::BddWrapper(bdd_one(manager), manager);
    for (auto const& clause: expected_qdimacs->clauses)
    {
        auto bdd_clause = expected_result.zero();
        for (auto const& var: clause)
        {
            dd::BddWrapper bdd_var(bdd_new_var_with_index(manager, (var > 0 ? var : -var)), manager);
            if (var < 0) bdd_var = -bdd_var;
            bdd_clause = bdd_clause + bdd_var;
        }
        expected_result = expected_result * bdd_clause;
    }

    if (!(expected_result * -over_approx_result).isZero())
    {
        throw std::runtime_error("Error: ApproxVarElim did not produce an overapproximation");
    }
    
}