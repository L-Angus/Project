#ifndef CONFIGUREMANAGER_HPP
#define CONFIGUREMANAGER_HPP

#include <bitset>
#include <iostream>
#include <string>

template <size_t N> struct Configuration {
  std::string moduleName;
  std::bitset<N> bits{};
};

using Bit256 = Configuration<256>;
using Bit128 = Configuration<128>;
using Bit64 = Configuration<64>;
using Bit32 = Configuration<32>;

template <typename RFModule, size_t N = 128> class RFModuleConfigureBase {
public:
  void Configure(const Configuration<N> &config) {
    static_cast<RFModule &>(*this).configure_impl(config);
  }

protected:
  virtual void configure_impl(const Configuration<N> &) = 0;
};

namespace LDModule {
class PLLFreConfigure : public RFModuleConfigureBase<PLLFreConfigure> {
public:
  void configure_impl(const Bit128 &config) {
    std::cout << "ModuleName: " << config.moduleName << std::endl;
    std::cout << config.bits << std::endl;
  }
};

class ModConfigure : public RFModuleConfigureBase<ModConfigure> {
public:
  void configure_impl(const Bit128 &config) {
    std::cout << "ModuleName: " << config.moduleName << std::endl;
    std::cout << config.bits << std::endl;
  }
};

class DUTClkConfigure : public RFModuleConfigureBase<DUTClkConfigure> {
public:
  void configure_impl(const Bit128 &config) {
    std::cout << "ModuleName: " << config.moduleName << std::endl;
    std::cout << config.bits << std::endl;
  }
};
}; // namespace LDModule

struct RERegConfig {
  USHORT usSlot;
  BoardType bType;
  RFType_E eType;
  UINT uiLen;
  UINT uiOffset;
  std::vector<UCHAR> uiValue;
};

#endif