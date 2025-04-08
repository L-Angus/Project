#ifndef __REG_TRANSACTION_HPP__
#define __REG_TRANSACTION_HPP__

#include "Com_defs.h"
#include "RegCacheBase.h"
#include "TX_Module_Reg.h"

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

// 寄存器基类
class RegBase {
public:
  virtual ~RegBase() = default;
};

// 寄存器包装模板类
template <typename T> class RegWrapper : public RegBase {
public:
  RegWrapper(T &reg) : reg_(reg) {}
  T &Get() { return reg_; }
  const T &Get() const { return reg_; }

private:
  T &reg_;
};

class RegTransaction {
public:
  // 使用模板定义回调类型
  using BeginCallback = std::function<void(ModuleType, RegBase &)>;
  using CommitCallback = std::function<void(ModuleType, const RegBase &)>;
  using RollbackCallback = std::function<void(ModuleType)>;

  bool AddModule(ModuleType module) {
    try {
      // 检查模块是否已经注册
      if (begin_callbacks_.find(module) != begin_callbacks_.end()) {
        last_error_ = "Module " + ModuleToString(module) + " already registered";
        return false;
      }

      // 设置默认回调
      // Begin回调：从Cache获取当前配置
      begin_callbacks_[module] = [this](ModuleType m, RegBase &reg) {
        std::cout << "Begin transaction for module: " << ModuleToString(m) << std::endl;

        // 根据模块类型获取对应的Cache实例
        if (m == ModuleType::PLL1) {
          if (auto *wrapper = dynamic_cast<RegWrapper<std::bitset<128>> *>(&reg)) {
            // 从Cache获取当前配置
            wrapper->Get() = PLLRegCache::Instance().GetRegConfig();
          }
        }
        // 其他模块类型的处理...
      };

      // Commit回调：更新Cache配置
      commit_callbacks_[module] = [this](ModuleType m, const RegBase &reg) {
        std::cout << "Commit transaction for module: " << ModuleToString(m) << std::endl;

        if (m == ModuleType::PLL1) {
          if (const auto *wrapper = dynamic_cast<const RegWrapper<std::bitset<128>> *>(&reg)) {
            // 更新Cache配置
            PLLRegCache::Instance().UpdateRegConfig(wrapper->Get());
          }
        }
        // 其他模块类型的处理...
      };

      // Rollback回调：重置Cache配置
      rollback_callbacks_[module] = [this](ModuleType m) {
        std::cout << "Rollback transaction for module: " << ModuleToString(m) << std::endl;

        if (m == ModuleType::PLL1) { 
          // 重置Cache配置
          PLLRegCache::Instance().Restore();
        }
        // 其他模块类型的处理...
      };
      return true;
    } catch (const std::exception &e) {
      last_error_ = "Failed to add module " + ModuleToString(module) + ": " + e.what();
      return false;
    }
  }

  // 设置回调
  bool SetBeginCallback(ModuleType module, BeginCallback cb) {
    if (!cb) {
      return false;
    }
    begin_callbacks_[module] = std::move(cb);
    return true;
  }

  bool SetCommitCallback(ModuleType module, CommitCallback cb) {
    if (!cb) {
      return false;
    }
    commit_callbacks_[module] = std::move(cb);
    return true;
  }

  bool SetRollbackCallback(ModuleType module, RollbackCallback cb) {
    if (!cb) {
      return false;
    }
    rollback_callbacks_[module] = std::move(cb);
    return true;
  }

  // 开始事务
  template <typename RegConfig> bool Begin(ModuleType module, RegConfig &config) {
    if (HasActiveTransaction(module)) {
      return false;
    }

    try {
      auto it = begin_callbacks_.find(module);
      if (it != begin_callbacks_.end()) {
        RegWrapper<decltype(config.GetRegConfig())> wrapper(config.GetRegConfig());
        it->second(module, wrapper);
        active_transactions_.insert(module);
        return true;
      }
      std::cerr << "Begin callback not found for module: " << ModuleToString(module) << std::endl;
    } catch (const std::exception &e) {
      last_error_ = e.what();
    }
    return false;
  }

  // 提交事务
  template <typename RegConfig> bool Commit(ModuleType module, const RegConfig &config) {
    if (!HasActiveTransaction(module)) {
      return false;
    }

    try {
      auto it = commit_callbacks_.find(module);
      if (it != commit_callbacks_.end()) {
        RegWrapper<decltype(config.GetRegConfig())> wrapper(
            const_cast<decltype(config.GetRegConfig()) &>(config.GetRegConfig()));
        it->second(module, wrapper);
        active_transactions_.erase(module);
        return true;
      }
      std::cerr << "Commit callback not found for module: " << ModuleToString(module) << std::endl;
    } catch (const std::exception &e) {
      last_error_ = e.what();
      Rollback(module);
    }
    return false;
  }

  // 回滚事务
  bool Rollback(ModuleType module) {
    if (!HasActiveTransaction(module)) {
      return false;
    }

    try {
      auto it = rollback_callbacks_.find(module);
      if (it != rollback_callbacks_.end()) {
        it->second(module);
        active_transactions_.erase(module);
        return true;
      }
      std::cerr << "Rollback callback not found for module: " << ModuleToString(module) << std::endl;
    } catch (const std::exception &e) {
      last_error_ = e.what();
    }
    return false;
  }

  // 回滚所有活动事务
  void RollbackAll() {
    std::vector<ModuleType> failed_rollbacks;
    for (const auto &module : active_transactions_) {
      if (!Rollback(module)) {
        failed_rollbacks.push_back(module);
      }
    }

    if (!failed_rollbacks.empty()) {
      last_error_ = "Failed to rollback modules: ";
      for (const auto &module : failed_rollbacks) {
        last_error_ += ModuleToString(module) + ", ";
      }
    }
  }

  // 获取最后的错误信息
  const std::string &GetLastError() const { return last_error_; }

  // 清除错误信息
  void ClearError() { last_error_.clear(); }

  // 检查事务状态
  bool HasActiveTransaction(ModuleType module) const { return active_transactions_.count(module) > 0; }

  // 获取活动事务数量
  size_t GetActiveTransactionCount() const { return active_transactions_.size(); }

  static std::string ModuleToString(ModuleType module) {
    switch (module) {
    case ModuleType::PLL1:
      return "PLL";
    case ModuleType::MOD:
      return "MOD";
    case ModuleType::FE:
      return "FE";
    case ModuleType::SGC1:
      return "SGC1";
    case ModuleType::DACCFG4:
      return "DACCFG4";
    default:
      return "Unknown";
    }
  }

private:
  std::unordered_map<ModuleType, BeginCallback> begin_callbacks_;
  std::unordered_map<ModuleType, CommitCallback> commit_callbacks_;
  std::unordered_map<ModuleType, RollbackCallback> rollback_callbacks_;
  std::unordered_set<ModuleType> active_transactions_;
  std::string last_error_;
};

#endif