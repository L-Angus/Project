#ifndef HARDWARESTRATEGY_H
#define HARDWARESTRATEGY_H

#include "CFGFileManager.hpp"
#include "DynamicQueryPolicy.hpp"
#include "RFModuleConfigure.h"
#include <bitset>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "Common.h"

template <typename CRTP> class HardwareStrategy {
public:
  void SetQueryParams(const QueryParams &params) {
    static_cast<CRTP *>(this)->SetQueryParams(params);
  }
  void BuildStrategy() { static_cast<CRTP *>(this)->Build(); }
  void ApplyStrategy() { static_cast<CRTP *>(this)->Apply(); }
  void ResetStrategy() { static_cast<CRTP *>(this)->Reset(); }
};

class RXStrategy : public HardwareStrategy<RXStrategy> {
public:
  RXStrategy(const SlotChannelMap &slotChannels) : m_slotChannels(slotChannels) {}

  void SetQueryParams(const QueryParams &params) { m_queryParams = params; }

  void Build() {
    ParseConfigFile();
    QueryAndCacheData();
  }

  void Apply() {
    for (const auto &[slot, slotData] : m_slotDataMapping) {
      // 配置 FE 模块
      ConfigureAndGenerate<FEModule, 256>(slot, slotData, RFType_E::FE, 0x1000);
      // 配置 REC 模块
      ConfigureAndGenerate<RECModule, 128>(slot, slotData, RFType_E::REC, 0x2000);
    }
    std::cout << "RX Strategy applied successfully." << std::endl;
  }

  void Reset() {
    m_configs.clear();
    m_slotDataMapping.clear();
  }

  const std::vector<RFRegConfig> &GetConfig() const { return m_configs; }

private:
  // 通用模块配置和寄存器生成
  template <typename Module, size_t BitSize>
  void ConfigureAndGenerate(uint32_t slot, const std::vector<SlotData> &slotData, RFType_E type,
                            uint32_t offset) {
    Module module(slot, slotData, m_queryParams);
    module.Configure();
    auto config = module.GetConfiguration();

    // 打印模块配置的比特位
    std::cout << " Module Combined Bits: " << config.bits.to_string() << std::endl;

    // 使用寄存器配置生成器
    RegConfigGenerator<std::bitset<BitSize>> generator(slot, config.bits, offset);
    m_configs.push_back(generator.Generate(type));
  }
  void ParseConfigFile() {
    auto &manager = CFGFileManager::GetInstance();
    mPrser = manager.GetParser("RX", "RX.csv");
    if (!mPrser) {
      throw std::runtime_error("RXStrategy::ParseConfigFile() failed to get parser");
    }
    mPrser->parse();
  }

  void QueryAndCacheData() {
    DataQueryEngine engine(mPrser);

    for (const auto &[slot, ports] : m_slotChannels) {
      PortNoQueryPolicy portQuery(ports);
      auto result = engine.ExecuteQuery(portQuery);

      if (result.GetMatchedRowCount() == 0) {
        throw std::runtime_error(
            "RXStrategy::QueryAndCacheData() failed to get query result for slot " +
            std::to_string(slot));
      }

      auto resultData = result.GetMatchedRows();
      std::vector<SlotData> slotData;

      for (const auto &row : resultData) {
        // 添加 FE 数据
        slotData.push_back(CreateSlotData(row, RFType_E::FE, 1, row[0]));
        // 添加 REC 数据
        slotData.push_back(CreateSlotData(row, RFType_E::REC, 2, std::nullopt));
      }
      m_slotDataMapping[slot] = std::move(slotData);
    }
  }

  // 通用逻辑提取：创建 SlotData
  SlotData CreateSlotData(const Row &row, RFType_E type, int moduleColumnIndex,
                          std::optional<std::string_view> portColumn) const {
    ModuleInfo moduleInfo;
    moduleInfo.moduleName = (type == RFType_E::FE ? "FE" : "REC");
    moduleInfo.moduleID = std::stoi(std::string(row[moduleColumnIndex]));
    std::optional<uint32_t> portNo;
    if (portColumn) {
      portNo = std::stoi(std::string(*portColumn));
    }
    return SlotData{type, moduleInfo, portNo};
  }

  SlotChannelMap m_slotChannels;
  QueryParams m_queryParams;
  CFGFileParser::CFGFileParserPtr mPrser;
  std::unordered_map<uint32_t, std::vector<SlotData>> m_slotDataMapping;
  std::vector<RFRegConfig> m_configs;
};

