#ifndef CFGFILEMANAGER_HPP
#define CFGFILEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "CFGFileNode.hpp"
#include "CFGFileParser.hpp"

class CFGFileManager {
public:
  using ParserCreator = std::function<CFGFileParser::CFGFileParserPtr(const std::string &)>;
  using ParserCreatorMap = std::unordered_map<std::string, ParserCreator>;
  using CFGParserMap = std::unordered_map<std::string, CFGFileParser::CFGFileParserPtr>;
  static CFGFileManager &GetInstance() {
    static CFGFileManager instance;
    return instance;
  }
  CFGFileManager(const CFGFileManager &) = delete;
  CFGFileManager &operator=(const CFGFileManager &) = delete;
  void SetRootPath(const std::string &root_path) {
    if (root_path.empty()) {
      throw CFGFileNodeException("Root path cannot be empty!");
    }
    std::string normalizedPath = NormalizePath(root_path);
    auto absolutePath = std::filesystem::absolute(normalizedPath);
    if (!std::filesystem::exists(absolutePath)) {
      throw CFGFileNodeException("Root path does not exist: " + normalizedPath);
    }
    if (!std::filesystem::is_directory(absolutePath)) {
      throw CFGFileNodeException("Root path is not a directory: " + normalizedPath);
    }
    this->mRootPath = absolutePath.string() + "/Config";
    mRootNode = CFGFileNode::Create(this->mRootPath);
  }

  void LoadCFGFile(const std::string &moduleName, const std::string &fileName) {
    EnsureRootPathSet();
    // 构造文件的完整路径
    std::string fullPath = mRootPath + "/" + moduleName + "/" + fileName;
    if (!std::filesystem::exists(fullPath)) {
      throw CFGFileNodeException("Config file does not exist: " + fullPath);
    }
    // 查找或创建模块节点
    auto moduleNode = mRootNode->GetChild(moduleName);
    if (!moduleNode) {
      moduleNode = CFGFileNode::Create(moduleName);
      mRootNode->AddChild(moduleName, moduleNode);
    }
    auto fileNode = CFGFileNode::Create(fileName, fullPath);
    moduleNode->AddChild(fileName, fileNode);

    auto it = mParserCreators.find(moduleName);
    if (it == mParserCreators.end()) {
      throw CFGFileNodeException("No parser registered for module: " + moduleName);
    }
    auto parser = it->second(fullPath);
    mCFGParsers.try_emplace(fullPath, std::move(parser));
  }

  // 获取文件路径
  std::string GetCFGFilePath(const std::string &moduleName, const std::string &fileName) const {
    EnsureRootPathSet();

    auto moduleNode = mRootNode->GetChild(moduleName);
    if (!moduleNode) {
      throw CFGFileNodeException("Module not found: " + moduleName);
    }

    auto fileNode = moduleNode->GetChild(fileName);
    if (!fileNode || !fileNode->isFile()) {
      throw CFGFileNodeException("File not found in module: " + fileName);
    }

    return fileNode->GetFilePath();
  }

  CFGFileNode::FileNodePtr GetRootNode() const { return mRootNode; }

  CFGFileParser::CFGFileParserPtr GetParser(const std::string &moduleName,
                                            const std::string &fileName) const {
    EnsureRootPathSet();
    std::string fullPath = mRootPath + "/" + moduleName + "/" + fileName;
    std::cout << "[FullPath]: " << fullPath << std::endl;
    if (!std::filesystem::exists(fullPath)) {
      throw CFGFileNodeException("File not found: " + fullPath);
    }
    auto hit = mCFGParsers.find(fullPath);
    if (hit == mCFGParsers.end()) {
      throw CFGFileNodeException("Parser not found for file: " + fullPath);
    }
    return hit->second;
  }

  ParserCreator GetParserCreator(const std::string &moduleName) const {
    auto it = mParserCreators.find(moduleName);
    if (it == mParserCreators.end()) {
      throw CFGFileNodeException("No parser registered for module: " + moduleName);
    }
    return it->second;
  }

  void LoadAllCFGFiles() {
    EnsureRootPathSet();
    TraverseAndLoadFiles(mRootPath);
  }

  void Clear() {
    mCFGParsers.clear();
    mRootNode.reset();
    mRootPath.clear();
  }

  const std::unordered_map<std::string, ParserCreator> &GetParserCreators() const {
    return mParserCreators;
  }

private:
  CFGFileManager() {
    RegisterModuleParser("FE", [](const std::string &fe) { return CreateParser<FE>(fe); });
    RegisterModuleParser("REC", [](const std::string &rec) { return CreateParser<REC>(rec); });
    RegisterModuleParser("Strategy", [](const std::string &rx) { return CreateParser<RX>(rx); });
  }

  ~CFGFileManager() {
    Clear();
    mParserCreators.clear();
  }
  std::string NormalizePath(const std::string &rawPath) {
    std::filesystem::path path(rawPath);
    path = std::filesystem::weakly_canonical(path);
    return path.generic_string();
  }

  void RegisterModuleParser(const std::string &moduleName, ParserCreator parser) {
    mParserCreators[moduleName] = std::move(parser);
  }

  void TraverseAndLoadFiles(const std::string &currentPath) {
    for (const auto &entry : std::filesystem::directory_iterator(currentPath)) {
      if (entry.is_directory()) {
        auto dirName = entry.path().filename().string();
        auto dirNode = CFGFileNode::Create(dirName);
        mRootNode->AddChild(dirName, dirNode);
        TraverseAndLoadFiles(entry.path().generic_string());
      } else if (entry.is_regular_file()) {
        auto fileName = entry.path().filename().string();
        auto moduleName = entry.path().parent_path().filename().string();
        LoadCFGFile(moduleName, fileName);
      }
    }
  }

  void EnsureRootPathSet() const {
    if (mRootPath.empty()) {
      throw CFGFileNodeException("Root path is not set!");
    }
  }

  std::string mRootPath;
  CFGFileNode::FileNodePtr mRootNode;
  std::unordered_map<std::string, ParserCreator> mParserCreators;
  std::unordered_map<std::string, CFGFileParser::CFGFileParserPtr> mCFGParsers;
};

#endif // CFGFILEMANAGER_HPP