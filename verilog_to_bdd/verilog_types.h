/*

Copyright 2019 Parakram Majumdar

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

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace verilog_to_bdd {

  /** classes
   * A set of types to represent a parsed module file.
   * Note that this is NOT intended to be a full parser for the verilog format.
   * Instead it is a very limit sub-set,
   *   designed purely to be able to parse
   *   a very specific set of files that I care about.
   */


  struct Module;
  struct Expression;
  typedef std::shared_ptr<Module> ModulePtr;
  typedef std::shared_ptr<Expression> ExpressionPtr;



  struct Module {
    typedef std::shared_ptr<std::vector<std::string> > StringVecPtr;
    typedef std::map<std::string, ExpressionPtr> AssignmentMap;
    typedef std::shared_ptr<AssignmentMap> AssignmentMapPtr;
    std::string name;
    StringVecPtr interface;
    StringVecPtr inputs;
    StringVecPtr outputs;
    StringVecPtr wires;
    AssignmentMapPtr assignments;
  };




  struct Expression {
    enum Type {
      NegType,
      AndType,
      OrType,
      WireType
    };
    virtual Type getExpressionType() const = 0;
    virtual ~Expression() {}
  };




  struct NegExpression : Expression {
    ExpressionPtr underlying;
    Expression::Type getExpressionType() const override { return Expression::NegType; }
  };



  struct AndExpression : Expression {
    ExpressionPtr lhs;
    ExpressionPtr rhs;
    Expression::Type getExpressionType() const override { return Expression::AndType; }
  };


  struct OrExpression : Expression {
    ExpressionPtr lhs;
    ExpressionPtr rhs;
    Expression::Type getExpressionType() const override { return Expression::OrType; }
  };


  struct WireExpression : Expression {
    std::string wireName;
    Expression::Type getExpressionType() const override { return Expression::WireType; }
  };



  /** struct ExpressionVisitor
   * A Visitor designed to be able to traverse the Expression hierarchy.
   * See: https://en.wikipedia.org/wiki/Visitor_pattern
   */
  template<typename ReturnType>
  struct ExpressionVisitor {
    ReturnType visitExpression(const Expression & e) {
      switch (e.getExpressionType())
      {
        case Expression::NegType:
          return visitNegExpression(static_cast<const NegExpression &>(e));
        case Expression::AndType:
          return visitAndExpression(static_cast<const AndExpression &>(e));
        case Expression::OrType:
          return visitOrExpression(static_cast<const OrExpression &>(e));
        case Expression::WireType:
          return visitWireExpression(static_cast<const WireExpression &>(e));
      }
    }
    virtual ReturnType visitNegExpression(const NegExpression &) = 0;
    virtual ReturnType visitAndExpression(const AndExpression &) = 0;
    virtual ReturnType visitOrExpression(const OrExpression &) = 0;
    virtual ReturnType visitWireExpression(const WireExpression &) = 0;
  };



} // end namespace verilog_to_bdd
