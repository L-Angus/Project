#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "CFGFileManager.hpp"
int main() {
  std::cout << "___________ Convertor ___________" << std::endl;
  auto &manager = CFGFileManager::GetInstance();
  auto creator = manager.GetParserCreators();
  std::cout << creator.size() << std::endl;

  return 0;
}
