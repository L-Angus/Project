#ifndef CFGFILEMANAGER_HPP
#define CFGFILEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

#include "CFGFileNode.hpp"
#include "ModuleParser.hpp"

class CFGFileManager {
public:
  using ParserCreator =
      std::function<ModuleParser::ModuleParserPtr(const std::string &)>;
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
    fileNode->SetParser(parser);
    fileNode->ParseFile();
    mCFGParsers[fullPath] = fileNode->GetParser();
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

  ModuleParser::ModuleParserPtr
  GetParserByFileName(const std::string &filePath) const {
    return FindParserInNode(mRootNode, filePath);
  }

  ModuleParser::ModuleParserPtr
  GetParserByModuleNameAndFileName(const std::string &moduleName,
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
    return LoadAllCFGFilesFromNode(mRootNode);
  }

  void Clear() {
    mCFGParsers.clear();
    mRootNode.reset();
    mRootPath.clear();
    mParserCreators.clear();
  }

private:
  CFGFileManager() {
    RegisterModuleParser("PLL", [](const std::string &cfg) {
      return std::make_shared<PLLParser>(cfg);
    });
    RegisterModuleParser("FE", [](const std::string &cfg) {
      return std::make_shared<FEParser>(cfg);
    });
    RegisterModuleParser("Mod", [](const std::string &cfg) {
      return std::make_shared<ModParser>(cfg);
    });
    RegisterModuleParser("SGC", [](const std::string &cfg) {
      return std::make_shared<SGCParser>(cfg);
    });
    RegisterModuleParser("DAC", [](const std::string &cfg) {
      return std::make_shared<DACParser>(cfg);
    });
  }
  std::string NormalizePath(const std::string &rawPath) {
    std::filesystem::path path(rawPath);
    path = std::filesystem::weakly_canonical(path);
    return path.generic_string();
  }

  void RegisterModuleParser(const std::string &moduleName,
                            ParserCreator parser) {
    mParserCreators[moduleName] = parser;
  }

  void LoadAllCFGFilesFromNode(CFGFileNode::FileNodePtr node) {
    auto curNode = node ? node : mRootNode;
    if (!curNode) {
      throw std::runtime_error("Root node is null!");
    }
    if (!curNode->isDirectory()) {
      std::string dir = (curNode == mRootNode)
                            ? mRootPath
                            : mRootPath + "/" + curNode->GetNodeName();
      ScanFileNode(curNode, dir);
    }
    if (curNode->isFile()) {
      auto parent = curNode->GetParent();
      if (!parent) {
        throw std::runtime_error("File node has no parent!");
      }
      auto moduleName = parent->GetNodeName();
      LoadCFGFile(moduleName, curNode->GetNodeName());
      return;
    }
    for (const auto &[_, child] : curNode->GetChildren()) {
      LoadAllCFGFilesFromNode(child);
    }
  }

  void ScanFileNode(CFGFileNode::FileNodePtr node, const std::string &dir) {
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (entry.is_directory()) {
        auto moduleName = entry.path().filename().string();
        auto moduleNode = node->GetChild(moduleName);
        if (!moduleNode) {
          moduleNode = std::make_shared<CFGFileNode>(moduleName);
          node->AddChild(moduleName, moduleNode);
        }
        ScanFileNode(moduleNode, entry.path().string());
      } else if (entry.is_regular_file()) {
        auto fileName = entry.path().filename().string();
        auto fileNode = node->GetChild(fileName);
        if (!fileNode) {
          fileNode = std::make_shared<CFGFileNode>(fileName);
          node->AddChild(fileName, fileNode);
        }
      }
    }
  }

  ModParser::ModuleParserPtr
  FindParserInNode(CFGFileNode::FileNodePtr node,
                   const std::string &filePath) const {
    if (!node) {
      return nullptr;
    }
    if (node->isFile() && node->GetNodeName() == filePath) {
      return node->GetParser();
    }
    for (const auto &[_, child] : node->GetChildren()) {
      auto parser = FindParserInNode(child, filePath);
      if (parser) {
        return parser;
      }
    }
    return nullptr;
  }

  std::string mRootPath;
  CFGFileNode::FileNodePtr mRootNode;
  std::unordered_map<std::string, ParserCreator> mParserCreators;
  std::unordered_map<std::string, ModuleParser::ModuleParserPtr> mCFGParsers;
};

#endif // CFGFILEMANAGER_HPP