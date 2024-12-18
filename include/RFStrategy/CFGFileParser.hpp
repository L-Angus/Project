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

private:
  CSVParser m_parser;
  std::string m_cfg;
};

struct FE {
  static constexpr inline const char *ModuleName = "FE";
};
struct REC {
  static constexpr inline const char *ModuleName = "REC";
};
struct RX {
  static constexpr inline const char *ModuleName = "RX";
};

using FEParser = GenericParser<FE>;
using RECParser = GenericParser<REC>;
using RXParser = GenericParser<RX>;

// Factory function to create parsers
template <typename Module> CFGFileParser::CFGFileParserPtr CreateParser(const std::string &cfg) {
  return std::make_shared<GenericParser<Module>>(cfg);
}

#endif // __MODULEPARSER_HPP__