class TXCWStrategy : public HardwareStrategy<TXCWStrategy> {
public:
  void Build() {
    auto &manager = CFGFileManager::GetInstance();
    mPrser =
        manager.GetParser(TX::CW::HW::ModuleName, std::string{TX::CW::HW::ModuleName} + ".csv");
    if (!mPrser) {
      throw std::runtime_error("TXCWStrategy::Build() failed to get parser");
    }
    mPrser->parse();
    QueryAndCacheData(mPrser);
  }
  void Apply() {
    for (const auto &[slot, ports] : m_slotChannels) {
      ConfigureAndGenerate<FEModule, 256>(slot, m_slotDataMapping[slot], RFType_E::FE, 0x1000);
      // ConfigureAndGenerate<FEModule, 256>(slot, m_slotDataMapping[slot], RFType_E::FE, 0x1000);
    }
  }
  void Reset() {
    m_configs.clear();
    m_slotChannels.clear();
  }
  const std::vector<RFRegConfig> &GetConfig() const { return m_configs; }

private:
  // 通用模块配置和寄存器生成
  template <typename Module, size_t BitSize>
  void ConfigureAndGenerate(uint32_t slot, const std::vector<SlotData> &slotData, RFType_E type,
                            uint32_t offset) {
    Module module(slot, slotData, m_queryParams);
    module.Configure();
    auto config = module.GetConfiguration();

    // 打印模块配置的比特位
    std::cout << " Module Combined Bits: " << config.bits.to_string() << std::endl;

    // 使用寄存器配置生成器
    RegConfigGenerator<std::bitset<BitSize>> generator(slot, config.bits, offset);
    m_configs.push_back(generator.Generate(type));
  }

  void QueryAndCacheData(CFGFileParser::CFGFileParserPtr parser) {
    DataQueryEngine engine(parser);

    for (const auto &[slot, ports] : m_slotChannels) {
      PortNoQueryPolicy portQuery(ports);
      auto result = engine.ExecuteQuery(portQuery);

      if (result.GetMatchedRowCount() == 0) {
        throw std::runtime_error(
            "RXStrategy::QueryAndCacheData() failed to get query result for slot " +
            std::to_string(slot));
      }

      auto resultData = result.GetMatchedRows();
      std::vector<SlotData> slotData;

      for (const auto &row : resultData) {
        // 添加 FE 数据
        slotData.push_back(CreateSlotData(row, RFType_E::FE, 1, row[0]));
        // 添加 REC 数据
        slotData.push_back(CreateSlotData(row, RFType_E::REC, 2, std::nullopt));
      }
      m_slotDataMapping[slot] = std::move(slotData);
    }
  }
  std::string GetCurrentModuleName() const { return std::string{TX::CW::HW::ModuleName}; }
  std::string GetCurrentModuleFile() const { return std::string{TX::CW::HW::ModuleName} + ".csv"; }

  std::string GetModuleNameByType(RFType_E type) const {
    static const std::unordered_map<RFType_E, std::string> typeToModuleName = {
        {RFType_E::FE, "FE"}, {RFType_E::REC, "REC"}};
    auto it = typeToModuleName.find(type);
    if (it != typeToModuleName.end()) {
      return it->second;
    }
    throw std::runtime_error("Invalid RFType_E value");
  }

  SlotData CreateSlotData(const Row &row, RFType_E type, int moduleColumnIndex,
                          std::optional<std::string_view> portColumn) const {
    ModuleInfo moduleInfo;
    moduleInfo.moduleName = GetModuleNameByType(type);
    moduleInfo.moduleID = std::stoi(std::string(row[moduleColumnIndex]));
    if (type == RFType_E::FE) {
      moduleInfo.priorModule = "SGC" + std::string(row[moduleColumnIndex - 1]);
    }
    std::optional<uint32_t> portNo;
    if (portColumn) {
      portNo = std::stoi(std::string(*portColumn));
    }
    return SlotData{type, moduleInfo, portNo};
  }

private:
  SlotChannelMap m_slotChannels;
  QueryParams m_queryParams;
  CFGFileParser::CFGFileParserPtr mPrser;
  std::unordered_map<uint32_t, std::vector<SlotData>> m_slotDataMapping;
  std::vector<RFRegConfig> m_configs;
  inline static std::string GetParentNode{"TX/CW"};
};

