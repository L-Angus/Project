#ifndef CONFIG_TABLE_HPP
#define CONFIG_TABLE_HPP

#include "ModuleParser.hpp"

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex> // 用于线程安全
#include <string>
#include <unordered_map>
#include <vector>

// CFGFileNode 类用于管理目录和文件节点
class CFGFileNode : public std::enable_shared_from_this<CFGFileNode> {
public:
  using FileNodePtr = std::shared_ptr<CFGFileNode>;
  using FileNodeMap = std::unordered_map<std::string, FileNodePtr>;

  explicit CFGFileNode(const std::string &node_name,
                       const std::string &file_name = "")
      : m_node_name(node_name), m_file_name(file_name) {
    std::cout << "Creating CFGFileNode: " << m_node_name << std::endl;
  }

  void AddChild(const std::string &childName, const FileNodePtr &childNode) {
    if (isFile()) {
      throw std::runtime_error("Cannot add child to a file node");
    }
    childNode->m_parent = shared_from_this();
    m_children[childName] = childNode;
  }

  FileNodePtr GetChild(const std::string &childName) const {
    auto it = m_children.find(childName);
    return it != m_children.end() ? it->second : nullptr;
  }

  FileNodePtr GetParent() const { return m_parent.lock(); }
  const std::string &GetNodeName() const { return m_node_name; }
  const std::string &GetFilePath() const { return m_file_name; }
  bool isFile() const { return !m_file_name.empty(); }
  const FileNodeMap &GetChildren() const { return m_children; }

  void ScanAndAddFiles(
      const std::string &currentPath,
      const std::unordered_map<
          std::string,
          std::function<ModuleParser::ModuleParserPtr(const std::string &)>>
          &parserCreators,
      std::unordered_map<std::string, ModuleParser::ModuleParserPtr>
          &cfgParsers) {
    for (std::filesystem::directory_iterator it(currentPath);
         it != std::filesystem::directory_iterator(); ++it) {
      try {
        if (it->is_directory()) {
          auto dirName = it->path().filename().string();
          auto dirNode = std::make_shared<CFGFileNode>(dirName);
          this->AddChild(dirName, dirNode);
          dirNode->ScanAndAddFiles(it->path().string(), parserCreators,
                                   cfgParsers); // 递归扫描子目录
        } else if (it->is_regular_file()) {
          auto fileName = it->path().filename().string();
          auto fileNode =
              std::make_shared<CFGFileNode>(fileName, it->path().string());
          this->AddChild(fileName, fileNode);
        }
      } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Filesystem error: " << e.what()
                  << " for path: " << it->path().string() << std::endl;
      }
    }
  }

private:
  std::string m_node_name;
  std::string m_file_name;
  std::weak_ptr<CFGFileNode> m_parent;
  FileNodeMap m_children;
};

// CFGFileManager 单例管理配置文件
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
    std::lock_guard<std::mutex> lock(mMutex); // 加锁，确保线程安全
    if (root_path.empty()) {
      throw std::runtime_error("Root path cannot be empty!");
    }
    std::string normalizedPath = NormalizePath(root_path);
    std::filesystem::path absolutePath =
        std::filesystem::absolute(normalizedPath);
    if (!std::filesystem::exists(absolutePath) ||
        !std::filesystem::is_directory(absolutePath)) {
      throw std::runtime_error("Invalid root path: " + root_path);
    }
    mRootPath = absolutePath.string();
    mRootNode = std::make_shared<CFGFileNode>(mRootPath);
  }

  void LoadCFGFile(const std::string &moduleName, const std::string &fileName) {
    std::lock_guard<std::mutex> lock(mMutex); // 加锁，确保线程安全
    if (mRootPath.empty()) {
      throw std::runtime_error("Root path is not set!");
    }
    std::string fullPath = mRootPath + "/" + moduleName + "/" + fileName;
    if (!std::filesystem::exists(fullPath)) {
      throw std::runtime_error("Config file does not exist: " + fullPath);
    }

    auto moduleNode = mRootNode->GetChild(moduleName);
    if (!moduleNode) {
      moduleNode = std::make_shared<CFGFileNode>(moduleName);
      mRootNode->AddChild(moduleName, moduleNode);
    }
    auto fileNode = std::make_shared<CFGFileNode>(fileName, fullPath);
    moduleNode->AddChild(fileName, fileNode);
  }

  void LoadAllCFGFiles() {
    std::lock_guard<std::mutex> lock(mMutex); // 加锁，确保线程安全
    if (mRootPath.empty()) {
      throw std::runtime_error("Root path is not set!");
    }
    mRootNode->ScanAndAddFiles(mRootPath, mParserCreators, mCFGParsers);
  }

  void RegisterModuleParser(const std::string &moduleName,
                            ParserCreator parser) {
    std::lock_guard<std::mutex> lock(mMutex); // 加锁，确保线程安全
    mParserCreators[moduleName] = std::move(parser);
  }

  ModuleParser::ModuleParserPtr GetParser(const std::string &parentNodeName,
                                          const std::string &fileName) {
    std::lock_guard<std::mutex> lock(mMutex); // 加锁，确保线程安全

    // 构造完整的文件路径
    std::string fullPath = mRootPath + "/" + parentNodeName + "/" + fileName;
    std::cout << "Full path: " << fullPath << std::endl;

    // 校验路径是否存在
    if (!std::filesystem::exists(fullPath)) {
      std::cerr << "Error: File does not exist: " << fullPath << std::endl;
      return nullptr; // 或者抛出异常
    }

    // 检查缓存中是否已有该解析器
    auto it = mCFGParsers.find(fullPath);
    if (it != mCFGParsers.end()) {
      return it->second;
    }

    // 延迟创建解析器
    auto parserCreatorIt = mParserCreators.find(parentNodeName);
    if (parserCreatorIt != mParserCreators.end()) {
      auto parser = parserCreatorIt->second(fullPath);
      mCFGParsers[fullPath] = parser;
      return parser;
    }

    return nullptr;
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

    RegisterModuleParser("REC", [](const std::string &cfg) {
      return std::make_shared<RECParser>(cfg);
    });
  }

  std::string NormalizePath(const std::string &rawPath) {
    std::filesystem::path path(rawPath);
    path = std::filesystem::weakly_canonical(path);
    return path.generic_string();
  }

  std::string mRootPath;
  CFGFileNode::FileNodePtr mRootNode;
  std::unordered_map<std::string, ParserCreator> mParserCreators;
  std::unordered_map<std::string, ModuleParser::ModuleParserPtr> mCFGParsers;
  std::mutex mMutex; // 用于确保线程安全
};

#endif