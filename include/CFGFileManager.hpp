#ifndef CFGFILEMANAGER_HPP
#define CFGFILEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "CFGFileNode.hpp"
#include "ModuleParser.hpp"

class CFGFileManager {
public:
  using ParserCreator =
      std::function<ModuleParser::ModuleParserPtr(const std::string &)>;
  using ParserCreatorMap = std::unordered_map<std::string, ParserCreator>;
  using CFGParserMap =
      std::unordered_map<std::string, ModuleParser::ModuleParserPtr>;
  static CFGFileManager &GetInstance() {
    static CFGFileManager instance;
    return instance;
  }
  CFGFileManager(const CFGFileManager &) = delete;
  CFGFileManager &operator=(const CFGFileManager &) = delete;
  void SetRootPath(const std::string &root_path) {
    if (root_path.empty()) {
      throw std::runtime_error("Root path cannot be empty!");
    }
    std::string normalizedPath = NormalizePath(root_path);
    auto absolutePath = std::filesystem::absolute(normalizedPath);
    if (!std::filesystem::exists(absolutePath)) {
      throw std::runtime_error("Root path does not exist: " + normalizedPath);
    }
    if (!std::filesystem::is_directory(absolutePath)) {
      throw std::runtime_error("Root path is not a directory: " +
                               normalizedPath);
    }
    this->mRootPath = absolutePath.string() + "/Config";
    mRootNode = std::make_shared<CFGFileNode>(this->mRootPath);
  }

  void LoadCFGFile(const std::string &moduleName, const std::string &fileName) {
    if (mRootPath.empty()) {
      throw std::runtime_error("Root path is not set!");
    }
    // 构造文件的完整路径
    std::string fullPath = mRootPath + "/" + moduleName + "/" + fileName;
    if (!std::filesystem::exists(fullPath)) {
      throw std::runtime_error("Config file does not exist: " + fullPath);
    }
    // 查找或创建模块节点
    auto moduleNode = mRootNode->GetChild(moduleName);
    if (!moduleNode) {
      moduleNode = std::make_shared<CFGFileNode>(moduleName);
      mRootNode->AddChild(moduleName, moduleNode);
    }
    auto fileNode = std::make_shared<CFGFileNode>(fileName, fullPath);
    moduleNode->AddChild(fileName, fileNode);

    auto it = mParserCreators.find(moduleName);
    if (it == mParserCreators.end()) {
      throw std::runtime_error("No parser registered for module: " +
                               moduleName);
    }
    auto parser = it->second(fullPath);
    parser->parse();
    mCFGParsers[fullPath] = parser;
  }

  // 获取文件路径
  std::string GetCFGFilePath(const std::string &moduleName,
                             const std::string &fileName) const {
    auto moduleNode = mRootNode->GetChild(moduleName);
    if (!moduleNode) {
      throw std::runtime_error("Module not found: " + moduleName);
    }

    auto fileNode = moduleNode->GetChild(fileName);
    if (!fileNode || !fileNode->isFile()) {
      throw std::runtime_error("File not found in module: " + fileName);
    }

    return fileNode->GetFilePath();
  }

  CFGFileNode::FileNodePtr GetRootNode() const { return mRootNode; }

  ModuleParser::ModuleParserPtr GetParser(const std::string &moduleName,
                                          const std::string &fileName) const {
    std::string fullPath = mRootPath + "/" + moduleName + "/" + fileName;
    if (!std::filesystem::exists(fullPath)) {
      throw std::runtime_error("File not found: " + fullPath);
    }
    auto hit = mCFGParsers.find(fullPath);
    if (hit == mCFGParsers.end()) {
      throw std::runtime_error("Parser not found for file: " + fullPath);
    }
    return hit->second;
  }

  void LoadAllCFGFiles() {
    if (mRootPath.empty()) {
      throw std::runtime_error("Root path is not set!");
    }
    TraverseAndLoadFiles(mRootPath);
  }

  void Clear() {
    mCFGParsers.clear();
    mRootNode.reset();
    mRootPath.clear();
    mParserCreators.clear();
  }

  const std::unordered_map<std::string, ParserCreator> &
  GetParserCreators() const {
    return mParserCreators;
  }

private:
  CFGFileManager() {
    RegisterModuleParser("FE", [](const std::string &cfg) {
      return std::make_shared<FEParser>(cfg);
    });
    RegisterModuleParser("REC", [](const std::string &cfg) {
      return std::make_shared<RECParser>(cfg);
    });
  }
  std::string NormalizePath(const std::string &rawPath) {
    std::filesystem::path path(rawPath);
    path = std::filesystem::weakly_canonical(path);
    return path.generic_string();
  }

  void RegisterModuleParser(const std::string &moduleName,
                            ParserCreator parser) {
    mParserCreators[moduleName] = std::move(parser);
  }

  void TraverseAndLoadFiles(const std::string &currentPath) {
    for (const auto &entry : std::filesystem::directory_iterator(currentPath)) {
      if (entry.is_directory()) {
        auto dirName = entry.path().filename().string();
        auto dirNode = std::make_shared<CFGFileNode>(dirName);
        mRootNode->AddChild(dirName, dirNode);
        TraverseAndLoadFiles(entry.path().generic_string());
      } else if (entry.is_regular_file()) {
        auto fileName = entry.path().filename().string();
        auto moduleName = entry.path().parent_path().filename().string();
        LoadCFGFile(moduleName, fileName);
      }
    }
  }

  std::string mRootPath;
  CFGFileNode::FileNodePtr mRootNode;
  std::unordered_map<std::string, ParserCreator> mParserCreators;
  std::unordered_map<std::string, ModuleParser::ModuleParserPtr> mCFGParsers;
};

#endif // CFGFILEMANAGER_HPP