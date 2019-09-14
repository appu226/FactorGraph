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
    m_vars(vars),
    m_manager(manager)
  {
    m_verilog_scanner.switch_streams(is, NULL);
  }

  void VerilogToBdd::parse() {
    m_verilog_parser.parse();
  }

  void VerilogToBdd::addBddPtr(const std::string & name, bdd_ptr func)
  {
    m_vars->addBddPtr(name, func);
  }

  bdd_ptr VerilogToBdd::getBddPtr(const std::string & name) const
  {
    return m_vars->getBddPtr(name);
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
