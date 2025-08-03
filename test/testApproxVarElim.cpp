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
#include <sstream>


namespace {
    struct IntersectionTestCase {
        oct_22::AveIntVec literals;
        oct_22::AveIntVec variables;
        oct_22::AveIntVec expectedResult;

        void run() const {
            auto result = oct_22::ApproxVarElim::intersection(literals, variables);
            if (result != expectedResult) {
                throw std::runtime_error("Intersection test failed");
            }
        }

        static void testAll() {
            std::vector<IntersectionTestCase> testCases = {
                {{1, 2, 3}, {2, 3, 4}, {2, 3}},
                {{-3, -2, -1}, {2, 3, 4}, {-3, -2}},
                {{-4, -2, 3, 4}, {1, 2, 3, 4}, {-4, -2, 3, 4}},
                {{}, {1, 2, 3}, {}},
                {{1, 2, 3}, {}, {}},
                {{-3, -2, -1}, {}, {}},
                {{}, {}, {}}
            };
    
            for (const auto& testCase : testCases) {
                testCase.run();
            }
        }
    }; // end struct IntersectionTestCase


    struct NegatedLiteralsTestCase {
        oct_22::AveIntVec clauseLiterals;
        oct_22::AveIntVec seedLiterals;
        oct_22::AveIntVec expectedResult;

        void run() const {
            auto result = oct_22::ApproxVarElim::negatedLiterals(clauseLiterals, seedLiterals);
            if (result != expectedResult) {
                throw std::runtime_error(
                    "Negated literals test failed, Case:{ " 
                    + to_string() 
                    + " } Actual: { " 
                    + intvec_to_string(result) 
                    + "}");
            }
        }

        static std::string intvec_to_string(const oct_22::AveIntVec& vec) {
            std::stringstream ss;
            for (const auto& v : vec) {
                ss << v << ' ';
            }
            return ss.str();
        }

        std::string to_string() const {
            std::stringstream result;
            result << "Clause: [";
            for (const auto& lit : clauseLiterals) {
                result << lit << ' ';
            }
            result << "], Seed: [";
            for (const auto& lit : seedLiterals) {
                result << lit << ' ';
            }
            result << "], Expected: [";
            for (const auto& lit : expectedResult) {
                result << lit << ' ';
            }
            result << "]";
            return result.str();
        }

        static void testAll() {
            std::vector<NegatedLiteralsTestCase> testCases = {
                {{1, 2, 3}, {2, 3}, {}},
                {{-3, -2, -1}, {-2}, {}},
                {{-4, -2, 3, 4}, {1, 2}, {-2}},
                {{}, {1, 2}, {}},
                {{1, 2, 3}, {}, {}},
                {{-3, -2, -1}, {}, {}},
                {{}, {}, {}},
                {{-16, -14, -12, -10, -8, -6, -4, -2, 3, 4, 7, 8, 11, 12, 15, 16}, {-16, -15, -14, -13, -8, -7, -6, -5, 9, 10, 11, 12, 13, 14, 15, 16}, {-16, -14, -12, -10, 7, 8, 15, 16}}
            };
    
            for (const auto& testCase : testCases) {
                testCase.run();
            }
        }
    }; // end struct NegatedLiteralsTestCase

    struct ResolveTestCase {
        oct_22::AveIntVec c1;
        oct_22::AveIntVec c2;
        int pivot;
        oct_22::AveIntVec expectedResult;

        void run() const {
            auto result = oct_22::ApproxVarElim::resolve(c1, c2, pivot);
            if (result->literals != expectedResult) {
                throw std::runtime_error("Resolve test failed");
            }
        }

