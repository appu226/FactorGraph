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

#include "verilog_to_bdd.h"
#include "location.hh"
#include "position.hh"

#include <stdexcept>

namespace {
  struct ExpressionToBddConverter: verilog_to_bdd::ExpressionVisitor<bdd_ptr>
  {
    public:
      typedef verilog_to_bdd::Module::AssignmentMapPtr AssignmentMapPtr;
      ExpressionToBddConverter(const verilog_to_bdd::BddVarMapPtr & vars,
                               const AssignmentMapPtr & assignments,
                               DdManager * manager) :
        m_vars(vars),
        m_assignments(assignments),
        m_manager(manager),
        m_creatingVars()
      { }
      
      virtual bdd_ptr visitNegExpression(const verilog_to_bdd::NegExpression & nexp) override
      {
        auto underlying = visitExpression(*nexp.underlying);
        auto result = bdd_not(m_manager, underlying);
        bdd_free(m_manager, underlying);
        return result;
      }
      virtual bdd_ptr visitAndExpression(const verilog_to_bdd::AndExpression & aexp) override
      {
        auto lhs = visitExpression(*aexp.lhs);
        auto rhs = visitExpression(*aexp.rhs);
        auto result = bdd_and(m_manager, lhs, rhs);
        bdd_free(m_manager, lhs);
        bdd_free(m_manager, rhs);
        return result;
      }
      virtual bdd_ptr visitOrExpression(const verilog_to_bdd::OrExpression & oexp) override
      {
        auto lhs = visitExpression(*oexp.lhs);
        auto rhs = visitExpression(*oexp.rhs);
        auto result = bdd_or(m_manager, lhs, rhs);
        bdd_free(m_manager, lhs);
        bdd_free(m_manager, rhs);
        return result;
      }
      virtual bdd_ptr visitWireExpression(const verilog_to_bdd::WireExpression & wexp) override
      {
        const std::string & wn = wexp.wireName;
        if (m_creatingVars.count(wexp.wireName) != 0)
        {
          std::stringstream ss;
          ss << "Found cyclic assignments { ";
          for (auto vn : m_creatingVars)
            ss << vn << ", ";
          ss << " } while parsing module";
          throw std::runtime_error(ss.str());
        }
        if (m_vars->containsBddPtr(wn))
          return m_vars->getBddPtr(wn);
        
        auto ai = m_assignments->find(wn);
        if (ai == m_assignments->end())
          throw std::runtime_error(std::string("Wire ") + wexp.wireName
                                   + " is neither an input nor an assignment");
        return convertVar(wn, ai->second);
      }
      bdd_ptr convertVar(const std::string & v, const verilog_to_bdd::ExpressionPtr & e)
      {
        if (m_vars->containsBddPtr(v))
          return m_vars->getBddPtr(v);
        m_creatingVars.insert(v);
        auto result = visitExpression(*e);
        m_vars->addBddPtr(v, result);
        m_creatingVars.erase(v);
        return bdd_dup(result);
      }

    private:
      verilog_to_bdd::BddVarMapPtr m_vars;
      AssignmentMapPtr m_assignments;
      DdManager * m_manager;
      std::set<std::string> m_creatingVars;
  };
} // end anonymous namepsace



namespace verilog_to_bdd {


  //------------------------------------------------------------
  // BddVarMap definitions
  BddVarMap::BddVarMap(DdManager * manager)
    : m_manager(manager)
  { }

  BddVarMap::~BddVarMap()
  {
    for (auto di = m_data.begin(); di != m_data.end(); ++di)
      bdd_free(m_manager, di->second);
  }

  bdd_ptr BddVarMap::getBddPtr(const std::string & varName) const
  {
    auto di = m_data.find(varName);
    if (di == m_data.end()) throw std::runtime_error("Could not find variable " + varName);
    return bdd_dup(di->second);
  }

  void BddVarMap::addBddPtr(const std::string & varName, bdd_ptr varBdd)
  {
    if (m_data.count(varName)) throw std::runtime_error("Variable " + varName + " already present");
    m_data[varName] = varBdd;
  }

  bool BddVarMap::containsBddPtr(const std::string & varName) const
  {
    return m_data.count(varName);
  }




  // -----------------------------------------------------------
  // VerilogToBdd definitions
  void
    VerilogToBdd::parse(std::istream *is, 
                        const std::string & filename,
                        const BddVarMapPtr & vars,
                        DdManager * manager)
    {
      VerilogToBdd ipt(is, filename, vars, manager);
      return ipt.parse();
    }

  VerilogToBdd::VerilogToBdd(std::istream * is, 
                             const std::string & filename,
                             const BddVarMapPtr & vars,
                             DdManager * manager) :
    m_verilog_scanner(*this),
    m_verilog_parser(m_verilog_scanner, *this),
    m_filename(filename),
    m_location(&m_filename, 1, 1),
    m_module(),
    m_vars(vars),
    m_manager(manager)
  {
    m_verilog_scanner.switch_streams(is, NULL);
  }

  void VerilogToBdd::parse() {
    m_verilog_parser.parse();
    if (!m_module)
      throw std::runtime_error("Could not find a module in " + m_filename);
    ExpressionToBddConverter expToBdd(m_vars, m_module->assignments, m_manager);
    for (auto assignment : *(m_module->assignments))
      bdd_free(m_manager, expToBdd.convertVar(assignment.first, assignment.second));
  }

  void VerilogToBdd::setModule(const ModulePtr & module)
  {
    m_module = module;
  }

  void VerilogToBdd::columns(unsigned int offset)
  {
    m_location.columns(offset);
  }

  void VerilogToBdd::lines(unsigned int offset)
  {
    m_location.lines(offset);
  }

  void VerilogToBdd::step()
  {
    m_location.step();
  }

  void VerilogToBdd::resetLocation()
  {
    m_location.initialize(&m_filename, 1, 1);
  }

  location VerilogToBdd::getLocation() const
  {
    return m_location;
  }

} // end namespace verilog_to_bdd
