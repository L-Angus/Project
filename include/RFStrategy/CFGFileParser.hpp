/*
 * Filename:
 * /Users/lishundong/Desktop/2023/projects/Project/include/ModuleParser.hpp
 * Path: /Users/lishundong/Desktop/2023/projects/Project/include
 * Created Date: Wednesday, December 4th 2024, 11:19:38 pm
 * Author: 李顺东
 *
 * Copyright (c) 2024 Your Company
 */

#ifndef __MODULEPARSER_HPP__
#define __MODULEPARSER_HPP__

#include "../CSVReader.h"
#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Base class for all module parsers
class CFGFileParser {
public:
  using CFGFileParserPtr = std::shared_ptr<CFGFileParser>;
  virtual ~CFGFileParser() = default;
  virtual void parse() = 0;
  virtual CSVParser::DataContainer GetModuleCFGData() const = 0;
  virtual std::any OnQuery(QueryStrategyCallback query) = 0;
  virtual const std::vector<std::string_view> &GetColumnNames() const = 0;
  const std::string &GetModuleName() const { return m_moduleName; }
  // 新增行的接口。此处改为 AddFittedRow，支持单行或批量
  virtual bool AddFittedRows(const std::vector<std::vector<std::string>> &fittedRows) = 0;

protected:
  CFGFileParser(std::string moduleName) : m_moduleName(std::move(moduleName)) {}
  std::string m_moduleName;
};

// Template for a generic parser
template <typename Module> class GenericParser : public CFGFileParser {
public:
  explicit GenericParser(const std::string &cfg, ParseMode mode = ParseMode::Synchronous)
      : CFGFileParser(Module::ModuleName), m_cfg(cfg), m_parser(mode) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override { return m_parser.GetCSVData(); }
  std::any OnQuery(QueryStrategyCallback query) override { return m_parser.OnQuery(query); }
  const std::vector<std::string_view> &GetColumnNames() const override {
    return m_parser.GetColumnNames();
  };

  /**
   * @brief 将一行或多行“拟合后的字符串”写入CSVParser。
   * @param fittedRows 多行，每行是 std::vector<std::string> 类型
   * @return 若插入成功则返回true；有异常则返回false或抛出
   */
  bool AddFittedRows(const std::vector<std::vector<std::string>> &fittedRows) override {
    try {
      for (auto &rowStrings : fittedRows) {
        // 1) 先缓存到本地，避免string_view悬空问题
        m_fitted_data_cache.push_back(rowStrings);

        // 2) 把这行的末尾取出，用 string_view 封装
        auto &backRow = m_fitted_data_cache.back();
        std::vector<std::string_view> rowView;
        rowView.reserve(backRow.size());
        for (auto &str : backRow) {
          rowView.push_back(str);
        }

        // 3) 通过 OnAdd 往CSVParser内部追加
        m_parser.OnAdd([&rowView](CSVParser::DataContainer &data) { data.emplace_back(rowView); });
      }
    } catch (const std::exception &ex) {
      std::cerr << "[GenericParser] AddFittedRows failed: " << ex.what() << std::endl;
      return false;
    }
    return true;
  }

private:
  CSVParser m_parser;
  std::string m_cfg;
  std::vector<std::vector<std::string>> m_fitted_data_cache;
};
namespace RX {
struct FE {
  static constexpr inline const char *ModuleName = "FE";
};
struct REC {
  static constexpr inline const char *ModuleName = "REC";
};
struct HW {
  static constexpr inline const char *ModuleName = "HW";
};
} // namespace RX

namespace TX::CW {
struct FE {
  static constexpr inline const char *ModuleName = "CW/FE";
};
struct DAC {
  static constexpr inline const char *ModuleName = "CW/REC";
};
struct MOD {
  static constexpr inline const char *ModuleName = "CW/MOD";
};
struct HW {
  static constexpr inline const char *ModuleName = "CW/HW";
};
struct SGC1 {
  static constexpr inline const char *ModuleName = "CW/SGC1";
};
struct SGC2 {
  static constexpr inline const char *ModuleName = "CW/SGC2";
};

}; // namespace TX::CW
namespace TX::DT {
struct FE {
  static constexpr inline const char *ModuleName = "DT/FE";
};
struct DAC {
  static constexpr inline const char *ModuleName = "DT/REC";
};
struct MOD {
  static constexpr inline const char *ModuleName = "DT/MOD";
};
struct HW {
  static constexpr inline const char *ModuleName = "DT/HW";
};
struct SGC1 {
  static constexpr inline const char *ModuleName = "DT/SGC1";
};
struct SGC2 {
  static constexpr inline const char *ModuleName = "DT/SGC2";
};
}; // namespace TX::DT

namespace TX::MOD {
struct FE {
  static constexpr inline const char *ModuleName = "MOD/FE";
};
struct DAC {
  static constexpr inline const char *ModuleName = "MOD/REC";
};
struct MOD {
  static constexpr inline const char *ModuleName = "MOD/MOD";
};
struct HW {
  static constexpr inline const char *ModuleName = "MOD/HW";
};
struct SGC1 {
  static constexpr inline const char *ModuleName = "MOD/SGC1";
};
struct SGC2 {
  static constexpr inline const char *ModuleName = "MOD/SGC2";
};
}; // namespace TX::MOD

// Factory function to create parsers
template <typename Module> CFGFileParser::CFGFileParserPtr CreateParser(const std::string &cfg) {
  return std::make_shared<GenericParser<Module>>(cfg);
}

#endif // __MODULEPARSER_HPP__