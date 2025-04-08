#ifndef __TX_MODULE_REG_HPP__
#define __TX_MODULE_REG_HPP__

#include "Com_defs.h"
#include "NonCopyable.h"
#include <iomanip>
#include <iostream>
#include <sstream>

class PLLRegConfig : public NonCopyable {
public:
  // PLL源枚举
  enum class PLLSource : uint8_t {
    PLL_DOWN_CONVERSION = 0x01, // PLL下变频
    PLL_UP_CONVERSION = 0x02,   // PLL上变频
    PLL_DUAL_TONE = 0x03        // PLL双音中的第二音
  };
  // PLL配置结构体
  struct PLLConfig {
    double freq;   // 40位
    PLLSource src; // 8位
  };
  PLLRegConfig() : reg_config_() {
    reg_config_.pll_0x5a_ = 0x5A;
    reg_config_.pll_0xa5_ = 0xA5;
    std::cout << "PLLRegConfig created" << std::endl;
  }

  void operator()(double freq, PLLSource src = PLLSource::PLL_DOWN_CONVERSION, FreqUnit unit = FreqUnit::Hz) {
    SetPLLSrc(PLLSource::PLL_DOWN_CONVERSION);
    SetPLLFreq(freq);
  }

  void Reset() { reg_config_ = {}; }

  void SetPLLFreq(double freq, FreqUnit unit = FreqUnit::Hz) {
    double freq_hz = freq;
    switch (unit) {
    case FreqUnit::KHz:
      freq_hz *= 1e3;
      break;
    case FreqUnit::MHz:
      freq_hz *= 1e6;
      break;
    case FreqUnit::GHz:
      freq_hz *= 1e9;
      break;
    default:
      break;
    }
    uint64_t freq_value = static_cast<uint64_t>(freq_hz);
    reg_config_.pll_freq_ = freq_value & 0xFFFFFFFFFFULL; // 40位
  }

  void SetPLLSrc(PLLSource src) {
    reg_config_.pll_src_ = static_cast<uint8_t>(src); // 8位
  }

  // 整体配置接口
  void ConfigPLL(const PLLConfig &config) {
    SetPLLSrc(config.src);
    SetPLLFreq(config.freq);
  }

  // 获取配置
  PLLConfig GetPLLConfig() const {
    return PLLConfig{.freq = static_cast<double>(reg_config_.pll_freq_),
                     .src = static_cast<PLLSource>(reg_config_.pll_src_)};
  }
  // 获取十六进制字符串表示
  std::string ToHex() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase;
    for (int i = 3; i >= 0; --i) {
      uint32_t value = (reg_config_bits_ >> (i * 32)).to_ulong();
      ss << std::setw(8) << std::setfill('0') << value;
    }
    return ss.str();
  }
  const Bit128 &GetRegConfig() const { return reg_config_bits_; }

private:
  union {
    PLL_REG_CONFIG reg_config_;
    Bit128 reg_config_bits_;
  };
};

class FERegConfig : public NonCopyable {
public:
  // 通道枚举
  enum class Channel : uint8_t { CH1 = 0, CH2 = 1, CH3 = 2, CH4 = 3, MAX = 4 };

  // 通道模式枚举
  enum class ChannelMode : uint8_t { MODE0 = 0, MODE1 = 1, MODE2 = 2, MODE3 = 3 };

  // 通道配置结构体
  struct ChannelConfig {
    struct {
      uint8_t mode;      // 2位
      uint8_t work_mode; // 4位
    } mode_config;

    struct {
      uint8_t att1_1; // 6位
      uint8_t att1_2; // 6位
    } att1;

    struct {
      uint8_t att3_1; // 6位
      uint8_t att3_2; // 6位
      uint8_t att3_3; // 6位
      uint8_t att3_4; // 6位
    } att3;
  };

  FERegConfig() { Reset(); }
  void Reset() { reg_config_ = {}; }

  // IO端口控制
  void SetPortEnable(uint8_t port_idx, bool enable) {
    if (port_idx < 24) {
      if (enable) {
        reg_config_.io_enable.ports |= (1U << port_idx);
      } else {
        reg_config_.io_enable.ports &= ~(1U << port_idx);
      }
    }
  }

