#ifndef HEADER_INFO_H
#define HEADER_INFO_H

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

class HeaderInfo {
public:
  using HeaderField = std::variant<double, int, std::string, bool>;
  using HeaderMap = std::unordered_map<std::string, HeaderField>;

  HeaderInfo() {
    // 初始化固定的头信息字段及默认值
    headerMap_["InputZoom"] = 1.0;
    headerMap_["InputCenter"] = 0;
    headerMap_["XUnit"] = "Sec";
    headerMap_["YUnit"] = "V";
    updateTimeString(); // 初始化 TimeString 字段为当前时间
  }

  // 添加字段到头信息
  void addField(const std::string &key, const HeaderField &value) {
    static const std::unordered_set<std::string> reservedFields = {
        "TimeString"};
    if (reservedFields.find(key) != reservedFields.end()) {
      throw std::invalid_argument(
          key + " is a reserved field and cannot be manually overridden.");
    }
    headerMap_[key] = value;
  }

  // 生成当前时间字符串，格式为 "Thu Apr 11 09:17:33.002 2024"
  std::string generateCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) %
                        1000;
    char buffer[100];
    if (std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S",
                      std::localtime(&currentTime))) {
      std::ostringstream oss;
      oss << buffer << '.' << std::setfill('0') << std::setw(3)
          << milliseconds.count() << ' '
          << (1900 + std::localtime(&currentTime)->tm_year);
      return oss.str();
    } else {
      throw std::runtime_error("Failed to format current time.");
    }
  }

  // 更新 TimeString 字段为当前时间
  void updateTimeString() {
    headerMap_["TimeString"] = generateCurrentTimeString();
  }

  // 更新已有字段的值
  void updateField(const std::string &key, const HeaderField &value) {
    if (headerMap_.find(key) != headerMap_.end()) {
      if (key == "TimeString") {
        throw std::invalid_argument(
            "TimeString must be updated using updateTimeString().");
      }
      headerMap_[key] = value;
    } else {
      throw std::invalid_argument("Field does not exist: " + key);
    }
  }

  // 获取头信息数据，返回 const 引用以避免拷贝
  const HeaderMap &getHeaderMap() const { return headerMap_; }

private:
  HeaderMap headerMap_;
};

#endif // HEADER_INFO_H
