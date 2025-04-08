#ifndef __REGCACHEBASE_HPP__
#define __REGCACHEBASE_HPP__

#include <bitset>

template <typename Derived, size_t N> class RegCacheBase {
public:
  static Derived &Instance() {
    static Derived instance;
    return instance;
  }
  const std::bitset<N> &GetRegConfig() const { return reg_config_; }
  const std::bitset<N> &GetBackupConfig() const { return backup_config_; }
  void UpdateRegConfig(const std::bitset<N> &config) {
    Backup();
    static_cast<Derived *>(this)->UpdateRegConfigImpl(config);
    is_updated_ = true;
  }
  bool IsUpdated() const { return is_updated_; }
  void SetUpdated(bool updated) { is_updated_ = updated; }
  void Backup() { backup_config_ = reg_config_; }
  void Restore() {
    reg_config_ = backup_config_;
    is_updated_ = false;
  }

  void Reset() {
    reg_config_.reset();
    is_updated_ = false;
  }

protected:
  RegCacheBase() = default;
  ~RegCacheBase() = default;

  std::bitset<N> reg_config_;
  std::bitset<N> backup_config_;
  bool is_updated_{false};

private:
  RegCacheBase(const RegCacheBase &) = delete;
  RegCacheBase &operator=(const RegCacheBase &) = delete;
  RegCacheBase(RegCacheBase &&) = delete;            // 禁止移动
  RegCacheBase &operator=(RegCacheBase &&) = delete; // 禁止移动赋值
};

class PLLRegCache : public RegCacheBase<PLLRegCache, 128> {
  friend class RegCacheBase<PLLRegCache, 128>;

private:
  PLLRegCache() = default;

  void UpdateRegConfigImpl(const std::bitset<128> &config) {
    reg_config_ |= config; // 直接使用位或运算更新
  }
};
#endif