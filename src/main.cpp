#include "./RawDataManager/DatConvertor.hpp"
#include "ConfigureManager.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

int main(int argc, char *argv[]) {

  std::cout << "___________ Convertor ___________" << std::endl;
  std::string file{argv[1]};
  std::cout << "file: " << file << std::endl;
  std::ifstream in(file, std::ios::binary);
  if (!in.good()) {
    throw std::runtime_error("INvalid filename: " + file);
  }
  in.seekg(0, std::ios::end);
  std::streamsize size = in.tellg();
  in.seekg(0, std::ios::beg);
  std::cout << "fileSize: " << size << std::endl;
  // 创建一个 vector 来缓存数据
  std::vector<CHAR> buffer(size);

  // 读取文件内容到 vector 中
  if (!in.read(buffer.data(), size)) {
    std::cerr << "读取文件失败: " << file << std::endl;
    return 1;
  }
  in.close();
  try {
    // 第一步：将CHAR类型转换为int64_t类型
    std::vector<int64_t> convertedData;
    ConvertTo64BitData(buffer, convertedData);

    // 第二步：重排通道数据并重新排序通道内数据
    std::map<unsigned int, std::vector<int16_t>> reorderedData;
    ReorderChannelsAndData(convertedData, reorderedData);

    unsigned int activeChannel = 0;

    for (const auto &[channel, rawdata] : reorderedData) {
      if (channel != activeChannel)
        continue;
      std::ofstream out(std::to_string(channel) + "demo.csv");
      if (!out.good()) {
        throw std::runtime_error("INvalid");
      }
      for (size_t i = 0; i < rawdata.size(); i += 4) {
        out << rawdata[i] << "," << rawdata[i + 2] << "\n"
            << rawdata[i + 1] << "," << rawdata[i + 3];
        out << std::endl;
      }
      out.close();
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  return 0;
}
