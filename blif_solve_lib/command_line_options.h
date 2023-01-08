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

#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <iostream>

namespace blif_solve {

  class ICommandLineOption {
    public:
      virtual ~ICommandLineOption() {}
      virtual std::string getName() const = 0;
      virtual std::string getHelp() const = 0;
      virtual bool tryParse(int & numRemainingArgs, char const * const * & currentArg) = 0;
  };




  template<typename TValue>
  class CommandLineOptionValue : public ICommandLineOption {
    public:
      static
        std::shared_ptr<CommandLineOptionValue<TValue> >
        create(const std::string & name,
               const std::string & help,
               const TValue & defaultValue)
        {
          return std::make_shared<CommandLineOptionValue<TValue> >(name, help, defaultValue);
        }

      std::string getName() const override
      {
        return m_name;
      }

      std::string getHelp() const override
      {
        return m_help;
      }

      TValue getValue() const
      {
        return m_value;
      }

      bool tryParse(int & numRemainingArgs, char const * const * & currentArg) override
      {
        std::string argStr(*currentArg);
        if (argStr == m_name)
        {
          --numRemainingArgs;
          ++currentArg;
          if (numRemainingArgs > 0)
          {
            std::istringstream iss;
            iss.str(*currentArg);
            iss >> m_value;
          } else
          {
            throw std::runtime_error("Could not find a value after argument '" + m_name + "'");
          }
          return true;
        }
        else
        {
          return false;
        }
      }

      CommandLineOptionValue(const std::string & name,
                             const std::string & help,
                             const TValue & defaultValue) :
        m_name(name),
        m_help(help),
        m_value(defaultValue)
      { }

    private:
      std::string m_name;
      std::string m_help;
      TValue m_value;
  };


  void printHelp(const std::vector<std::shared_ptr<ICommandLineOption> > & commandLineOptions)
  {
    std::cout << "\n\n";
    for (int i = 0; i < commandLineOptions.size(); ++i)
      std::cout << "\t" << commandLineOptions[i]->getName() << "\t" << commandLineOptions[i]->getHelp() << "\n";
    std::cout << std::endl;
  }

  void parseCommandLineOptions(int argc, 
                               char const * const * argv, 
                               const std::vector<std::shared_ptr<ICommandLineOption> > & commandLineOptions)
  {
    for (int i = 0; i < argc; ++i)
    {
      if (std::string(argv[i]) == "--help")
      {
        printHelp(commandLineOptions);
        exit(0);
      }
    }
    while (argc > 0)
    {
      bool parsed = false;
      for (int i = 0; !parsed && i < commandLineOptions.size(); ++i)
        parsed = commandLineOptions[i]->tryParse(argc, argv);
      if (!parsed)
      {
        std::cout << "Could not recognize argument '" << *argv;
        printHelp(commandLineOptions);
        exit(-1);
      }
      --argc;
      ++argv;
    }
  }


} // end namespace blif_solve
