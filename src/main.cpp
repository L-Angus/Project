#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "CFGFileManager.hpp"
int main() {
  std::cout << "___________ Convertor ___________" << std::endl;
  try {
    CFGFileManager &FileManager = CFGFileManager::GetInstance();
    FileManager.SetRootPath("./");
    FileManager.LoadCFGFile("TX/DUT", "DUT.cfg");
    FileManager.PrintTree();
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }

  return 0;
}
