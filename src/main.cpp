#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "ConfigTable.hpp"
int main() {
  std::cout << "___________ Convertor ___________" << std::endl;
  auto &cfgFileManager = CFGFileManager::GetInstance();
  try {
    cfgFileManager.SetRootPath("../Config");
    cfgFileManager.LoadAllCFGFiles();

    std::string moduleName = "FE";
    std::string fileName = "Fe1.csv";

    auto parser = cfgFileManager.GetParser(moduleName, fileName);
    if (parser) {
      parser->parse(); // 执行解析
      auto data = parser->GetModuleCFGData();
      std::cout << "Parsed Data: " << std::endl;
      for (const auto &row : data) {
        for (const auto &col : row) {
          std::cout << col << " ";
        }
        std::cout << std::endl;
      }
    } else {
      std::cerr << "Failed to get parser for " << fileName << std::endl;
    }

    std::string moduleName1 = "REC";
    std::string fileName1 = "rec.csv";

    auto parser1 = cfgFileManager.GetParser(moduleName1, fileName1);
    if (parser1) {
      parser1->parse(); // 执行解析
      auto data = parser1->GetModuleCFGData();
      std::cout << "Parsed Data: " << std::endl;
      for (const auto &row : data) {
        for (const auto &col : row) {
          std::cout << col << " ";
        }
        std::cout << std::endl;
      }
    } else {
      std::cerr << "Failed to get parser for " << fileName1 << std::endl;
    }

  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }

  return 0;
}
