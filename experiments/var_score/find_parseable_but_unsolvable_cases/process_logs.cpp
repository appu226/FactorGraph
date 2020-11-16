#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <optional>
#include <vector>

#define INFO(x)  // std::cout << "[INFO] " << x << std::endl;
#define DEBUG(x) // std::cout << "[DEBUG] " << x << std::endl;



namespace {

  struct ResultRow
  {
    std::string             filename;
    std::optional<double> parsingTime;
    std::optional<int>    subProblems;
    std::optional<double> solvingTime;
    std::optional<int>    maxBddSize;
  };

  template<typename TValue>
    void printOptional(const std::optional<TValue> & optVal)
    {
      if (optVal.has_value())
        std::cout << optVal.value() << "\t";
      else
        std::cout << "--\t";
    }

  void printResults(const std::vector<ResultRow> & resultRows)
  {
    std::cout << "FileName\tParsingTime\tSubProblems\tSolvingTime\tMaxBddSize\n";
    for (const auto & row: resultRows)
    {
      std::cout << row.filename << "\t";
      printOptional(row.parsingTime);
      printOptional(row.subProblems);
      printOptional(row.solvingTime);
      printOptional(row.maxBddSize);
      std::cout << "\n";
    }
    std::cout << std::endl;
  }

  ResultRow process(const std::string & filename)
  {
    ResultRow result;
    {
      int lastSlashPos = filename.find_last_of("/\\");
      if (lastSlashPos == std::string::npos)
        result.filename = filename;
      else
        result.filename = filename.substr(lastSlashPos + 1);
    }

    // var_score/var_score --blif ../../data_sets/all_blifs/b01.blif --verbosity INFO
    // [INFO] Parsed 1 sub problems in 0.006693 sec
    // [INFO] Computed sub result
    // [INFO] Finished in 0.000178 sec with maxBddSize 44

    // read all lines in the log
    std::ifstream fin(filename);
    std::string line;
    while(getline(fin, line))
    {
      // case 1: parsing time
      if (line.find("[INFO] Parsed ") == 0)
      {
        int subProblems;
        float parsingTime;
        sscanf(line.c_str(),
               "[INFO] Parsed %d sub problems in %f sec",
               &subProblems,
               &parsingTime);
        result.subProblems = subProblems;
        result.parsingTime = parsingTime;
      }

      // case 2: solution time and max bdd size
      else if (line.find("[INFO] Finished in ") == 0)
      {
        float solvingTime;
        int maxBddSize;
        sscanf(line.c_str(),
               "[INFO] Finished in %f sec with maxBddSize %d",
               &solvingTime,
               &maxBddSize);
        result.solvingTime = solvingTime;
        result.maxBddSize = maxBddSize;
      }
    }
    return result;
  }



} // end anonymous namespace




int main(int argc, char const * const * const argv)
{
  std::vector<ResultRow> resultRows;
  for (int iarg = 1; iarg < argc; ++iarg)
  {
    std::string filename = argv[iarg];
    resultRows.push_back(process(filename));
  }
  printResults(resultRows);
  return 0;
}