  void SetPorts(const std::vector<uint8_t> &ports) {
    uint32_t port_mask = 0;
    for (const auto &port_idx : ports) {
      if (port_idx < 24) {
        port_mask |= (1U << port_idx);
      }
    }
    reg_config_.io_enable.ports = port_mask & 0xFFFFFF;
  }

  // 组控制
  void SetGroupEnable(uint8_t group_idx, bool enable) {
    if (group_idx < 8) {
      if (enable) {
        reg_config_.io_enable.groups |= (1U << group_idx);
      } else {
        reg_config_.io_enable.groups &= ~(1U << group_idx);
      }
    }
  }

  void SetGroups(const std::vector<uint8_t> &groups) {
    uint8_t group_mask = 0;
    for (const auto &group_idx : groups) {
      if (group_idx < 8) {
        group_mask |= (1U << group_idx);
      }
    }
    reg_config_.io_enable.groups = group_mask & 0xFF;
  }

  // 通道模式控制
  void SetChannelMode(Channel ch, ChannelMode mode) {
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      reg_config_.ch_mode[static_cast<uint8_t>(ch)].mode = static_cast<uint8_t>(mode) & 0x3;
    }
  }

  void SetChannelWorkMode(Channel ch, uint8_t work_mode) {
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      reg_config_.ch_mode[static_cast<uint8_t>(ch)].work_mode = work_mode & 0xF;
    }
  }

  // 通道衰减控制
  void SetChannelAtt1(Channel ch, uint8_t att1_1, uint8_t att1_2) {
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      reg_config_.ch[static_cast<uint8_t>(ch)].att1.att1_1 = att1_1 & 0x3F;
      reg_config_.ch[static_cast<uint8_t>(ch)].att1.att1_2 = att1_2 & 0x3F;
    }
  }

  void SetChannelAtt3(Channel ch, uint8_t att3_1, uint8_t att3_2, uint8_t att3_3, uint8_t att3_4) {
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      auto &att3 = reg_config_.ch[static_cast<uint8_t>(ch)].att3;
      att3.att3_1 = att3_1 & 0x3F;
      att3.att3_2 = att3_2 & 0x3F;
      att3.att3_3 = att3_3 & 0x3F;
      att3.att3_4 = att3_4 & 0x3F;
    }
  }

  // 通道整体配置
  void ConfigChannel(Channel ch, const ChannelConfig &config) {
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      SetChannelMode(ch, static_cast<ChannelMode>(config.mode_config.mode));
      SetChannelWorkMode(ch, config.mode_config.work_mode);
      SetChannelAtt1(ch, config.att1.att1_1, config.att1.att1_2);
      SetChannelAtt3(ch, config.att3.att3_1, config.att3.att3_2, config.att3.att3_3, config.att3.att3_4);
    }
  }

  // 获取配置
  ChannelConfig GetChannelConfig(Channel ch) const {
    ChannelConfig config{};
    if (static_cast<uint8_t>(ch) < static_cast<uint8_t>(Channel::MAX)) {
      const auto idx = static_cast<uint8_t>(ch);
      config.mode_config.mode = reg_config_.ch_mode[idx].mode;
      config.mode_config.work_mode = reg_config_.ch_mode[idx].work_mode;
      config.att1.att1_1 = reg_config_.ch[idx].att1.att1_1;
      config.att1.att1_2 = reg_config_.ch[idx].att1.att1_2;
      config.att3.att3_1 = reg_config_.ch[idx].att3.att3_1;
      config.att3.att3_2 = reg_config_.ch[idx].att3.att3_2;
      config.att3.att3_3 = reg_config_.ch[idx].att3.att3_3;
      config.att3.att3_4 = reg_config_.ch[idx].att3.att3_4;
    }
    return config;
  }

  const Bit256 &GetRegConfig() const { return reg_config_bits_; }

private:
  union {
    FE_REG_CONFIG reg_config_;
    Bit256 reg_config_bits_;
  };
};

