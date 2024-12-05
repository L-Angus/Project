#ifndef DATATYPE_HPP
#define DATATYPE_HPP

#include <complex>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using SitePinRawData = std::map<unsigned int, std::vector<int16_t>>;

using SitePinSignalMap =
    std::map<unsigned int,
             std::map<std::string, std::vector<std::complex<double>>>>;
using SitePinFileMap =
    std::map<unsigned int, std::map<std::string, std::string>>;

enum class RawDataType { SIGNAL, FILE };
enum class RawDataMode { LOCAL, REMOTE };

class RawDataMap {
public:
  using DataVariant = std::variant<SitePinSignalMap, SitePinFileMap>;

  RawDataType dataType;     // 数据类型
  RawDataMode dataMode;     // 数据模式
  DataVariant retrieveData; // 数据存储

  void Clear() {
    retrieveData = SitePinSignalMap{};
    dataType = RawDataType::SIGNAL;
    dataMode = RawDataMode::LOCAL;
  }
  template <typename T> const T &RetrieveData() const {
    if constexpr (std::is_same_v<T, SitePinSignalMap>) {
      if (dataType != RawDataType::SIGNAL) {
        throw std::runtime_error("RawDataMap does not contain signal data");
      }
    } else if constexpr (std::is_same_v<T, SitePinFileMap>) {
      if (dataType != RawDataType::FILE) {
        throw std::runtime_error("RawDataMap does not contain file data");
      }
    } else {
      throw std::runtime_error("Unsupported data type for RetrieveData");
    }
    return std::get<T>(retrieveData);
  }
};
#endif
