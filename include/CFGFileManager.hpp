#ifndef CFGFILEMANAGER_HPP
#define CFGFILEMANAGER_HPP

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "CFGFileNode.hpp"

class CFGFileManager {
public:
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
    std::cout << "Root path: " << normalizedPath << std::endl;
    auto absolutePath = std::filesystem::absolute(normalizedPath);
    if (!std::filesystem::exists(absolutePath)) {
      throw std::runtime_error("Root path does not exist: " + normalizedPath);
    }
    if (!std::filesystem::is_directory(absolutePath)) {
      throw std::runtime_error("Root path is not a directory: " +
                               normalizedPath);
    }
    this->mRootPath = absolutePath.string();
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
    moduleNode->AddChild(fileName,
                         std::make_shared<CFGFileNode>(fileName, fullPath));
    // TDOD: 添加文件内容解析
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

  void PrintTree() const { mRootNode->PrintTree(); };

private:
  CFGFileManager() = default;
  std::string NormalizePath(const std::string &rawPath) {
    std::filesystem::path path(rawPath);
    path = std::filesystem::weakly_canonical(path);
    return path.generic_string();
  }

  std::string mRootPath;
  CFGFileNode::FileNodePtr mRootNode;
  //   std::unordered_map<std::string, .>
};

#endif // CFGFILEMANAGER_HPP