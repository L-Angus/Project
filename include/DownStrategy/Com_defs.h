#ifndef __COM_DEFS_HPP__
#define __COM_DEFS_HPP__

#include <bitset>
#include <cstdint>

struct PLL_REG_CONFIG {
  unsigned int pll_0x5a_ : 8;
  uint64_t pll_freq_ : 40;
  unsigned int pll_src_ : 8;
  unsigned int pll_0xa5_ : 8;
  unsigned int : 64;
};

struct MOD_REG_CONFIG {
  struct {
    unsigned int power_enable : 1;
    unsigned int quad_freq_enable : 1;
    unsigned int e2prom_enable : 1;
    unsigned int : 13;
  } control;

  struct {
    unsigned int sw_stl : 5;
    unsigned int : 3;
    unsigned int att : 4;
    unsigned int : 4;
  } ch[2]; // ch1和ch2的配置

  unsigned int : 84;
};

struct DACCFG4_REG_CONFIG {
  struct {
    unsigned int rf1 : 1;
    unsigned int rf2 : 1;
    unsigned int : 2;
  } rf_control;

  unsigned int adc_mux_sel : 5;
  unsigned int : 7;
  unsigned int adc_enable : 1;
  unsigned int : 16;
};

struct SGC1_REG_CONFIG {
  struct {
    unsigned int power_5v : 1;
    unsigned int ampctl_5v : 1;
    unsigned int power_12v : 1;
    unsigned int ampctl_12v : 1;
    unsigned int e2prom : 1;
    unsigned int : 11;
  } power_control;

  struct {
    unsigned int sw12 : 2;
    unsigned int sw34 : 2;
    unsigned int : 4;
    unsigned int alc : 1;
    unsigned int : 7;
  } switch_control;

  struct {
    unsigned int att1 : 6;
    unsigned int : 2;
    unsigned int att2 : 6;
    unsigned int : 2;
    unsigned int att3 : 4;
  } att_control;

  unsigned int : 76;
};

struct FE_REG_CONFIG {
  struct {
    unsigned int ports : 24;
    unsigned int groups : 8;
  } io_enable;

  struct {
    unsigned int mode : 2;
    unsigned int work_mode : 4;
  } ch_mode[4]; // 4个通道的模式控制

  struct {
    struct {
      unsigned int att1_1 : 6;
      unsigned int att1_2 : 6;
      unsigned int : 4;
    } att1;

    struct {
      unsigned int att3_1 : 6;
      unsigned int att3_2 : 6;
      unsigned int att3_3 : 6;
      unsigned int att3_4 : 6;
      unsigned int : 4;
    } att3;
  } ch[4]; // 4个通道的衰减控制

  unsigned int : 16;
};

enum class FreqUnit {
  Hz,  // 赫兹
  KHz, // 千赫兹
  MHz, // 兆赫兹
  GHz  // 吉赫兹
};

enum class ModuleType { PLL1, PLL2, MOD, FE, SGC1, SGC2, DACCFG3, DACCFG4 };
enum class StimMode { CW_MODE, DT_MODE, MOD_MODE }; // 定义激励模式枚举类型
enum class BoardType { LD, LM };
struct RFRegConfig {
  unsigned short usSlot;
  BoardType bType;
  StimMode sMode;
  unsigned int uiLen;
  unsigned int uiOffset;
  std::vector<unsigned char> uiValue;
};

template <size_t N> using BitSetN = std::bitset<N>;
using Bit128 = BitSetN<128>;
using Bit256 = BitSetN<256>;

struct QueryCondition {
  float freq;
  float power;
  FreqUnit unit = FreqUnit::Hz;
};

#endif