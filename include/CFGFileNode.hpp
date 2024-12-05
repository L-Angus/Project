#ifndef CONFIG_FILE_NODE_HPP
#define CONFIG_FILE_NODE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "ModuleParser.hpp"

class CFGFileNode : public std::enable_shared_from_this<CFGFileNode> {
public:
  typedef std::shared_ptr<CFGFileNode> FileNodePtr;
  typedef std::unordered_map<std::string, FileNodePtr> FileNodeMap;

  explicit CFGFileNode(const std::string &node_name,
                       const std::string &file_name = "")
      : m_node_name(node_name), m_file_name(file_name), mParser(nullptr) {
    std::cout << "Node name: " << m_node_name << std::endl;
  }
  void AddChild(const std::string &childName, const FileNodePtr &childNode) {
    if (isFile()) {
      throw std::runtime_error("Cannot add child to a file node");
    }
    childNode->m_parent = shared_from_this();
    m_children[childName] = childNode;
  }

  FileNodePtr GetChild(const std::string &childName) {
    if (m_children.find(childName) == m_children.end())
      return nullptr;
    return m_children[childName];
  }

  FileNodePtr GetParent() { return m_parent.lock(); }
  const std::string &GetNodeName() const { return m_node_name; }
  const std::string &GetFilePath() const { return m_file_name; }
  bool isFile() const { return !m_file_name.empty(); }
  bool isDirectory() const { return m_children.size() > 0; }
  const FileNodeMap &GetChildren() const { return m_children; }

  ModuleParser::ModuleParserPtr GetParser() const { return mParser; }

  void SetParser(const ModuleParser::ModuleParserPtr &parser) {
    if (!isFile()) {
      throw std::runtime_error("Cannot set parser to a directory node");
    }
    mParser = parser;
  }

  void ParseFile() {
    if (mParser) {
      mParser->parse();
    }
  }

private:
  std::string m_node_name;
  std::string m_file_name;
  std::weak_ptr<CFGFileNode> m_parent;
  ModuleParser::ModuleParserPtr mParser;
  std::unordered_map<std::string, FileNodePtr> m_children;
};

#endif // CONFIG_FILE_NODE_HPP