#ifndef RFMODULECONFIGURE_H
#define RFMODULECONFIGURE_H

#include <bitset>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../CSVReader.h"
#include "CFGFileManager.hpp"
#include "Common.h"
#include "DynamicQueryPolicy.hpp"
#include "FEInner.h"

template <size_t N> struct Configuration {
  std::string moduleName;
  std::bitset<N> bits{};
};

using Bit256 = Configuration<256>;
using Bit128 = Configuration<128>;

template <typename RFModule, size_t N = 128> class RFModuleConfigure {
public:
  void Configure() { static_cast<RFModule &>(*this).configure_impl(); }

protected:
  virtual void configure_impl() = 0;
};

// class RECModule : public RFModuleConfigure<RECModule, 128> {
// public:
//   RECModule(const std::vector<unsigned int> &recCombines, double freq, double power)
//       : recCombines(recCombines), freq(freq), power(power) {}
//   void configure_impl() override {
//     config.moduleName = moduleName;
//     std::cout << "Configure " << moduleName << std::endl;
//     for (auto &combine : recCombines) {
//       config.bits.set(combine, true);
//     }
//     std::cout << "Bit size: " << config.bits.size() << std::endl;
//     std::cout << "Freq: " << freq << ", Power: " << power << std::endl;
//   }
//   const Configuration<128> &GetConfiguration() const { return config; }

// private:
//   Configuration<128> config{};
//   std::vector<unsigned int> recCombines;
//   double freq;
//   double power;
//   inline static std::string moduleName{"REC"};
// };
// class FEModule : public RFModuleConfigure<FEModule, 256> {
// public:
//   FEModule(uint32_t slot, const std::vector<uint32_t> &ports,
//            const std::vector<ModuleInfo> &modules, QueryParams queryParams)
//       : mSlot(slot), mPorts(ports), moduleInfos(std::move(modules)), mQueryParams(queryParams) {
//     std::cout << "modules: " << modules.size() << std::endl;
//   }
//   void configure_impl() override {
//     auto &fileManager = CFGFileManager::GetInstance();
//     std::cout << moduleInfos.size() << std::endl;
//     for (const auto &module : moduleInfos) {
//       // 根据端口号查询对应的配置文件
//       std::string filename = "FE" + std::to_string(module.moduleID) + ".csv";
//       std::cout << "Filename: " << filename << std::endl;
//       auto parser = fileManager.GetParser("FE", filename);
//       if (!parser) {
//         throw std::runtime_error("Failed to get parser for FE" +
//         std::to_string(module.moduleID));
//       }

//       parser->parse();
//       DataQueryEngine engine(parser);
//       FreqPowerQueryPolicy freqPowerQuery(mQueryParams.queryFreq, mQueryParams.queryPower);
//       QueryResult result = engine.ExecuteQuery(freqPowerQuery);

//       // 模拟通过 FEInner 设置比特位
//       FEInner feInner;
//       feInner.SetPort(
//           std::set<unsigned int>{mPorts.begin(), mPorts.end()}); // 示例：根据端口号设置比特位
//       auto febits = feInner.GetPort();
//       std::cout << "FE bits: " << febits.to_string() << std::endl;
//       config.bits |= febits; // 合并当前端口的比特位
//     }
//   }
//   const Configuration<256> &GetConfiguration() const { return config; }

// private:
//   Configuration<256> config{};
//   uint32_t mSlot;
//   std::vector<uint32_t> mPorts;
//   std::vector<ModuleInfo> moduleInfos;
//   QueryParams mQueryParams;
//   inline static std::string moduleName{"FE"};
// };
class FEModule : public RFModuleConfigure<FEModule, 256> {
public:
  FEModule(uint32_t slot, const std::vector<SlotData> &slotData, QueryParams queryParams)
      : mSlot(slot), mSlotData(slotData), mQueryParams(queryParams) {}

  void configure_impl() override {
    auto &fileManager = CFGFileManager::GetInstance();

    for (const auto &data : mSlotData) {
      if (data.type != RFType_E::FE || !data.portNo)
        continue;

      const auto &module = data.moduleInfo;
      std::string filename = "FE" + std::to_string(module.moduleID) + ".csv";
      auto parser = fileManager.GetParser("FE", filename);
      if (!parser) {
        throw std::runtime_error("Failed to get parser for FE" + std::to_string(module.moduleID));
      }

      parser->parse();
      DataQueryEngine engine(parser);
      FreqPowerQueryPolicy freqPowerQuery(mQueryParams.queryFreq, mQueryParams.queryPower);
      QueryResult result = engine.ExecuteQuery(freqPowerQuery);

      // 模拟通过 FEInner 设置比特位
      FEInner feInner;
      feInner.SetPort({*data.portNo}); // 使用 portNo
      config.bits |= feInner.GetPort();
    }
  }

  const Configuration<256> &GetConfiguration() const { return config; }

private:
  Configuration<256> config{};
  uint32_t mSlot;
  std::vector<SlotData> mSlotData;
  QueryParams mQueryParams;
};

class RECModule : public RFModuleConfigure<RECModule, 128> {
public:
  RECModule(uint32_t slot, const std::vector<SlotData> &slotData, QueryParams queryParams)
      : mSlot(slot), mSlotData(slotData), mQueryParams(queryParams) {}

  void configure_impl() override {
    auto &fileManager = CFGFileManager::GetInstance();

    for (const auto &data : mSlotData) {
      if (data.type != RFType_E::REC)
        continue;

      const auto &module = data.moduleInfo;
      std::string filename = "REC" + std::to_string(module.moduleID) + ".csv";
      auto parser = fileManager.GetParser("REC", filename);
      if (!parser) {
        throw std::runtime_error("Failed to get parser for REC" + std::to_string(module.moduleID));
      }

      parser->parse();
      DataQueryEngine engine(parser);
      FreqPowerQueryPolicy freqPowerQuery(mQueryParams.queryFreq, mQueryParams.queryPower);
      QueryResult result = engine.ExecuteQuery(freqPowerQuery);

      // 设置比特位（假设 REC 模块有自己的配置逻辑）
      // RECInner recInner;
      // recInner.SetConfig(result.GetMatchedRows());
      // config.bits |= recInner.GetBits();
    }
  }

  const Configuration<128> &GetConfiguration() const { return config; }

private:
  Configuration<128> config{};
  uint32_t mSlot;
  std::vector<SlotData> mSlotData;
  QueryParams mQueryParams;
};

// template <typename RFModule> class RFModuleFactory {
// public:
//   using CreatorFunc =
//       std::function<std::unique_ptr<RFModule>(const std::vector<unsigned int> &, double,
//       double)>;

//   // 静态注册模块
//   static void RegisterModule(const std::string &name, CreatorFunc creator) {
//     GetRegistry()[name] = std::move(creator);
//   }

//   // 静态创建模块实例
//   static std::unique_ptr<RFModule> CreateModule(const std::string &name,
//                                                 const std::vector<unsigned int> &ports, double
//                                                 freq, double power) {
//     const auto &registry = GetRegistry();
//     auto it = registry.find(name);
//     if (it != registry.end()) {
//       return it->second(ports, freq, power);
//     }
//     throw std::runtime_error("No creator registered for module: " + name);
//   }

// private:
//   // 获取静态注册表
//   static std::map<std::string, CreatorFunc> &GetRegistry() {
//     static std::map<std::string, CreatorFunc> registry;
//     return registry;
//   }
// };

#endif // RFMODULECONFIGURE_H