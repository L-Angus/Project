#ifndef DynamicQueryPolicy_HPP
#define DynamicQueryPolicy_HPP

#include "../CSVReader.h"
#include "CFGFileParser.hpp"

#include <any>
#include <set>
#include <unordered_set>
#include <vector>

using Row = std::vector<std::string_view>; // 单行数据
using DataContainer = std::vector<Row>;    // 匹配的数据集合

class QueryResult {
public:
  QueryResult() = default;
  size_t GetMatchedRowCount() const { return matchedRows.size(); }
  const DataContainer &GetMatchedRows() const { return matchedRows; }
  void AddMatchedRow(const Row &row) { matchedRows.push_back(row); }
  bool IsEmpty() const { return matchedRows.empty(); }

private:
  DataContainer matchedRows; // 匹配的行数据
};

class IQueryPolicy {
public:
  virtual ~IQueryPolicy() = default;
  virtual bool Execute(const DataContainer &data, QueryResult &result) const = 0;
};

class PortNoQueryPolicy : public IQueryPolicy {
public:
  explicit PortNoQueryPolicy(const std::vector<uint32_t> &portNos)
      : m_portNos(portNos.begin(), portNos.end()) {}

  bool Execute(const DataContainer &data, QueryResult &result) const override {
    for (const auto &row : data) {
      if (Matches(row)) {
        result.AddMatchedRow(row);
      }
    }
    return !result.GetMatchedRows().empty();
  }

private:
  std::unordered_set<uint32_t> m_portNos; // 批量端口集合

  // 判断行是否匹配条件
  bool Matches(const Row &row) const {
    return m_portNos.find(stoi(std::string{row[0]})) != m_portNos.end();
  }
};

class FreqPowerQueryPolicy : public IQueryPolicy {
public:
  FreqPowerQueryPolicy(double freq, double power) : m_freq(freq), m_power(power) {}
  bool Execute(const DataContainer &data, QueryResult &result) const override {
    for (const auto &row : data) {
      if (Matches(row)) {
        result.AddMatchedRow(row);
      }
    }
    return !result.IsEmpty();
  }

private:
  bool Matches(const Row &row) const {
    double freq = stod(std::string{row[0]});
    double power = stod(std::string{row[1]});
    return freq == m_freq && power == m_power;
  }

  double m_freq;
  double m_power;
};

class DataQueryEngine {
public:
  DataQueryEngine(CFGFileParser::CFGFileParserPtr parser) : m_parser(std::move(parser)) {}

  QueryResult ExecuteQuery(const IQueryPolicy &policy) {
    QueryResult result;
    [[maybe_unused]] bool success = policy.Execute(m_parser->GetModuleCFGData(), result);
    return result;
  }

  CFGFileParser::CFGFileParserPtr GetOwnership() const { return m_parser; }

  // 可选“验收式”再查询，校验插入的数据确实可见
  [[maybe_unused]] bool
  VerifyRowExists(std::function<bool(const CSVParser::DataContainer &)> checker) {
    auto data = m_parser->GetModuleCFGData();
    return checker(data);
  }

  // 缓存插值数据（单行或多行）
  bool AddFittedRows(const std::vector<std::vector<std::string>> &rows) {
    return m_parser->AddFittedRows(rows);
  }

private:
  CFGFileParser::CFGFileParserPtr m_parser;
};

#endif