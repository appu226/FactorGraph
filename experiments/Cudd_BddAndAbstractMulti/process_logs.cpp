#include <iostream>
#include <string>
#include <regex>
#include <iomanip>




/////////////////////////////////////
// macros for info/debug statments //
/////////////////////////////////////
#define INFO(x) //std::cout << "[INFO] " << x << std::endl;
#define DEBUG(x) //std::cout << "[DEBUG] " << x << std::endl;





//////////////////////////////////////////////////
// structure for defining one row in the output //
//////////////////////////////////////////////////
struct RowData {
  std::string testCaseName;
  std::string numFactors;
  std::string numTotalVars;
  std::string numVarsToQuantify;
  std::string numOtherVars;
  std::string numPartitions;
  std::string timeTakenToCreateFactors;
  std::string timeTakenToClip;
  std::string numSolutions;

  RowData():
    testCaseName("--"),
    numFactors("--"),
    numTotalVars("--"),
    numVarsToQuantify("--"),
    numOtherVars("--"),
    numPartitions("--"),
    timeTakenToCreateFactors("--"),
    timeTakenToClip("--"),
    numSolutions("--")
  { }

};




///////////////////////////
// funciton declarations //
///////////////////////////
std::ostream & operator << (std::ostream & out, const RowData & rowData);
void processLine(const std::string & line, RowData & row);
void printHeaders();
int main();






///////////////////////////////////////////////
// regular expressions for parsing the lines //
///////////////////////////////////////////////
std::regex startTestCaseRegex("Running (.*)");
std::regex factorGraphCreationRegex("\\[INFO\\] created (\\d*) factors with (\\d*) vars \\((\\d*) primary inputs \\+ (\\d*) others\\) in ([0-9\\.e\\+]*) sec");
std::regex numPartitionsRegex("\\[INFO\\] processing (\\d*) partitions");
std::regex timeTakenRegex("\\[INFO\\] Finished over approximating method ExactAndAbstractMulti in ([0-9\\.e\\+]*) sec");
std::regex numSolutionsRegex("\\[INFO\\] Over approximating method ExactAndAbstractMulti finished with ([[0-9\\.e\\+]*) solutions.");






//////////
// main //
/////////
int main()
{

  // print the headers of the table
  printHeaders();

  // process each line
  RowData row;
  std::string line;
  while(std::getline(std::cin, line))
    processLine(line, row);

  // print the last row
  if (row.testCaseName != "--")
    std::cout << row;

  // finish
  INFO("SUCCESS");
  return 0;
}





//////////////////////////////////////////////////////
// func to parse each line and fill in the row data //
//////////////////////////////////////////////////////
void processLine(const std::string & line, RowData & row)
{

  std::smatch match;
  // case 1: start of a new test case
  if (std::regex_match(line, match, startTestCaseRegex))
  {
    // print the previous row if it exists
    if (row.testCaseName != "--")
      std::cout << row;

    // reset the row data and add the test case name
    row = RowData();
    row.testCaseName = match[1];
  }

  // case 2: factor graph details
  else if (std::regex_match(line, match, factorGraphCreationRegex))
  {
    row.numFactors = match[1];
    row.numTotalVars = match[2];
    row.numVarsToQuantify = match[3];
    row.numOtherVars = match[4];
    row.timeTakenToCreateFactors = match[5];
  }

  // case 3: number of partitions
  else if (std::regex_match(line, match, numPartitionsRegex))
  {
    row.numPartitions = match[1];
  }

  // case 4: time taken to finish clipping
  else if (std::regex_match(line, match, timeTakenRegex))
  {
    row.timeTakenToClip = match[1];
  }

  // case 5: number of satisfying assignments
  else if (std::regex_match(line, match, numSolutionsRegex))
  {
    row.numSolutions = match[1];
  }

  // default case
  else
    DEBUG("skipping line " << line);
}


////////////////////////////////////
// print the headers of the table //
////////////////////////////////////
void printHeaders()
{
  RowData row;
  row.testCaseName = "Test Case";
  row.numFactors = "Factors";
  row.numTotalVars = "Vars";
  row.numVarsToQuantify = "PI Vars";
  row.numOtherVars = "Other V";
  row.numPartitions = "Partitions";
  row.timeTakenToCreateFactors = "Parse Time";
  row.timeTakenToClip = "Clip Time";
  row.numSolutions = "Solns";
  std::cout << row;
}


/////////////////
// print a row //
/////////////////
std::ostream & operator << (std::ostream & out, const RowData & row)
{
  if (row.numFactors == "0")
    return out;

  out << std::setw(20) << row.testCaseName << "\t"
      << row.numFactors << "\t"
      << row.numTotalVars << "\t"
      << row.numVarsToQuantify << "\t"
      << row.numOtherVars << "\t"
      << std::setw(12) << row.numPartitions << "\t"
      << std::setw(12) << row.timeTakenToCreateFactors << "\t"
      << std::setw(12) << row.timeTakenToClip << "\t"
      << std::setw(12) << row.numSolutions
      << "\n";
  return out;
}
