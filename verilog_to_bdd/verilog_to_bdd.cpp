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
#include "verilog_types.h"
#include "location.hh"
#include "position.hh"

#include <sstream>

namespace verilog_to_bdd {

  std::shared_ptr<Module>
    VerilogToBdd::parse(std::istream *is, const std::string & filename)
    {
      VerilogToBdd ipt(is, filename);
      return ipt.parse();
    }

  VerilogToBdd::VerilogToBdd(std::istream * is, const std::string & filename) :
    m_verilog_scanner(*this),
    m_verilog_parser(m_verilog_scanner, *this),
    m_module(),
    m_filename(filename),
    m_location(&m_filename, 1, 1)
  {
    m_verilog_scanner.switch_streams(is, NULL);
  }

  std::shared_ptr<Module> VerilogToBdd::parse() {
    m_verilog_parser.parse();
    return m_module;
  }

  void VerilogToBdd::setModule(const std::shared_ptr<Module> & module)
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