class MODRegConfig : public NonCopyable {
public:
  // 控制配置结构体
  struct ControlConfig {
    bool power_enable;     // 1位
    bool quad_freq_enable; // 1位
    bool e2prom_enable;    // 1位
  };

  // 通道配置结构体
  struct ChannelConfig {
    uint8_t sw_stl; // 5位
    uint8_t att;    // 4位
  };

  MODRegConfig() { Reset(); }
  void Reset() { reg_config_ = {}; }

  // 控制配置
  void SetControl(const ControlConfig &config) {
    reg_config_.control.power_enable = config.power_enable;
    reg_config_.control.quad_freq_enable = config.quad_freq_enable;
    reg_config_.control.e2prom_enable = config.e2prom_enable;
  }

  // 通道配置
  void SetChannel(uint8_t ch_idx, const ChannelConfig &config) {
    if (ch_idx < 2) {
      reg_config_.ch[ch_idx].sw_stl = config.sw_stl & 0x1F; // 5位
      reg_config_.ch[ch_idx].att = config.att & 0xF;        // 4位
    }
  }

  const Bit128 &GetRegConfig() const { return reg_config_bits_; }

private:
  union {
    MOD_REG_CONFIG reg_config_;
    Bit128 reg_config_bits_;
  };
};

class DACCFG4RegConfig : public NonCopyable {
public:
  // RF控制配置
  struct RFConfig {
    bool rf1;
    bool rf2;
  };

  DACCFG4RegConfig() { Reset(); }
  void Reset() { reg_config_ = {}; }

  void SetRFControl(const RFConfig &config) {
    reg_config_.rf_control.rf1 = config.rf1;
    reg_config_.rf_control.rf2 = config.rf2;
  }

  void SetADCMuxSel(uint8_t sel) {
    reg_config_.adc_mux_sel = sel & 0x1F; // 5位
  }

  void SetADCEnable(bool enable) { reg_config_.adc_enable = enable; }

  const Bit128 &GetRegConfig() const { return reg_config_bits_; }

private:
  union {
    DACCFG4_REG_CONFIG reg_config_;
    Bit128 reg_config_bits_;
  };
};

class SGC1RegConfig : public NonCopyable {
public:
  // 电源控制配置
  struct PowerConfig {
    bool power_5v;
    bool ampctl_5v;
    bool power_12v;
    bool ampctl_12v;
    bool e2prom;
  };

  // 开关控制配置
  struct SwitchConfig {
    uint8_t sw12; // 2位
    uint8_t sw34; // 2位
    bool alc;
  };

  // 衰减控制配置
  struct AttConfig {
    uint8_t att1; // 6位
    uint8_t att2; // 6位
    uint8_t att3; // 4位
  };

  SGC1RegConfig() { Reset(); }
  void Reset() { reg_config_ = {}; }

  void SetPowerControl(const PowerConfig &config) {
    reg_config_.power_control.power_5v = config.power_5v;
    reg_config_.power_control.ampctl_5v = config.ampctl_5v;
    reg_config_.power_control.power_12v = config.power_12v;
    reg_config_.power_control.ampctl_12v = config.ampctl_12v;
    reg_config_.power_control.e2prom = config.e2prom;
  }

  void SetSwitchControl(const SwitchConfig &config) {
    reg_config_.switch_control.sw12 = config.sw12 & 0x3; // 2位
    reg_config_.switch_control.sw34 = config.sw34 & 0x3; // 2位
    reg_config_.switch_control.alc = config.alc;
  }

  void SetAttControl(const AttConfig &config) {
    reg_config_.att_control.att1 = config.att1 & 0x3F; // 6位
    reg_config_.att_control.att2 = config.att2 & 0x3F; // 6位
    reg_config_.att_control.att3 = config.att3 & 0xF;  // 4位
  }

  const Bit128 &GetRegConfig() const { return reg_config_bits_; }

private:
  union {
    SGC1_REG_CONFIG reg_config_;
    Bit128 reg_config_bits_;
  };
};

#endif