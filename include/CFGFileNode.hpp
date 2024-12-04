#ifndef CONFIG_FILE_NODE_HPP
#define CONFIG_FILE_NODE_HPP

#include <memory>
#include <string>
#include <unordered_map>

class CFGFileNode {
public:
  typedef std::shared_ptr<CFGFileNode> FileNodePtr;
  explicit CFGFileNode(const std::string &node_name,
                       const std::string &file_name = "")
      : m_node_name(node_name), m_file_name(file_name) {}

  void AddChild(const std::string &childName, const FileNodePtr &childNode) {
    m_children[childName] = childNode;
  }

  FileNodePtr GetChild(const std::string &childName) {
    if (m_children.find(childName) == m_children.end())
      return nullptr;
    return m_children[childName];
  }

  const std::string &GetFilePath() const { return m_file_name; }

  bool isFile() const { return !m_file_name.empty(); }
  bool isDirectory() const { return m_children.size() > 0; }

  void PrintTree(int depth = 0) {
    for (int i = 0; i < depth; i++)
      std::cout << "  ";
    std::cout << m_node_name << std::endl;
    for (auto &child : m_children) {
      child.second->PrintTree(depth + 1);
    }
  }

private:
  std::string m_node_name;
  std::string m_file_name;
  std::unordered_map<std::string, FileNodePtr> m_children;
};

#endif // CONFIG_FILE_NODE_HPP