class TXMODStrategy : public HardwareStrategy<TXMODStrategy> {
public:
  void Build() {
    auto &manager = CFGFileManager::GetInstance();
    mPrser =
        manager.GetParser(TX::CW::HW::ModuleName, std::string{TX::CW::HW::ModuleName} + ".csv");
    if (!mPrser) {
      throw std::runtime_error("TXCWStrategy::Build() failed to get parser");
    }
    mPrser->parse();
    QueryAndCacheData(mPrser);
  }
  void Apply() {
    for (const auto &[slot, ports] : m_slotChannels) {
      ConfigureAndGenerate<FEModule, 256>(slot, m_slotDataMapping[slot], RFType_E::FE, 0x1000);
      // ConfigureAndGenerate<FEModule, 256>(slot, m_slotDataMapping[slot], RFType_E::FE, 0x1000);
    }
  }
  void Reset() {
    m_configs.clear();
    m_slotChannels.clear();
  }
  const std::vector<RFRegConfig> &GetConfig() const { return m_configs; }

private:
  // 通用模块配置和寄存器生成
  template <typename Module, size_t BitSize>
  void ConfigureAndGenerate(uint32_t slot, const std::vector<SlotData> &slotData, RFType_E type,
                            uint32_t offset) {
    Module module(slot, slotData, m_queryParams);
    module.Configure();
    auto config = module.GetConfiguration();

    // 打印模块配置的比特位
    std::cout << " Module Combined Bits: " << config.bits.to_string() << std::endl;

    // 使用寄存器配置生成器
    RegConfigGenerator<std::bitset<BitSize>> generator(slot, config.bits, offset);
    m_configs.push_back(generator.Generate(type));
  }

  void QueryAndCacheData(CFGFileParser::CFGFileParserPtr parser) {
    DataQueryEngine engine(parser);

    for (const auto &[slot, ports] : m_slotChannels) {
      PortNoQueryPolicy portQuery(ports);
      auto result = engine.ExecuteQuery(portQuery);

      if (result.GetMatchedRowCount() == 0) {
        throw std::runtime_error(
            "RXStrategy::QueryAndCacheData() failed to get query result for slot " +
            std::to_string(slot));
      }

      auto resultData = result.GetMatchedRows();
      std::vector<SlotData> slotData;

      for (const auto &row : resultData) {
        // 添加 FE 数据
        slotData.push_back(CreateSlotData(row, RFType_E::FE, 1, row[0]));
        // 添加 REC 数据
        slotData.push_back(CreateSlotData(row, RFType_E::REC, 2, std::nullopt));
      }
      m_slotDataMapping[slot] = std::move(slotData);
    }
  }
  std::string GetCurrentModuleName() const { return std::string{TX::CW::HW::ModuleName}; }
  std::string GetCurrentModuleFile() const { return std::string{TX::CW::HW::ModuleName} + ".csv"; }

  std::string GetModuleNameByType(RFType_E type) const {
    static const std::unordered_map<RFType_E, std::string> typeToModuleName = {
        {RFType_E::FE, "FE"}, {RFType_E::REC, "REC"}};
    auto it = typeToModuleName.find(type);
    if (it != typeToModuleName.end()) {
      return it->second;
    }
    throw std::runtime_error("Invalid RFType_E value");
  }

  SlotData CreateSlotData(const Row &row, RFType_E type, int moduleColumnIndex,
                          std::optional<std::string_view> portColumn) const {
    ModuleInfo moduleInfo;
    moduleInfo.moduleName = GetModuleNameByType(type);
    moduleInfo.moduleID = std::stoi(std::string(row[moduleColumnIndex]));
    if (type == RFType_E::FE) {
      moduleInfo.priorModule = "SGC" + std::string(row[moduleColumnIndex - 1]);
    }
    std::optional<uint32_t> portNo;
    if (portColumn) {
      portNo = std::stoi(std::string(*portColumn));
    }
    return SlotData{type, moduleInfo, portNo};
  }

private:
  SlotChannelMap m_slotChannels;
  QueryParams m_queryParams;
  CFGFileParser::CFGFileParserPtr mPrser;
  std::unordered_map<uint32_t, std::vector<SlotData>> m_slotDataMapping;
  std::vector<RFRegConfig> m_configs;
  inline static std::string GetParentNode{"TX/MOD"};
};

#endif // HARDWARESTRATEGY_H