#ifndef DAT_CONVERTOR_HPP
#define DAT_CONVERTOR_HPP

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

// 自定义类型定义
typedef int CHK_RET;
#define ERR_SUCCEED 0

// 字符类型定义
typedef char CHAR;

// 输入输出流类型定义
template <typename T> class STREAM_OUT {
public:
  STREAM_OUT &operator>>(T &value) {
    // 实现具体的反序列化逻辑
    return *this;
  }

  STREAM_OUT &operator>>(size_t &value) {
    // 实现size_t类型的读取
    return *this;
  }

  STREAM_OUT &operator>>(std::map<unsigned int, std::vector<unsigned int>> &value) {
    // 实现map类型的读取
    return *this;
  }
};

template <typename T> class STREAM_IN {
public:
  STREAM_IN &operator<<(const T &value);
};

// 硬件相关定义
#define IN
#define OUT
#define DDRA 0
enum VP_TYPE_E { VPTYPE_RC_CVP };

// 硬件访问函数声明
CHK_RET DRV_MVPMemExtRead(VP_TYPE_E type, int addr, int offset, size_t readSize, size_t actualSize,
                          void *buffer);

// 序列化结构体
struct SerializeBody {
  size_t samplecount;
  std::vector<CHAR> rawData;
};

// 定义I/Q结构体，用于存储I和Q的数据
struct IQData {
  int16_t I1;
  int16_t I2;
  int16_t Q1;
  int16_t Q2;
};

// 每行存储4个通道
using ChannelData = std::array<IQData, 4>;

using Bit16ChannelData = std::vector<int16_t>;
using Bit64ChannelData = std::vector<int64_t>;
using ChannelDataMap = std::map<unsigned int, Bit16ChannelData>;

// 转换函数，将std::vector<CHAR>转换为std::vector<int64_t>
void ConvertTo64BitData(const std::vector<CHAR> &rawData, Bit64ChannelData &bit64ChannelData) {
  if (rawData.size() % 8 != 0) {
    throw std::runtime_error("原始数据大小不是8的倍数,无法转换为int64_t类型");
  }

  bit64ChannelData.reserve(rawData.size() / 8);

  for (size_t i = 0; i < rawData.size(); i += 8) {
    int64_t value = 0;
    for (size_t j = 0; j < 8; ++j) {
      value |= static_cast<int64_t>(static_cast<unsigned char>(rawData[i + j])) << (8 * j);
    }
    bit64ChannelData.push_back(value);
  }
}

// 重新排序通道和通道内数据的函数
void ReorderChannelsAndData(const Bit64ChannelData &rawData, ChannelDataMap &mChannelData,
                            size_t samplecount) {
  constexpr size_t ChannelsPerLine = 4;
  constexpr size_t BitsPerLine = 256;
  constexpr size_t Int64sPerLine = BitsPerLine / 64;

  if (rawData.size() % Int64sPerLine != 0) {
    throw std::runtime_error("原始数据大小不符合每行的完整倍数");
  }

  // 计算每个通道需要保留的样本数
  const size_t samplesPerChannel = samplecount / ChannelsPerLine;

  for (size_t i = 0; i < rawData.size(); i += Int64sPerLine) {
    size_t channelOffsets[ChannelsPerLine] = {1, 0, 3, 2};

    for (size_t channel = 0; channel < ChannelsPerLine; ++channel) {
      size_t offset = i + channelOffsets[channel];
      int64_t channelData = rawData[offset];

      int16_t Q2 = static_cast<int16_t>(channelData & 0xFFFF);
      int16_t Q1 = static_cast<int16_t>((channelData >> 16) & 0xFFFF);
      int16_t I2 = static_cast<int16_t>((channelData >> 32) & 0xFFFF);
      int16_t I1 = static_cast<int16_t>((channelData >> 48) & 0xFFFF);

      mChannelData[channel].emplace_back(I1);
      mChannelData[channel].emplace_back(Q1);
      mChannelData[channel].emplace_back(I2);
      mChannelData[channel].emplace_back(Q2);
    }
  }

  // 截取每个通道的多余数据
  for (auto &[channel, data] : mChannelData) {
    if (data.size() > samplesPerChannel) {
      data.resize(samplesPerChannel);
    }
  }
}

void FilterInvalidChannelData(
    const ChannelDataMap &totalChannelDataMap,
    const std::map<unsigned int, std::vector<unsigned int>> &activeChannels,
    ChannelDataMap &filteredChannelData) {
  for (auto it = totalChannelDataMap.begin(); it != totalChannelDataMap.end(); ++it) {
    if (activeChannels.find(it->first) != activeChannels.end()) {
      auto key = activeChannels.at(it->first)[0];
      filteredChannelData[key] = std::move(it->second);
    }
  }
}

void GetActiveChannelData(const std::vector<CHAR> &rawData,
                          const std::map<unsigned int, std::vector<unsigned int>> &activeChannels,
                          ChannelDataMap &activeChannelData, const SerializeBody &rx) {
  Bit64ChannelData convertedChannelData;
  ConvertTo64BitData(rawData, convertedChannelData);
  ChannelDataMap totalChannelDataMap;
  ReorderChannelsAndData(convertedChannelData, totalChannelDataMap, rx.samplecount);
  FilterInvalidChannelData(totalChannelDataMap, activeChannels, activeChannelData);
}

CHK_RET GetRawData(IN STREAM_OUT<SerializeBody> &tInStream,
                   OUT STREAM_IN<ChannelDataMap> &tOutStream) {
  CHK_RET iRet = ERR_SUCCEED;
  SerializeBody rx;
  std::map<unsigned int, std::vector<unsigned int>> adcChannels;
  tInStream >> rx.samplecount >> adcChannels;

  const size_t maxReadSize = 65536 * 16;
  size_t bufferSize = rx.samplecount * 16;
  size_t alignedBufferSize = (bufferSize + 63) & ~size_t(63);
  size_t offset = 0;
  size_t remainToRead = alignedBufferSize;

  std::vector<CHAR> totalBuffer(alignedBufferSize);
  while (remainToRead > 0) {
    size_t readSize = std::min(remainToRead, maxReadSize);
    iRet = DRV_MVPMemExtIrpRead(VP_TYPE_E::VPTYPE_RC_CVP, DDRA, 0x0BB00000 + offset, readSize,
                                readSize, totalBuffer.data() + offset);
    if (iRet != ERR_SUCCEED) {
      break;
    }
    offset += readSize;
    remainToRead -= readSize;
  }

  const size_t extraBytes = alignedBufferSize - bufferSize;
  const size_t bytesPerChannel = extraBytes / 4;

  std::vector<CHAR> trimmedBuffer;
  trimmedBuffer.reserve(bufferSize);

  const size_t channelBlockSize = 16;
  size_t totalBlocks = alignedBufferSize / channelBlockSize;

  for (size_t block = 0; block < totalBlocks; ++block) {
    size_t start = block * channelBlockSize;
    size_t end = start + channelBlockSize - bytesPerChannel;

    trimmedBuffer.insert(trimmedBuffer.end(), totalBuffer.begin() + start,
                         totalBuffer.begin() + end);
  }

  rx.rawData = std::move(trimmedBuffer);
  try {
    ChannelDataMap activeChannelData;
    GetActiveChannelData(rx.rawData, adcChannels, activeChannelData, rx);
    tOutStream << activeChannelData;
  } catch (const std::exception &e) {
    throw std::runtime_error("转换数据失败");
  }
}

#endif // DAT_CONVERTOR_HPP
