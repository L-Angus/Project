#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "RFStrategy/DataFittingManager.hpp"
#include "RFStrategy/HardwareStrategy.h"
#include "RFStrategy/RFModuleConfigure.h"

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
  auto &manager = CFGFileManager::GetInstance();
  manager.SetRootPath("./");
  manager.LoadAllCFGFiles();

  QueryParams queryParams;
  queryParams.queryFreq = 12;
  queryParams.queryPower = 8.5;

  SlotChannelMap slotChannels = {{0, {0, 7, 16}}};

  RXStrategy rxStrategy(slotChannels);
  {
    rxStrategy.ResetStrategy();
    rxStrategy.SetQueryParams(queryParams);
    rxStrategy.BuildStrategy();
    std::cout << "Buidld Strategy" << std::endl;
    rxStrategy.ApplyStrategy();
    std::cout << "Apply Strategy" << std::endl;
  }
  // rxStrategy.GetConfig();
  // auto num = NextPowerOfTwo(65537);
  // std::cout << num << std::endl;

  return 0;
}
