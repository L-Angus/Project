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
#include "DataFittingManager.hpp"
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

class FEModule : public RFModuleConfigure<FEModule, 256> {
public:
  FEModule(uint32_t slot, const std::vector<SlotData> &slotData, QueryParams queryParams)
      : mSlot(slot), mSlotData(slotData), mQueryParams(queryParams),
        m_fittingStrategy(std::make_shared<FreqPowerFittingStrategy>()) {}

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
      if (result.IsEmpty()) {
        // 2. 若未找到，则调用拟合策略
        if (m_fittingStrategy) {
          FittingParams fittingParams(mQueryParams.queryFreq, mQueryParams.queryPower);
          auto newRowOpt = m_fittingStrategy->DoFittingRow(engine, fittingParams);
          if (newRowOpt.has_value()) {
            // 3. 写回
            auto newRow = newRowOpt.value();
            /** 覆盖1行或者多行的写回 */
            bool success = engine.AddFittedRows({newRow});
            if (!success) {
              throw std::runtime_error("Failed to add fitted rows");
            } else {
              std::cout << "[FEModule] Insert row success: ";
              for (auto &cell : newRow) {
                std::cout << cell << " ";
              }
              std::cout << std::endl;
            }

            // 4. 可选：做一次验收
            bool validated = engine.VerifyRowExists([&](const auto &dataContainer) {
              for (const auto &row : dataContainer) {
                if (row[0] == std::to_string(mQueryParams.queryFreq) &&
                    row[1] == std::to_string(mQueryParams.queryPower)) {
                  return true;
                }
              }
              return false;
            });
            if (!validated) {
              throw std::runtime_error(
                  "[FEModule] Inserted row not found in data container. Possibly an error.");
            }
          }
        }
      }
    }

    // // 模拟通过 FEInner 设置比特位
    // FEInner feInner;
    // feInner.SetPort({*data.portNo}); // 使用 portNo
    // config.bits |= feInner.GetPort();
  }

  const Configuration<256> &GetConfiguration() const { return config; }

private:
  Configuration<256> config{};
  uint32_t mSlot;
  std::vector<SlotData> mSlotData;
  QueryParams mQueryParams;
  std::shared_ptr<IFittingStrategy> m_fittingStrategy = nullptr;
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

#endif // RFMODULECONFIGURE_H