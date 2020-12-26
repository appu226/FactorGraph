/*

Copyright 2020 Parakram Majumdar

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

#include "dotty.h"

#include <sstream>
#include <algorithm>

namespace {

  std::string bddToString(bdd_ptr bdd)
  {
    std::stringstream ss;
    ss << bdd;
    return '"' + ss.str() + '"';
  }

} // end anonymous namespace




namespace dd
{

  void Dotty::addFactor(const BddWrapper & factor, bool addEdges)
  {
    m_factors[factor].label = bddToString(factor.getUncountedBdd());
    if (addEdges)
    {
      auto fsup = factor.support();
      for (const auto & kv : m_variables)
      {
        auto var = kv.first;
        auto commonSup = fsup.cubeIntersection(var);
        if (!bdd_is_one(commonSup.getManager(), commonSup.getUncountedBdd()))
        {
          m_edges[std::make_pair(factor, var)];
        }
      }
    }
  }

  void Dotty::addVariable(const BddWrapper & variable, bool addEdges)
  {
    m_variables[variable].label = bddToString(variable.getUncountedBdd());
    if (addEdges)
    {
      for (const auto & kv : m_factors)
      {
        auto factor = kv.first;
        auto commonSup = variable.cubeIntersection(factor.support());
        if (!bdd_is_one(commonSup.getManager(), commonSup.getUncountedBdd()))
          m_edges[std::make_pair(factor, variable)];
      }
    }
  }

  void Dotty::addEdge(const BddWrapper & factor, const BddWrapper & variable)
  {
    m_edges[std::make_pair(factor, variable)];
  }

  void Dotty::setFactorAttributes(const BddWrapper & factor, const std::string & attributes)
  {
    m_factors[factor].attributes = attributes;
  }

  void Dotty::setFactorLabel(const BddWrapper & factor, const std::string & label)
  {
    m_factors[factor].label = label;
  }

  void Dotty::setVariableAttributes(const BddWrapper & variable, const std::string & attributes)
  {
    m_variables[variable].attributes = attributes;
  }

  void Dotty::setVariableLabel(const BddWrapper & variable, const std::string & label)
  {
    m_variables[variable].label = label;
  }

  void Dotty::setEdgeAttributes(const BddWrapper & factor, const BddWrapper & variable, const std::string & attributes)
  {
    m_edges[std::make_pair(factor, variable)].attributes = attributes;
  }

  void Dotty::setEdgeLabel(const BddWrapper & factor, const BddWrapper & variable, const std::string & label)
  {
    m_edges[std::make_pair(factor, variable)].label = label;
  }

  void Dotty::writeToDottyFile(std::ostream & stream) const
  {
    stream << "graph FactorGraph {\n";
    {
      for (const auto & factorProperties : m_factors)
      {
        const auto & properties = factorProperties.second;
        std::string attributes = "shape=box";
        if (!properties.attributes.empty())
          attributes.append(" ").append(properties.attributes);
        stream << "  " << properties.label << " [" << attributes << "];\n";
      }

      for (const auto & variableProperties : m_variables)
      {
        const auto & properties = variableProperties.second;
        std::string attributes = "shape=ellipse";
        if (!properties.attributes.empty())
          attributes.append(" ").append(properties.attributes);
        stream << "  " << properties.label << " [" << attributes << "];\n";
      }

      for (const auto & edgeProperties : m_edges)
      {
        const auto & factor = edgeProperties.first.first;
        const auto & variable = edgeProperties.first.second;
        const auto & properties = edgeProperties.second;
        std::string attributes;
        if (!properties.label.empty())
          attributes.append("label=").append(properties.label);
        if (!properties.attributes.empty())
          attributes.append(" ").append(properties.attributes);
        const auto & factorLabel = m_factors.at(factor).label;
        const auto & variableLabel = m_variables.at(variable).label;
        stream << "  " << factorLabel << " -- " << variableLabel;
        if (!attributes.empty())
          stream << " [" << attributes << "]";
        stream << ";\n";
      }
    }
    stream << "}\n";
  }





} // end namespace dd


