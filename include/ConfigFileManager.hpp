#ifndef CONFIG_FILE_MANAGER_HPP
#define CONFIG_FILE_MANAGER_HPP
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

class TreeNode {
public:
  TreeNode(const std::string &name, const std::string &filePath = "")
      : name(name), filePath(filePath) {}

  void addChild(const std::string &childName,
                const std::shared_ptr<TreeNode> &child) {
    children[childName] = child;
  }

  std::shared_ptr<TreeNode> getChild(const std::string &childName) const {
    auto it = children.find(childName);
    if (it != children.end()) {
      return it->second;
    }
    return nullptr;
  }

  const std::string &getFilePath() const { return filePath; }

  void printTree(int level = 0) const {
    for (int i = 0; i < level; ++i)
      std::cout << "  ";
    std::cout << name;
    if (!filePath.empty()) {
      std::cout << " (" << filePath << ")";
    }
    std::cout << std::endl;
    for (const auto &[key, child] : children) {
      child->printTree(level + 1);
    }
  }

private:
  std::string name;
  std::string filePath;
  std::unordered_map<std::string, std::shared_ptr<TreeNode>> children;
};

class ConfigFileManager {
public:
  static ConfigFileManager &getInstance() {
    static ConfigFileManager instance;
    return instance;
  }

  ConfigFileManager(const ConfigFileManager &) = delete;
  ConfigFileManager &operator=(const ConfigFileManager &) = delete;

  void setRootPath(const std::string &rootPath) {
    std::filesystem::path rootDir = normalizePath(rootPath);

    if (!std::filesystem::exists(rootDir) ||
        !std::filesystem::is_directory(rootDir)) {
      throw std::runtime_error("Invalid rootPath: " + rootDir.string());
    }

    this->rootPath = rootDir.string();
    isRootPathSet = true;
  }

  std::string getRootPath() const {
    if (!isRootPathSet) {
      throw std::runtime_error("Root path is not set!");
    }
    return rootPath;
  }

  void addFile(const std::string &rawPath, const std::string &filePath = "") {
    if (!isRootPathSet) {
      throw std::runtime_error(
          "Root path is not set. Cannot add files or directories.");
    }

    if (rawPath.empty()) {
      throw std::invalid_argument("Path cannot be empty");
    }

    // 标准化路径并拼接完整路径
    std::string relativePath = normalizePath(rawPath);
    std::filesystem::path fullPath =
        std::filesystem::path(rootPath) / relativePath;

    if (!filePath.empty()) {
      fullPath /= filePath; // 进一步拼接文件名
    }

    // 校验文件或目录是否存在
    if (!std::filesystem::exists(fullPath)) {
      throw std::runtime_error("File or directory does not exist: " +
                               fullPath.string());
    }

    auto current = root;

    // 解析路径中的目录结构
    size_t pos = 0;
    std::string token;
    std::string remainingPath = relativePath;

    while ((pos = remainingPath.find('/')) != std::string::npos) {
      token = remainingPath.substr(0, pos);
      if (!current->getChild(token)) {
        current->addChild(token, std::make_shared<TreeNode>(token));
      }
      current = current->getChild(token);
      remainingPath.erase(0, pos + 1);
    }

    // 最后一个节点：当前目录的文件或目录
    if (!current->getChild(remainingPath)) {
      current->addChild(remainingPath,
                        std::make_shared<TreeNode>(remainingPath, filePath));
    } else {
      current->getChild(remainingPath)->setFilePath(filePath);
    }
  }

  std::string getConfigFile(const std::string &rawPath,
                            const std::string &fileName) const {
    if (!isRootPathSet) {
      throw std::runtime_error("Root path is not set!");
    }

    if (rawPath.empty() || fileName.empty()) {
      throw std::invalid_argument("Path or file name cannot be empty");
    }

    // 标准化路径并拼接完整路径
    std::string relativePath = normalizePath(rawPath);
    std::filesystem::path fullPath =
        std::filesystem::path(rootPath) / relativePath / fileName;

    if (!std::filesystem::exists(fullPath)) {
      throw std::runtime_error("Config file does not exist: " +
                               fullPath.string());
    }

    return fullPath.string();
  }

  void printTree() const {
    if (!isRootPathSet) {
      throw std::runtime_error("Root path is not set. Cannot print tree.");
    }
    std::cout << "Root Path: " << rootPath << std::endl;
    root->printTree();
  }

private:
  ConfigFileManager()
      : root(std::make_shared<TreeNode>("Root")), isRootPathSet(false) {}

  static std::string normalizePath(const std::string &rawPath) {
    std::filesystem::path p(rawPath);
    return p.generic_string();
  }

  std::string rootPath;
  std::shared_ptr<TreeNode> root;
  bool isRootPathSet; // 标志 rootPath 是否已经设置
};

#endif // CONFIG_FILE_MANAGER_HPP