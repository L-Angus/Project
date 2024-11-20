#ifndef DAT_CONVERTOR_HPP
#define DAT_CONVERTOR_HPP

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

typedef char CHAR;

// 定义I/Q结构体，用于存储I和Q的数据
struct IQData {
  int16_t I1;
  int16_t I2;
  int16_t Q1;
  int16_t Q2;
};

// 每行存储4个通道
using ChannelData = std::array<IQData, 4>;

// 转换函数，将std::vector<CHAR>转换为std::vector<int64_t>
void ConvertTo64BitData(const std::vector<CHAR> &rawData,
                        std::vector<int64_t> &bit64ChannelData) {
  if (rawData.size() % 8 != 0) {
    throw std::runtime_error("原始数据大小不是8的倍数,无法转换为int64_t类型");
  }

  bit64ChannelData.reserve(rawData.size() / 8);

  for (size_t i = 0; i < rawData.size(); i += 8) {
    int64_t value = 0;
    for (size_t j = 0; j < 8; ++j) {
      value |= static_cast<int64_t>(static_cast<unsigned char>(rawData[i + j]))
               << (8 * j);
    }
    bit64ChannelData.push_back(value);
  }
}

// 重新排序通道和通道内数据的函数

void ReorderChannelsAndData(
    const std::vector<int64_t> &rawData,
    std::map<unsigned int, std::vector<int16_t>> &mChannelData) {
  //   std::vector<ChannelData> reorderedData;

  // 每行包含4个int64_t，总计256位
  constexpr size_t ChannelsPerLine = 4;
  constexpr size_t BitsPerLine = 256;
  constexpr size_t Int64sPerLine = BitsPerLine / 64;

  if (rawData.size() % Int64sPerLine != 0) {
    throw std::runtime_error("原始数据大小不符合每行的完整倍数");
  }

  for (size_t i = 0; i < rawData.size(); i += Int64sPerLine) {

    // 当前原始顺序：2-1-4-3
    // 目标顺序：1-2-3-4
    // 通道偏移索引
    size_t channelOffsets[ChannelsPerLine] = {1, 0, 3, 2};

    for (size_t channel = 0; channel < ChannelsPerLine; ++channel) {
      size_t offset = i + channelOffsets[channel];
      int64_t channelData = rawData[offset];

      // 拆解每个channel中的数据，当前顺序为：Q2, Q1, I2, I1
      int16_t Q2 = static_cast<int16_t>(channelData & 0xFFFF);
      int16_t Q1 = static_cast<int16_t>((channelData >> 16) & 0xFFFF);
      int16_t I2 = static_cast<int16_t>((channelData >> 32) & 0xFFFF);
      int16_t I1 = static_cast<int16_t>((channelData >> 48) & 0xFFFF);

      // 重新排列为：I1, I2, Q1, Q2
      mChannelData[channel].emplace_back(I1);
      mChannelData[channel].emplace_back(I2);
      mChannelData[channel].emplace_back(Q1);
      mChannelData[channel].emplace_back(Q2);
    }
  }
}

#endif // DAT_CONVERTOR_HPP