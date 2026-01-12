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
#include <oct_22/ave2.h>


namespace {

  void testAve2ClauseIntersect() {
    using Ave2Clause = oct_22::Ave2Clause;
    auto clause1 = std::make_shared<Ave2Clause>(Ave2Clause({1, -2, 3, 4}));
    auto clause2 = std::make_shared<Ave2Clause>(Ave2Clause({-2, 3, 5}));
    auto expectedIntersection = std::make_shared<Ave2Clause>(Ave2Clause({-2, 3}));
    auto intersection = clause1->intersect(*clause2);
    if (!intersection || !(*intersection == *expectedIntersection)) {
      throw std::runtime_error("Ave2Clause intersection test failed");
    }
  }

  void testFilterOutClausesWithNoVarsToEliminate() {
    using Ave2Clause = oct_22::Ave2Clause;
    
    // Create clauses: some with variables to eliminate, some without
    auto clause1 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2, 3}));       // has vars to eliminate
    auto clause2 = std::make_shared<Ave2Clause>(Ave2Clause({4, 5}));          // no vars to eliminate
    auto clause3 = std::make_shared<Ave2Clause>(Ave2Clause({2, 6, 7}));       // has vars to eliminate
    auto clause4 = std::make_shared<Ave2Clause>(Ave2Clause({8, 9, 10}));      // no vars to eliminate
    auto clause5 = std::make_shared<Ave2Clause>(Ave2Clause({-1, -2, 11}));    // has vars to eliminate (negative)
    auto clause6 = std::make_shared<Ave2Clause>(Ave2Clause({-4, -5, 12}));    // no vars to eliminate (negative)
    auto clause7 = std::make_shared<Ave2Clause>(Ave2Clause({-3, 13, 14}));    // has vars to eliminate (negative)
    
    auto clauseVec = std::make_shared<std::vector<oct_22::Ave2ClauseCPtr> >();
    clauseVec->push_back(clause1);
    clauseVec->push_back(clause2);
    clauseVec->push_back(clause3);
    clauseVec->push_back(clause4);
    clauseVec->push_back(clause5);
    clauseVec->push_back(clause6);
    clauseVec->push_back(clause7);
    
    // Variables to eliminate: {1, 2, 3}
    Ave2Clause literalsToEliminate ({-3, -2, -1, 1, 2, 3});
    
    // Call filterOutClausesWithNoVarsToEliminate
    auto result = oct_22::Ave2::filterOutClausesWithNoVarsToEliminate(clauseVec, literalsToEliminate);

    // Result should contain clauses with no variables to eliminate: clause2, clause4, clause6
    if (result->size() != 3) {
      throw std::runtime_error("filterOutClausesWithNoVarsToEliminate: expected 3 clauses in result, got " + std::to_string(result->size()));
    }
    
    // Check that clause2, clause4, and clause6 are in result
    bool found_clause2 = false, found_clause4 = false, found_clause6 = false;
    for (const auto& c : *result) {
      if (*c == *clause2) found_clause2 = true;
      if (*c == *clause4) found_clause4 = true;
      if (*c == *clause6) found_clause6 = true;
    }
    
    if (!found_clause2 || !found_clause4 || !found_clause6) {
      throw std::runtime_error("filterOutClausesWithNoVarsToEliminate: expected clauses not found in result");
    }
    
    // Remaining clauses should be clause1, clause3, clause5, and clause7
    if (clauseVec->size() != 4) {
      throw std::runtime_error("filterOutClausesWithNoVarsToEliminate: expected 4 clauses remaining, got " + std::to_string(clauseVec->size()));
    }
    
    // Check that clause1, clause3, clause5, and clause7 are in clauseVec
    bool found_clause1 = false, found_clause3 = false, found_clause5 = false, found_clause7 = false;
    for (const auto& c : *clauseVec) {
      if (*c == *clause1) found_clause1 = true;
      if (*c == *clause3) found_clause3 = true;
      if (*c == *clause5) found_clause5 = true;
      if (*c == *clause7) found_clause7 = true;
    }
    
    if (!found_clause1 || !found_clause3 || !found_clause5 || !found_clause7) {
      throw std::runtime_error("filterOutClausesWithNoVarsToEliminate: expected clauses not found in remaining clauseVec");
    }
  }

  void testAve2ClauseResolveOnVar() {
    using Ave2Clause = oct_22::Ave2Clause;
    
    // Test case 1: Basic resolution
    // clause1: {1, 2, 3}, clause2: {-1, 4, 5}
    // Resolve on var 1: should get {2, 3, 4, 5}
    auto clause1 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2, 3}));
    auto clause2 = std::make_shared<Ave2Clause>(Ave2Clause({-1, 4, 5}));
    auto expected1 = std::make_shared<Ave2Clause>(Ave2Clause({2, 3, 4, 5}));
    auto result1 = clause1->resolveOnVar(1, *clause2);
    if (!result1.has_value() || !(*result1.value() == *expected1)) {
      throw std::runtime_error("resolveOnVar test 1 failed");
    }
    
    // Test case 2: Resolution with duplicate literals
    // clause1: {1, 2}, clause2: {-1, 2}
    // Resolve on var 1: should get {2} (duplicate removed)
    auto clause3 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2}));
    auto clause4 = std::make_shared<Ave2Clause>(Ave2Clause({-1, 2}));
    auto expected2 = std::make_shared<Ave2Clause>(Ave2Clause({2}));
    auto result2 = clause3->resolveOnVar(1, *clause4);
    if (!result2.has_value() || !(*result2.value() == *expected2)) {
      throw std::runtime_error("resolveOnVar test 2 failed");
    }
    
    // Test case 3: No resolution possible (var not in clauses)
    // clause1: {1, 2}, clause2: {3, 4}
    // Resolve on var 5: should fail (nullopt)
    auto clause5 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2}));
    auto clause6 = std::make_shared<Ave2Clause>(Ave2Clause({3, 4}));
    auto result3 = clause5->resolveOnVar(5, *clause6);
    if (result3.has_value()) {
      throw std::runtime_error("resolveOnVar test 3 failed: expected nullopt");
    }
    
    // Test case 4: No resolution possible (var only in one clause)
    // clause1: {1, 2}, clause2: {3, 4}
    // Resolve on var 1: should fail (tautology would result)
    auto clause7 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2}));
    auto clause8 = std::make_shared<Ave2Clause>(Ave2Clause({3, 4}));
    auto result4 = clause7->resolveOnVar(1, *clause8);
    if (result4.has_value()) {
      throw std::runtime_error("resolveOnVar test 4 failed: expected nullopt");
    }
    
    // Test case 5: Resolution with conflicting literals
    // clause1: {-1, -2, 3}, clause2: {1, -3, 4}
    // Resolve on var 1: should fail (conflict on var 3)
    auto clause9 = std::make_shared<Ave2Clause>(Ave2Clause({-1, -2, 3}));
    auto clause10 = std::make_shared<Ave2Clause>(Ave2Clause({1, -3, 4}));
    auto result5 = clause9->resolveOnVar(1, *clause10);
    if (result5.has_value()) {
      throw std::runtime_error("resolveOnVar test 5 failed: expected nullopt");
    }

    // Test case 6: Resolution with negative literals
    // clause1: {-1, -2}, clause2: {1, 3}
    // Resolve on var 1: should get {-2, 3}
    auto clause11 = std::make_shared<Ave2Clause>(Ave2Clause({-1, -2}));
    auto clause12 = std::make_shared<Ave2Clause>(Ave2Clause({1, 3}));
    auto expected6 = std::make_shared<Ave2Clause>(Ave2Clause({-2, 3}));
    auto result6 = clause11->resolveOnVar(1, *clause12);
    if (!result6.has_value() || !(*result6.value() == *expected6)) {
      throw std::runtime_error("resolveOnVar test 6 failed");
    }


    // Test case 7: Failed resolution if both clauses contain the same sign of the pivot
    // clause1: {1, 2}, clause2: {1, 3}
    // Resolve on var 1: should fail (both clauses contain positive literal)
    auto clause13 = std::make_shared<Ave2Clause>(Ave2Clause({1, 2}));
    auto clause14 = std::make_shared<Ave2Clause>(Ave2Clause({1, 3}));
    auto result7 = clause13->resolveOnVar(1, *clause14);
    if (result7.has_value()) {
      throw std::runtime_error("resolveOnVar test 7 failed: expected nullopt");
    }

  }

} // end anonymous namespace


void testAve2(DdManager * manager) {
    testAve2ClauseIntersect();
    testFilterOutClausesWithNoVarsToEliminate();
    testAve2ClauseResolveOnVar();
}

