#ifndef __COMMON__
#define __COMMON__

#include <map>
#include <string>
#include <vector>

typedef enum boardType { LD = 0, LM } BoardType;
typedef enum RFModuleType { FE = 0, REC, PLL, DUTClk, Mod, DAC, SGC, RF_TYPE_MAX } RFType_E;

struct RFRegConfig {
  unsigned short usSlot;
  BoardType bType;
  RFType_E eType;
  uint32_t uiLen;
  uint32_t uiOffset;
  std::vector<unsigned char> uiValue;
};

using SlotChannelMap = std::map<uint32_t, std::vector<uint32_t>>;

struct QueryParams {
  double queryFreq;
  double queryPower;
};

struct ModuleInfo {
  std::string moduleName;
  unsigned int moduleID;
};

struct SlotData {
  RFType_E type;                  // 模块类型 (FE, REC, etc.)
  ModuleInfo moduleInfo;          // 模块信息（名称和编号）
  std::optional<uint32_t> portNo; // 对 FE 模块有效的端口号
};

// template <typename BitsType> class RegConfigGenerator {
// public:
//   RegConfigGenerator(uint32_t slot, const BitsType &bits, uint32_t offset)
//       : m_slot(slot), m_bits(bits), m_offset(offset) {}

//   RFRegConfig Generate(RFType_E type) const {
//     RFRegConfig config;
//     config.usSlot = m_slot;
//     config.uiOffset = m_offset;
//     config.uiLen = m_bits.size() / 8;
//     config.uiValue.assign(m_bits.begin(), m_bits.end());
//     config.eType = type;
//     return config;
//   }

// private:
//   uint32_t m_slot;
//   BitsType m_bits;
//   uint32_t m_offset;
// };
template <typename BitsType> class RegConfigGenerator {
public:
  // 构造函数
  RegConfigGenerator(uint32_t slot, const BitsType &bits, uint32_t offset)
      : m_slot(slot), m_bits(bits), m_offset(offset) {}

  // 生成寄存器配置
  RFRegConfig Generate(RFType_E type) const {
    RFRegConfig config;
    config.usSlot = m_slot;               // 设置槽位号
    config.uiOffset = m_offset;           // 设置寄存器偏移
    config.uiLen = m_bits.size() / 8;     // 设置数据长度
    config.uiValue = BitsToBytes(m_bits); // 转换 bit 数据为字节数组
    config.eType = type;                  // 设置寄存器类型
    return config;
  }

private:
  uint32_t m_slot;        // 槽位号
  const BitsType &m_bits; // bit 数据
  uint32_t m_offset;      // 寄存器偏移

  // 将 bit 数据转换为字节数组
  static std::vector<uint8_t> BitsToBytes(const BitsType &bits) {
    std::vector<uint8_t> bytes((bits.size() + 7) / 8, 0); // 按字节分配
    for (size_t i = 0; i < bits.size(); ++i) {
      if (bits[i]) {
        bytes[i / 8] |= (1 << (i % 8)); // 设置相应的位
      }
    }
    return bytes;
  }
};

#endif // __COMMON__