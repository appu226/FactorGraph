#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <map>
#include <set>
#include <optional>

#define INFO(x)  // std::cout << "[INFO] " << x << std::endl;
#define DEBUG(x) // std::cout << "[DEBUG] " << x << std::endl;



namespace {

  void printResults(const std::map<int, std::map<int, double> > & results, std::set<int> & xaxis)
  {

    // print x axis labels
    for (int x: xaxis)
      std::cout << "\t" << x;
    std::cout << "\n";


    for (auto keyAndRow: results)
    {
      int y = keyAndRow.first;
      std::cout << y << "\t";
      const auto & row = keyAndRow.second;
      for (auto x: xaxis)
        if (row.count(x) > 0)
          std::cout << row.find(x)->second << "\t";
        else
          std::cout << "--" << "\t";
      std::cout << "\n";
    }
  }

  void process(const std::string & filename)
  {
    
    // variables to hold parsed information
    int lss;
    int numConv = 0;
    std::optional<double> numSolutions;
    std::optional<double> timeTaken;

    // data structure for creating the table
    std::map<
      int,               // largest support set
      std::map<
        int,             // number of convergences
        double           // number of satisfying states / time taken
        > > numSolutionsTable, timeTable;
    std::set<int> numConvergenceSet;


    // read all lines in the log
    std::ifstream fin(filename);
    std::string line;
    while(getline(fin, line))
    {
      // case 1: starting a new blif_solve routine
      if (line.find("blif_solve/blif_solve") == 0)
      {

        // save details of previous run
          if (numSolutions)
          {
            numSolutionsTable [lss][numConv] = *numSolutions;
            numConvergenceSet.insert(numConv);
          }
          if (timeTaken)
            timeTable    [lss][numConv] = *timeTaken;

        // parse details of next run
        sscanf(line.c_str(),
               "blif_solve/blif_solve --under_approximating_method Skip --largest_support_set %d",
               &lss);
        numSolutions.reset();
        timeTaken.reset();
        numConv = 0;
      }

      // case 2: time taken
      else if (line.find("[INFO] Finished over approximating method FactorGraphApprox in") == 0)
      {
        double ttBuf;
        sscanf(line.c_str(),
               "[INFO] Finished over approximating method FactorGraphApprox in %lf sec",
               &ttBuf);
        timeTaken.emplace(ttBuf);
      }

      // case 3: number of solutions
      else if (line.find("# solutions") == 0)
      {
        double numSolBuf;
        fin >> numSolBuf;
        numSolutions.emplace(numSolBuf);
      }

      // case 4: number of convergences
      else if (line.find("[INFO] Ran") == 0 && line.find("FactorGraph convergences") != std::string::npos)
      {
        int numConvForThisPartition;
        sscanf(line.c_str(),
               "[INFO] Ran %d FactorGraph convergences",
               &numConvForThisPartition);
        numConv = std::max(numConv, numConvForThisPartition);
      }
    }
    // finished reading file

    // save the details of the last run, when the file ends
    if (numSolutions) {
      numSolutionsTable[lss][numConv] = *numSolutions;
      numConvergenceSet.insert(numConv);
    }
    if (timeTaken) timeTable[lss][numConv] = *timeTaken;

    // print out nice-ish tables
    std::cout << filename << "\nNumber of solutions (x: numConvergence, y: lss)" << std::endl;
    printResults(numSolutionsTable, numConvergenceSet);
    std::cout << filename << "\nTime taken (x: numConvergence, y: lss)" << std::endl;
    printResults(timeTable, numConvergenceSet);


  }



} // end anonymous namespace




int main(int argc, char const * const * const argv)
{
  for (int iarg = 1; iarg < argc; ++iarg)
  {
    std::string filename = argv[iarg];
    INFO("processing " << filename);
    process(filename);
    INFO("done processing " << filename);
  }
  return 0;
}
