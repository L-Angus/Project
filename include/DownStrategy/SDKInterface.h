#ifndef __SDKINTERFACE_HPP__
#define __SDKINTERFACE_HPP__

#include <bitset>
#include <iostream>
#include <map>
#include <memory>

#include "Com_defs.h"
#include "RegCacheBase.h"
#include "RegTransaction.h"

class RFStimInterface {
public:
  struct ModuleGroup {
    BoardType bType;
    StimMode sMode;
    std::vector<ModuleType> modules;
  };

  RFStimInterface(BoardType board, StimMode mode)
      : board_type_(board), stim_mode_(mode), transaction_(std::make_unique<RegTransaction>()) {
    InitializeTransactionSet();
  }

  // 修改Execute函数
  void SetCondition(const QueryCondition &condition) {
    if (board_type_ == BoardType::LD) {
      if (stim_mode_ == StimMode::CW_MODE) {
        ConfigureModule<PLLRegConfig>(ModuleType::PLL1, [&condition](PLLRegConfig &config) {
          config.SetPLLFreq(condition.freq, FreqUnit::MHz);
          config.SetPLLSrc(PLLRegConfig::PLLSource::PLL_UP_CONVERSION);
        });
      }
    }
  }
  bool Execute() {}

private:
  void InitializeTransactionSet() {
    std::cout << "Initializing transaction set..." << std::endl;
    // LD板模块
    transaction_->AddModule(ModuleType::PLL1);
    transaction_->AddModule(ModuleType::PLL2);
    transaction_->AddModule(ModuleType::DACCFG3);
    transaction_->AddModule(ModuleType::DACCFG4);
    transaction_->AddModule(ModuleType::MOD);

    // LM板模块
    transaction_->AddModule(ModuleType::FE);
    transaction_->AddModule(ModuleType::SGC1);
    transaction_->AddModule(ModuleType::SGC2);
  }

  void InitializeModuleGroups() {
    module_groups_ = {
        {BoardType::LD, StimMode::CW_MODE, {ModuleType::PLL1, ModuleType::DACCFG4, ModuleType::MOD}},
        {BoardType::LD,
         StimMode::MOD_MODE,
         {ModuleType::PLL1, ModuleType::DACCFG3, ModuleType::DACCFG4, ModuleType::MOD}},
        {BoardType::LD, StimMode::DT_MODE, {ModuleType::PLL1, ModuleType::PLL2, ModuleType::DACCFG4, ModuleType::MOD}},
        {BoardType::LM, StimMode::CW_MODE, {ModuleType::FE}},
        {BoardType::LM, StimMode::DT_MODE, {ModuleType::SGC1, ModuleType::SGC2, ModuleType::FE}},
        {BoardType::LM, StimMode::MOD_MODE, {ModuleType::FE, ModuleType::MOD}}};
  }

  bool IsModuleUpdated(ModuleType module) const {
    switch (module) {
    case ModuleType::PLL1:
      return PLLRegCache::Instance().IsUpdated();
    // ... 其他模块的更新状态检查
    default:
      return false;
    }
  }

  const Bit128 &GetModuleConfig(ModuleType module) const {
    switch (module) {
    case ModuleType::PLL1:
      return PLLRegCache::Instance().GetRegConfig();
    // ... 其他模块的配置获取
    default:
      throw std::runtime_error("Unknown module type");
    }
  }

private:
  BoardType board_type_;
  StimMode stim_mode_;
  std::unique_ptr<RegTransaction> transaction_ = nullptr;
  std::vector<ModuleGroup> module_groups_;

  bool WriteToHardware(ModuleType module, const Bit128 &reg) {
    try {
      std::cout << "Writing to hardware for: " << reg.to_string() << std::endl;
      return true;
    } catch (const std::exception &e) {
      std::cerr << "Hardware write failed for " << RegTransaction::ModuleToString(module) << ": " << e.what()
                << std::endl;
      return false;
    }
  }

  // 添加清除更新标志的辅助函数
  void ClearModuleUpdateFlag(ModuleType module) {
    switch (module) {
    case ModuleType::PLL1:
      PLLRegCache::Instance().SetUpdated(false);
      break;
    // case ModuleType::MOD:
    //   MODRegCache::Instance().SetUpdated(false);
    //   break;
    // case ModuleType::FE:
    //   FERegCache::Instance().SetUpdated(false);
    //   break;
    // case ModuleType::SGC1:
    //   SGC1RegCache::Instance().SetUpdated(false);
    //   break;
    default:
      break;
    }
  }

  template <typename RegConfig>
  bool ConfigureModule(ModuleType module, const std::function<void(RegConfig &)> &configFunc) {
    try {
      RegConfig config;
      if (!transaction_->Begin(module, config)) {
        std::cerr << "Begin transaction failed for " << RegTransaction::ModuleToString(module) << std::endl;
        return false;
      }
      configFunc(config);
      if (!transaction_->Commit(module, config)) {
        std::cerr << "Commit transaction failed for " << RegTransaction::ModuleToString(module) << std::endl;
        transaction_->Rollback(module);
        return false;
      }
      return true;
    } catch (const std::exception &e) {
      std::cerr << "Exception in ConfigureModule: " << e.what() << std::endl;
      return false;
    }
  }
};

#endif
