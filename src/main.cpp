#include <fstream>
#include <iostream>
#include <map>
#include <vector>

// #include "RFStrategy/HardwareStrategy.h"
// #include "RFStrategy/RFModuleConfigure.h"

// 假设有这个函数，用于计算不小于x的下一个2的幂
size_t NextPowerOfTwo(size_t x) {
  if (x <= 1)
    return 1;
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  // 如果是64位需要进一步扩展，根据平台决定
  return x + 1;
}

int main() {
  std::cout << "___________ Convertor ___________" << std::endl;
  // auto &manager = CFGFileManager::GetInstance();
  // manager.SetRootPath("../");
  // manager.LoadAllCFGFiles();

  // QueryParams queryParams;
  // queryParams.queryFreq = 300;
  // queryParams.queryPower = -30;

  // SlotChannelMap slotChannels = {{0, {0, 1, 3}}};

  // RXStrategy rxStrategy(slotChannels);
  // {
  //   rxStrategy.ResetStrategy();
  //   rxStrategy.SetQueryParams(queryParams);
  //   rxStrategy.BuildStrategy();
  //   rxStrategy.ApplyStrategy();
  // }
  // rxStrategy.GetConfig();

  auto num = NextPowerOfTwo(65537);
  std::cout << num << std::endl;

  return 0;
}