        static void testAll() {
            std::vector<ResolveTestCase> testCases{
                {{}, {}, 1, {}},
                {{-1}, {}, 1, {}},
                {{-2}, {}, 1, {-2}},
                {{1}, {}, 1, {}},
                {{2}, {}, 1, {2}},
                {{}, {-1}, 1, {}},
                {{}, {-2}, 1, {-2}},
                {{}, {1}, 1, {}},
                {{}, {2}, 1, {2}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 1, {-9, -8, -7, -6, -3, 2, 4, 5, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 2, {-9, -8, -7, -6, -3, 4, 5, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 3, {-9, -8, -7, -6, 2, 4, 5, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 4, {-9, -8, -7, -6, -3, 2, 5, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 5, {-9, -8, -7, -6, -3, 2, 4, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 6, {-9, -8, -7, -3, 2, 4, 5, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 7, {-9, -8, -6, -3, 2, 4, 5, 6, 8}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 8, {-9, -7, -6, -3, 2, 4, 5, 6}},
                {{-9, -6, -3, 2, 5, 8}, {-9, -8, -7, 4, 5, 6}, 9, {-8, -7, -6, -3, 2, 4, 5, 6, 8}}
            };

            for (const auto& testCase : testCases) {
                testCase.run();
            }
        }
    }; // end struct ResolveTestCase

    struct ApproxVarElimTestCase {
        DdManager* manager;
        std::istream &qdimacs_stream;
        std::istream &expected_stream;
        bool should_be_exact;
        size_t search_depth;
        bool recompute_exact_result;

        void run() {
            auto qdimacs = dd::Qdimacs::parseQdimacs(qdimacs_stream);
            auto ave = oct_22::ApproxVarElim::parseQdimacs(*qdimacs);
            ave->approximatelyEliminateAllVariables(search_depth);
            auto const& clauses = ave->getResultClauses();
            // std::cout << "ApproxVarElim: " << clauses.size() << " clauses after elimination." << std::endl;

            dd::BddWrapper over_approx_result = dd::BddWrapper(bdd_one(manager), manager);
            for (auto const& clause: clauses)
            {
                auto bdd_clause = over_approx_result.zero();
                for (auto const& var: clause->literals)
                {
                    // std::cout << var << ' ';
                    dd::BddWrapper bdd_var(bdd_new_var_with_index(manager, (var > 0 ? var : -var)), manager);
                    if (var < 0) bdd_var = -bdd_var;
                    bdd_clause = bdd_clause + bdd_var;
                }
                // std::cout << std::endl;
                over_approx_result = over_approx_result * bdd_clause;
            }


            dd::BddWrapper expected_result = dd::BddWrapper(bdd_one(manager), manager);
            if (recompute_exact_result)
            {
                dd::BddWrapper conjunction = expected_result.one();
                for (auto const& clause: qdimacs->clauses)
                {
                    auto bdd_clause = expected_result.zero();
                    for (auto const& var: clause)
                    {
                        dd::BddWrapper bdd_var(bdd_new_var_with_index(manager, (var > 0 ? var : -var)), manager);
                        if (var < 0) bdd_var = -bdd_var;
                        bdd_clause = bdd_clause + bdd_var;
                    }
                    conjunction = conjunction * bdd_clause;
                }
                auto quantififedVariables = expected_result.one();
                for (auto const& qvar: qdimacs->quantifiers.back().variables)
                {
                    dd::BddWrapper bdd_qvar(bdd_new_var_with_index(manager, qvar), manager);
                    quantififedVariables = quantififedVariables * bdd_qvar;
                }
                expected_result = conjunction.existentialQuantification(quantififedVariables);
            }
            else
            {
                auto expected_qdimacs = dd::Qdimacs::parseQdimacs(expected_stream);
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
            }

            if (should_be_exact)
            {
                if (over_approx_result != expected_result)
                {
                    std::cout << "computed result: " << std::endl;
                    bdd_print_minterms(manager, over_approx_result.getUncountedBdd());
                    std::cout << "expected result: " << std::endl;
                    bdd_print_minterms(manager, expected_result.getUncountedBdd());
                    throw std::runtime_error("ApproxVarElim did not produce exact result.");
                }
            }
            else
            {
                if (!(expected_result * -over_approx_result).isZero())
                {
                    std::cout << "computed result: " << std::endl;
                    bdd_print_minterms(manager, over_approx_result.getUncountedBdd());
                    std::cout << "expected result: " << std::endl;
                    bdd_print_minterms(manager, expected_result.getUncountedBdd());
                    throw std::runtime_error("ApproxVarElim did not produce overapprox result.");
                }
            }
        }
    }; // end struct ApproxVarElimTestCase



    void testSmallCase1(DdManager* manager)
    {
        std::string problem_qdimacs = 
            "p cnf 5 3\n"
            "a 1 2 3 0\n"
            "e 4 5 0\n"
            "-1 3 4 0\n"
            "-4 2 0\n"
            ;
        std::stringstream qdimacs_stream(problem_qdimacs);

        std::string expected_string = 
            "p cnf 5 1\n"
            "-1 3 2 0\n";
        std::stringstream expected_stream(expected_string);

        ApproxVarElimTestCase tc {manager, qdimacs_stream, expected_stream, true, 3, false};
        tc.run();
    }

    void testSmallCase2(DdManager* manager)
    {
        std::string problem_qdimacs = 
            "p cnf 11 6\n"
            "a 6 7 8 9 10 11 0\n"
            "e 1 2 3 4 5 0\n"
            "1 6 0\n"
            "-1 2 7 0\n"
            "-2 3 8 0\n"
            "-3 4 9 0\n"
            "-4 5 10 0\n"
            "-5 11 0\n"
            ;
        std::stringstream qdimacs_stream(problem_qdimacs);

        std::string expected_string = 
            "p cnf 6 1\n"
            "6 7 8 9 10 11 0\n";
        std::stringstream expected_stream(expected_string);

        ApproxVarElimTestCase tc {manager, qdimacs_stream, expected_stream, true, 7, false};
        tc.run();
    }

    void testSmallCase3(DdManager* manager)
    {
        std::string problem_qdimacs = 
            "p cnf 11 6\n"
            "a 6 7 8 9 10 11 0\n"
            "e 1 2 3 4 5 0\n"
            "1 6 0\n"
            "-1 2 7 0\n"
            "-2 3 8 0\n"
            "-3 4 9 0\n"
            "-4 5 10 0\n"
            "-5 11 0\n"
            ;
        std::stringstream qdimacs_stream(problem_qdimacs);
        std::stringstream expected_stream;

        ApproxVarElimTestCase tc {manager, qdimacs_stream, expected_stream, true, 7, true};
        tc.run();
    }

    void testSmallCase4(DdManager* manager)
    {
        std::string problem_qdimacs = 
            "p cnf 12 6\n"
            "a 6 7 8 9 10 11 12 0\n"
            "e 1 2 3 4 5 0\n"
            "1 6 0\n"
            "-1 2 7 0\n"
            "-2 3 8 0\n"
            "-3 4 9 0\n"
            "-4 5 10 0\n"
            "-5 1 11 0\n"
            "-1 12 0\n"
            ;
        std::stringstream qdimacs_stream(problem_qdimacs);
        std::stringstream expected_stream;

        ApproxVarElimTestCase tc {manager, qdimacs_stream, expected_stream, true, 9, true};
        tc.run();
    }


    void testAdder(DdManager* manager)
    {
        std::ifstream adder_fin("test/data/adder.qdimacs");
        std::ifstream expected_fin("test/expected_outputs/adder.cnf");

        ApproxVarElimTestCase tc {manager, adder_fin, expected_fin, false, 3, false};
        tc.run();

    } // end testAdder

    void testFactorization8(DdManager* manager)
    {
        std::ifstream factorization8_fin("test/data/Factorization_factorization8_factor_graph_input.qdimacs");
        std::ifstream expected_fin("test/expected_outputs/Factorization_factorization8.output.cnf");

        ApproxVarElimTestCase tc {manager, factorization8_fin, expected_fin, false, 4, false};
        tc.run();

    } // end testFactorization8
} // end anonymous namespace


void testApproxVarElim(DdManager * manager)
{
    IntersectionTestCase::testAll();
    NegatedLiteralsTestCase::testAll();
    ResolveTestCase::testAll();

    testSmallCase1(manager);
    testSmallCase2(manager);
    // std::cout << "Running testSmallCase3..." << std::endl;
    testSmallCase3(manager);
    // std::cout << "Running testSmallCase4..." << std::endl;
    testSmallCase4(manager);
    // std::cout << "Running adder test..." << std::endl;
    testAdder(manager);
    // std::cout << "Runnning factorization8 test..." << std::endl;
    // testFactorization8(manager);
    // std::cout << "All ApproxVarElim tests passed!" << std::endl;
}