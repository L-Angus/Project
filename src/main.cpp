#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "../include/DownStrategy/Com_defs.h"
#include "../include/DownStrategy/SDKInterface.h"
#include "../include/DownStrategy/TX_Module_Reg.h"

int main() {
  std::cout << "___________ Convertor ___________" << std::endl;
  RFStimInterface sdk_interface(BoardType::LD, StimMode::CW_MODE);
  sdk_interface.SetCondition({60, 12, FreqUnit::MHz});

  return 0;
}
