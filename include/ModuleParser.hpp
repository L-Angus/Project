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

#include "CSVReader.h"
#include <string>
#include <string_view>
#include <vector>

class ModuleParser {
public:
  using CFGData = std::vector<std::vector<std::string_view>>;
  using ModuleParserPtr = std::shared_ptr<ModuleParser>;

  explicit ModuleParser(const std::string &moduleName)
      : m_moduleName(moduleName) {}
  virtual ~ModuleParser() = default;
  virtual void parse() = 0;
  virtual CSVParser::DataContainer GetModuleCFGData() const = 0;
  const std::string &GetModuleName() const { return m_moduleName; }

protected:
  std::string m_moduleName; // 模块名称
};

class PLLParser : public ModuleParser {
public:
  explicit PLLParser(const std::string &cfg)
      : ModuleParser("PLL"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}

  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

class DACParser : public ModuleParser {
public:
  explicit DACParser(const std::string &cfg)
      : ModuleParser("DAC"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

class ModParser : public ModuleParser {
public:
  explicit ModParser(const std::string &cfg)
      : ModuleParser("MOD"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

class SGCParser : public ModuleParser {
public:
  explicit SGCParser(const std::string &cfg)
      : ModuleParser("SGC"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

class FEParser : public ModuleParser {
public:
  explicit FEParser(const std::string &cfg)
      : ModuleParser("FE"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

class RECParser : public ModuleParser {
public:
  explicit RECParser(const std::string &cfg)
      : ModuleParser("REC"), m_cfg(cfg), m_parser(ParseMode::Synchronous) {}
  void parse() override { m_parser.ParseDataFromCSV(m_cfg); }
  CSVParser::DataContainer GetModuleCFGData() const override {
    return m_parser.GetCSVData();
  }

private:
  std::string m_cfg;
  CSVParser m_parser;
};

#endif // __MODULEPARSER_HPP__